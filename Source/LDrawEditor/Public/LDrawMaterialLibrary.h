// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LDrawMaterialLibrary.generated.h"

USTRUCT()
struct FLDrawMaterialEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = LDrawMaterialLibrary)
	int32 ColorID = 0;

	UPROPERTY(EditAnywhere, Category = LDrawMaterialLibrary)
	UMaterialInterface* Material = nullptr;
};

UCLASS()
class LDRAWEDITOR_API ULDrawMaterialLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = LDrawMaterialLibrary)
	TArray<FLDrawMaterialEntry> Entries;
};
