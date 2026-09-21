// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "LDrawMaterialLibrary.h"
#include "LDrawAssetEditorSubsystem.generated.h"

/**
 *
 */
UCLASS()
class LDRAWEDITOR_API ULDrawAssetEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // UEngineSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Lookups
    UStaticMesh* GetMesh(const FString& PartName);
    UMaterialInterface* GetMaterial(int32 ColorCode);

private:
    // Mesh cache: PartName -> Mesh
    TMap<FName, TSoftObjectPtr<UStaticMesh>> MeshCache;

    // Material cache: ColorCode -> Material
    TMap<int32, UMaterialInterface*> MaterialCache;

    void OnAssetRegistryReady();

    void BuildMeshCache();
    void BuildMaterialCache();

    // Config
    FString LDrawPartsPath = TEXT("/LDraw/LDrawParts");
    TSoftObjectPtr<ULDrawMaterialLibrary> MaterialLibraryAsset;
};
