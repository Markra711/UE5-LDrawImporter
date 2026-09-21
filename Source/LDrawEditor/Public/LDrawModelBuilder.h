// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

class LDrawModelBuilder
{
public:
    /**
    * Combines all other methods and classes to create the final LDraw model as a StaticMesh
    *
    * @param FilePath Absolute path to .ldr file
    * @param DestinationPath Where the asset is supposed to be saved e.g. /Game/Models/Brick_1x1
    * @return UStaticMesh*
    */
    UStaticMesh* BuildActor(const FString& FilePath, const FString& DestinationPath);

private:
    /**
    * Adds a MeshDescription to an existing one
    *
    * @param Target  The MeshDescription on which we add the new one
    * @param Source  The new MeshDescription which we add to the existing one
    * @param Transform The FTransform of the new MeshDescription
    * @param Material The UMaterialInterface of the new MeshDescription
    */
    void AppendMesh(
        FMeshDescription& Target,
        FMeshDescription& Source,
        const FTransform& Transform,
        UMaterialInterface* Material,
        TMap<UMaterialInterface*, FPolygonGroupID>& MaterialToGroup
    );
};