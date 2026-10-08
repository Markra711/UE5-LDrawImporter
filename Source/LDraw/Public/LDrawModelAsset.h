// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LDrawModelData.h"
#include "LDrawModelAsset.generated.h"

class UAssetImportData;

/**
 * A parsed LDraw file, stored as an asset.
 *
 * This is the shared input every output type is built from. It is deliberately
 * faithful to the source file: the hierarchy, the build steps and the unresolved
 * color IDs are all kept, because flattening destroys them and no builder can
 * get them back afterwards.
 */
UCLASS(BlueprintType)
class LDRAW_API ULDrawModelAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	FLDrawModelData Model;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	TObjectPtr<UAssetImportData> AssetImportData;
#endif
};
