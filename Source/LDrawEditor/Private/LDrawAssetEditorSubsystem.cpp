// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawAssetEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"

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

	UE_LOG(LogTemp, Log,
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

	UE_LOG(LogTemp, Log,
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
		UE_LOG(LogTemp, Error,
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
	const FString& PartName
)
{
	const TSoftObjectPtr<UStaticMesh>* Entry =
		MeshCache.Find(FName(*PartName));

	if (!Entry)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LDrawAssetEditorRegistry: Missing mesh for part %s"),
			*PartName
		);
		return nullptr;
	}

	return Entry->LoadSynchronous();
}

UMaterialInterface* ULDrawAssetEditorSubsystem::GetMaterial(
	int32 ColorCode
)
{
	if (UMaterialInterface** Mat =
		MaterialCache.Find(ColorCode))
	{
		return *Mat;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Missing material for color %d"),
		ColorCode
	);

	return nullptr;
}