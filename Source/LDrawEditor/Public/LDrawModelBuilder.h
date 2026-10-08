// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

class LDrawModelBuilder
{
public:
    /**
    * Combines all other methods and classes to create the final LDraw model as a StaticMesh asset.
    *
    * The package, name and flags are the ones the editor handed to the factory, so the
    * editor stays in charge of where the asset lands and what it is called.
    *
    * @param FilePath Absolute path to .ldr file
    * @param InParent Package to create the asset in
    * @param InName   Name of the asset
    * @param Flags    Object flags for the asset
    * @return The created StaticMesh, or nullptr if the file could not be built
    */
    UStaticMesh* BuildStaticMesh(const FString& FilePath, UObject* InParent, FName InName, EObjectFlags Flags);

    /**
    * Replaces the geometry and materials of an existing StaticMesh with the contents of an LDraw file.
    *
    * Used both for the initial import and for reimporting, which is why it rebuilds an
    * asset in place instead of creating one.
    *
    * @param FilePath   Absolute path to .ldr file
    * @param StaticMesh The mesh to rebuild
    * @return true if successful
    */
    bool FillStaticMesh(const FString& FilePath, UStaticMesh* StaticMesh);
};
