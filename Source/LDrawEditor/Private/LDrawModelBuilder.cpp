// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawModelBuilder.h"

#include "LDrawAssetEditorSubsystem.h"

#include "LDrawParser.h"
#include "LDrawResolver.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"


UStaticMesh* LDrawModelBuilder::BuildStaticMesh(const FString& FilePath, UObject* InParent, FName InName, EObjectFlags Flags)
{
	if (!InParent)
	{
		UE_LOG(LogLDraw, Error, TEXT("LDrawModelBuilder: No package to create the asset in"));
		return nullptr;
	}

	/*
	The editor already derived the package and the asset name from the imported file,
	so the asset is created in place instead of building a package path by hand.
	*/
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InParent, InName, Flags);
	if (!StaticMesh)
	{
		UE_LOG(LogLDraw, Error, TEXT("LDrawModelBuilder: Could not create StaticMesh '%s'"), *InName.ToString());
		return nullptr;
	}

	if (!FillStaticMesh(FilePath, StaticMesh))
	{
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(StaticMesh);

	return StaticMesh;
}


bool LDrawModelBuilder::FillStaticMesh(const FString& FilePath, UStaticMesh* StaticMesh)
{
	if (!StaticMesh)
	{
		return false;
	}

	// Access custom asset subsystem (meshes, materials, etc.)
	ULDrawAssetEditorSubsystem* Assets =
		GEditor ? GEditor->GetEditorSubsystem<ULDrawAssetEditorSubsystem>() : nullptr;

	if (!Assets)
	{
		UE_LOG(LogLDraw, Error, TEXT("LDrawModelBuilder: LDrawAssetEditorSubsystem is not available"));
		return false;
	}

	// --------------------------------------------------
	// 1. Parse LDraw file
	// --------------------------------------------------
	FLDrawModelData Model;
	FString ParserError;

	if (!FLDrawParser::ParseFile(FilePath, Model, ParserError))
	{
		UE_LOG(LogLDraw, Error, TEXT("LDrawParser: %s"), *ParserError);
		return false;
	}

	// --------------------------------------------------
	// 2. Flatten the hierarchy into a part list
	// --------------------------------------------------
	/*
	The whole root assembly for now. Once the import dialog can select an output
	type, the step range it offers becomes an FLDrawFlattenFilter passed in here.
	*/
	TArray<FLDrawResolvedPart> Parts;
	FString ResolverError;

	if (!FLDrawResolver::Flatten(Model, Parts, ResolverError))
	{
		UE_LOG(LogLDraw, Error, TEXT("LDrawResolver: %s"), *ResolverError);
		return false;
	}

	StaticMesh->Modify();

	// --------------------------------------------------
	// 3. Build combined MeshDescription
	// --------------------------------------------------
	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	TPolygonGroupAttributesRef<FName> PolygonGroupSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

	// Maps materials to polygon groups
	TMap<UMaterialInterface*, FPolygonGroupID> MaterialToGroup;

	// Material and slot name per polygon group, in the order the groups were created
	TArray<UMaterialInterface*> GroupMaterials;
	TArray<FName> GroupSlotNames;

	for (const FLDrawResolvedPart& Part : Parts)
	{
		// Resolve source mesh + material
		UStaticMesh* SourceMesh = Assets->GetMesh(Part.PartID);
		if (!SourceMesh)
		{
			continue;
		}

		UMaterialInterface* Material = Assets->GetMaterial(Part.Color);
		if (!Material)
		{
			continue;
		}

		// Read source mesh LOD0 description
		const FMeshDescription* SourceDesc = SourceMesh->GetMeshDescription(0);
		if (!SourceDesc)
		{
			UE_LOG(LogLDraw, Warning,
				TEXT("LDrawModelBuilder: Part '%s' in line %d has no LOD0 mesh description"),
				*Part.PartID.ToString(),
				Part.SourceLine
			);
			continue;
		}

		// --------------------------------------------------
		// Polygon group / material mapping
		// --------------------------------------------------
		FPolygonGroupID TargetGroupID;
		if (const FPolygonGroupID* Found = MaterialToGroup.Find(Material))
		{
			TargetGroupID = *Found;
		}
		else
		{
			TargetGroupID = MeshDescription.CreatePolygonGroup();

			/*
			The mesh build resolves a polygon group to a material slot by comparing this
			name against FStaticMaterial::ImportedMaterialSlotName, so it has to be unique
			per group. Two distinct material assets can share a name, hence the fallback.
			*/
			FName SlotName = Material->GetFName();
			if (GroupSlotNames.Contains(SlotName))
			{
				SlotName = FName(*FString::Printf(
					TEXT("%s_%d"),
					*Material->GetName(),
					TargetGroupID.GetValue()
				));
			}

			PolygonGroupSlotNames[TargetGroupID] = SlotName;

			MaterialToGroup.Add(Material, TargetGroupID);
			GroupMaterials.Add(Material);
			GroupSlotNames.Add(SlotName);
		}

		// --------------------------------------------------
		// Append source mesh into target mesh
		// --------------------------------------------------
		FStaticMeshOperations::FAppendSettings AppendSettings;

		/*
		The transform already is in Unreal space. AppendMeshDescription applies it to
		positions, normals and tangents, and reverses the winding order for mirrored
		parts, which LDraw expresses as a matrix with a negative determinant.
		*/
		AppendSettings.MeshTransform = Part.Transform;

		// Route every polygon group of the part into the group that holds its material
		AppendSettings.PolygonGroupsDelegate = FAppendPolygonGroupsDelegate::CreateLambda(
			[TargetGroupID](const FMeshDescription& Source, FMeshDescription& Target, PolygonGroupMap& RemapPolygonGroup)
			{
				for (const FPolygonGroupID SourceGroupID : Source.PolygonGroups().GetElementIDs())
				{
					RemapPolygonGroup.Add(SourceGroupID, TargetGroupID);
				}
			});

		FStaticMeshOperations::AppendMeshDescription(*SourceDesc, MeshDescription, AppendSettings);
	}

	// --------------------------------------------------
	// 4. Assign materials to StaticMesh
	// --------------------------------------------------
	TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();
	StaticMaterials.Reset(GroupMaterials.Num());

	for (int32 GroupIndex = 0; GroupIndex < GroupMaterials.Num(); GroupIndex++)
	{
		StaticMaterials.Add(FStaticMaterial(
			GroupMaterials[GroupIndex],
			GroupSlotNames[GroupIndex],
			GroupSlotNames[GroupIndex]
		));
	}

	// --------------------------------------------------
	// 5. Transfer MeshDescription into StaticMesh
	// --------------------------------------------------
	StaticMesh->SetNumSourceModels(1);
	StaticMesh->CreateMeshDescription(0);

	FMeshDescription* DestDesc = StaticMesh->GetMeshDescription(0);
	*DestDesc = MoveTemp(MeshDescription);

	StaticMesh->CommitMeshDescription(0);

	// --------------------------------------------------
	// 6. Build settings
	// --------------------------------------------------
	FStaticMeshSourceModel& SrcModel = StaticMesh->GetSourceModel(0);
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bUseMikkTSpace = false;

	StaticMesh->Build(false);
	StaticMesh->SetLightingGuid();

	// --------------------------------------------------
	// 7. Remember the source file so the asset can be reimported
	// --------------------------------------------------
	if (!StaticMesh->GetAssetImportData())
	{
		StaticMesh->SetAssetImportData(NewObject<UAssetImportData>(StaticMesh, TEXT("AssetImportData")));
	}
	StaticMesh->GetAssetImportData()->Update(FilePath);

	StaticMesh->PostEditChange();
	StaticMesh->MarkPackageDirty();

	return true;
}
