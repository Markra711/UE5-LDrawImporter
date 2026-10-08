// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawAssetEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "LDrawLog.h"

void ULDrawAssetEditorSubsystem::Initialize(
	FSubsystemCollectionBase& Collection
)
{
	Super::Initialize(Collection);

	// Set path to ColorMap asset
	MaterialLibraryAsset = TSoftObjectPtr<ULDrawMaterialLibrary>(
		FSoftObjectPath(
			TEXT("/LDraw/LDrawMaterials/LDrawMaterialLibrary.LDrawMaterialLibrary")
		)
	);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	AssetRegistryModule.Get().OnFilesLoaded().AddUObject(
		this,
		&ULDrawAssetEditorSubsystem::OnAssetRegistryReady
	);
}

void ULDrawAssetEditorSubsystem::OnAssetRegistryReady()
{
	BuildMeshCache();
	BuildMaterialCache();

	UE_LOG(LogLDraw, Log,
		TEXT("LDrawAssetSubsystem initialized: %d meshes, %d materials"),
		MeshCache.Num(),
		MaterialCache.Num()
	);
}

void ULDrawAssetEditorSubsystem::Deinitialize()
{
	MeshCache.Empty();
	MaterialCache.Empty();

	Super::Deinitialize();
}

void ULDrawAssetEditorSubsystem::BuildMeshCache()
{
	MeshCache.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(*LDrawPartsPath);
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetData;
	AssetRegistry.GetAssets(Filter, AssetData);

	for (const FAssetData& Asset : AssetData)
	{
		MeshCache.Add(
			Asset.AssetName,
			TSoftObjectPtr<UStaticMesh>(Asset.GetSoftObjectPath())
		);
	}

	UE_LOG(LogLDraw, Log,
		TEXT("LDraw mesh cache built: %d meshes"),
		MeshCache.Num()
	);
}

void ULDrawAssetEditorSubsystem::BuildMaterialCache()
{
	MaterialCache.Empty();

	ULDrawMaterialLibrary* MaterialLibrary = MaterialLibraryAsset.LoadSynchronous();
	if (!MaterialLibrary)
	{
		UE_LOG(LogLDraw, Error,
			TEXT("Failed to load LDrawColorMap asset")
		);
		return;
	}

	for (const FLDrawMaterialEntry& Entry : MaterialLibrary->Entries)
	{
		if (Entry.Material)
		{
			MaterialCache.Add(
				Entry.ColorID,
				Entry.Material
			);
		}
	}
}

UStaticMesh* ULDrawAssetEditorSubsystem::GetMesh(
	FName PartID
)
{
	const TSoftObjectPtr<UStaticMesh>* Entry =
		MeshCache.Find(PartID);

	if (!Entry)
	{
		UE_LOG(LogLDraw, Warning,
			TEXT("LDrawAssetEditorSubsystem: Missing mesh for part %s"),
			*PartID.ToString()
		);
		return nullptr;
	}

	return Entry->LoadSynchronous();
}

UMaterialInterface* ULDrawAssetEditorSubsystem::GetMaterial(
	int32 ColorCode
)
{
	if (const TObjectPtr<UMaterialInterface>* Mat =
		MaterialCache.Find(ColorCode))
	{
		return *Mat;
	}

	UE_LOG(LogLDraw, Warning,
		TEXT("Missing material for color %d"),
		ColorCode
	);

	return nullptr;
}