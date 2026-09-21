// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawModelBuilder.h"

#include "LDrawAssetEditorSubsystem.h"

#include "LDrawParser.h"
#include "LDrawResolver.h"
#include "LDrawTransformConverter.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "StaticMeshAttributes.h"


UStaticMesh* LDrawModelBuilder::BuildActor(const FString& FilePath, const FString& DestinationPath)
{
	// Access custom asset subsystem (meshes, materials, etc.)
	ULDrawAssetEditorSubsystem* Assets =
		GEditor->GetEditorSubsystem<ULDrawAssetEditorSubsystem>();

	// --------------------------------------------------
	// 1. Parse LDraw file
	// --------------------------------------------------
	LDrawParser Parser;

	FString RootModelName;
	TMap<FString, FLDrawModel> Models;
	FString ParserError;

	if (!Parser.ParseFile(*FilePath, RootModelName, Models, ParserError))
	{
		UE_LOG(LogTemp, Error, TEXT("LDrawParser: %s"), *ParserError);
		return nullptr;
	}

	// --------------------------------------------------
	// 2. Resolve hierarchy into flat part list
	// --------------------------------------------------
	LDrawResolver Resolver;

	TArray<FLDrawPart> Parts;
	FString ResolverError;

	if (!Resolver.Resolve(RootModelName, Models, Parts, ResolverError))
	{
		UE_LOG(LogTemp, Error, TEXT("LDrawResolver: %s"), *ResolverError);
		return nullptr;
	}

	// --------------------------------------------------
	// 3. Asset / package setup
	// --------------------------------------------------
	const FString AssetName = TEXT("SM_") + RootModelName.Replace(TEXT(" "), TEXT(""));

	// Strip asset name from destination path if needed
	FString CleanPath = DestinationPath;
	int32 CutOffIndex;
	CleanPath.FindLastChar('/', CutOffIndex);
	int32 CutOffLength = CleanPath.Len() - CutOffIndex;
	CleanPath.LeftChopInline(CutOffLength);

	const FString FullPackageName = CleanPath / AssetName;

	UPackage* Package = CreatePackage(*FullPackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Package could not be created. FullPackageName: %s"), *FullPackageName);
		return nullptr;
	}
	Package->FullyLoad();

	// Create StaticMesh asset
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(
		Package,
		*AssetName,
		RF_Public | RF_Standalone
	);

	// --------------------------------------------------
	// 4. Build combined MeshDescription
	// --------------------------------------------------
	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	// Maps materials to polygon groups
	TMap<UMaterialInterface*, FPolygonGroupID> MaterialToGroup;

	LDrawTransformConverter TransformConverter;

	for (const FLDrawPart& Part : Parts)
	{
		// Resolve source mesh + material
		FString PartID = Part.Name.Replace(TEXT(".dat"), TEXT(""));
		UStaticMesh* SourceMesh = Assets->GetMesh(PartID);
		if (!SourceMesh)
		{
			continue;
		}

		UMaterialInterface* Material = Assets->GetMaterial(Part.Color);
		if (!Material)
		{
			continue;
		}

		// Convert LDraw transform to UE transform
		FTransform UETransform = TransformConverter.Convert(Part.Transform);

		// Read source mesh LOD0 description
		FMeshDescription* SourceDesc = SourceMesh->GetMeshDescription(0);

		// Append source mesh into target mesh
		AppendMesh(
			MeshDescription,
			*SourceDesc,
			UETransform,
			Material,
			MaterialToGroup
		);
	}

	// --------------------------------------------------
	// 5. Assign materials to StaticMesh
	// --------------------------------------------------
	for (const auto& Pair : MaterialToGroup)
	{
		FStaticMaterial StaticMat(Pair.Key);
		StaticMat.MaterialSlotName = Pair.Key->GetFName();
		StaticMesh->GetStaticMaterials().Add(StaticMat);
	}

	// --------------------------------------------------
	// 6. Transfer MeshDescription into StaticMesh
	// --------------------------------------------------
	StaticMesh->SetNumSourceModels(1);
	StaticMesh->CreateMeshDescription(0);

	FMeshDescription* DestDesc = StaticMesh->GetMeshDescription(0);
	*DestDesc = MeshDescription;

	StaticMesh->CommitMeshDescription(0);

	// --------------------------------------------------
	// 7. Build settings
	// --------------------------------------------------
	FStaticMeshSourceModel& SrcModel = StaticMesh->GetSourceModel(0);
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bUseMikkTSpace = false;

	StaticMesh->Build(false);
	StaticMesh->SetLightingGuid();

	// --------------------------------------------------
	// 8. Register asset
	// --------------------------------------------------
	FAssetRegistryModule::AssetCreated(StaticMesh);
	Package->MarkPackageDirty();

	return StaticMesh;
}


void LDrawModelBuilder::AppendMesh(
	FMeshDescription& Target,
	FMeshDescription& Source,
	const FTransform& Transform,
	UMaterialInterface* Material,
	TMap<UMaterialInterface*, FPolygonGroupID>& MaterialToGroup
)
{
	// Access attributes for source and target meshes
	FStaticMeshAttributes SrcAttr(Source);
	SrcAttr.Register();

	FStaticMeshAttributes DstAttr(Target);
	DstAttr.Register();

	auto SrcPositions = SrcAttr.GetVertexPositions();
	auto SrcNormals = SrcAttr.GetVertexInstanceNormals();
	auto SrcUVs = SrcAttr.GetVertexInstanceUVs();

	auto DstPositions = DstAttr.GetVertexPositions();
	auto DstNormals = DstAttr.GetVertexInstanceNormals();
	auto DstUVs = DstAttr.GetVertexInstanceUVs();

	// --------------------------------------------------
	// Polygon group / material mapping
	// --------------------------------------------------
	FPolygonGroupID TargetGroupID;
	if (FPolygonGroupID* Found = MaterialToGroup.Find(Material))
	{
		TargetGroupID = *Found;
	}
	else
	{
		TargetGroupID = Target.CreatePolygonGroup();
		DstAttr.GetPolygonGroupMaterialSlotNames()[TargetGroupID] =
			Material->GetFName();

		MaterialToGroup.Add(Material, TargetGroupID);
	}

	// --------------------------------------------------
	// Vertex duplication and transform
	// --------------------------------------------------
	TMap<FVertexID, FVertexID> VertexMap;

	for (FVertexID SrcVertexID : Source.Vertices().GetElementIDs())
	{
		const FVector3f SrcPos = SrcPositions[SrcVertexID];
		const FVector   NewPos = Transform.TransformPosition((FVector)SrcPos);

		FVertexID DstVertexID = Target.CreateVertex();
		DstPositions[DstVertexID] = (FVector3f)NewPos;

		VertexMap.Add(SrcVertexID, DstVertexID);
	}

	// --------------------------------------------------
	// Polygons + vertex instances
	// --------------------------------------------------
	for (FPolygonID SrcPolyID : Source.Polygons().GetElementIDs())
	{
		TArray<FVertexInstanceID> NewVertexInstances;

		for (FVertexInstanceID SrcVI :
		Source.GetPolygonVertexInstances(SrcPolyID))
		{
			FVertexID SrcVertexID =
				Source.GetVertexInstanceVertex(SrcVI);

			FVertexID DstVertexID = VertexMap[SrcVertexID];
			FVertexInstanceID DstVI =
				Target.CreateVertexInstance(DstVertexID);

			// Transform normal (no scale!) and normalize
			FVector NewNormal =
				Transform.TransformVectorNoScale(
					(FVector)SrcNormals[SrcVI]
				).GetSafeNormal();

			DstNormals[DstVI] = (FVector3f)NewNormal;

			// Copy UV channel 0
			DstUVs.Set(DstVI, 0, SrcUVs.Get(SrcVI, 0));

			NewVertexInstances.Add(DstVI);
		}

		Target.CreatePolygon(TargetGroupID, NewVertexInstances);
	}
}

