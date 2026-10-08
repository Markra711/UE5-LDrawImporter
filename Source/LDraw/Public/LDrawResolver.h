// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LDrawModelData.h"
#include "LDrawResolver.generated.h"

/**
 * One part of a flattened model: color resolved, transform in Unreal space.
 *
 * Keeps where it came from, so a built instance can be mapped back to the place
 * in the model that produced it. That mapping is what lets a consumer say "this
 * instance is the brick from step 7 of sub-assembly 3".
 */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawResolvedPart
{
	GENERATED_BODY()

	// Part ID without extension or directory, e.g. "3005"
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	FName PartID;

	// Resolved LDraw color ID, never LDRAW_COLOR_INHERIT
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 Color = 0;

	// Unreal space, relative to the origin of the flattened assembly
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	FTransform Transform = FTransform::Identity;

	// Assembly that placed this part
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 AssemblyIndex = INDEX_NONE;

	// Step within that assembly
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 StepIndex = INDEX_NONE;

	// Placement within that step
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 PlacementIndex = INDEX_NONE;

	// Line in the source file, for diagnostics
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 SourceLine = 0;
};

/**
 * Selects which part of a model to flatten.
 *
 * The step range applies to the selected assembly only. A sub-assembly is a
 * finished unit at the moment it is placed, so it always contributes all of its
 * own steps, which is also how printed instructions work.
 */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawFlattenFilter
{
	GENERATED_BODY()

	// Assembly to flatten. INDEX_NONE flattens the root assembly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LDraw)
	int32 AssemblyIndex = INDEX_NONE;

	// First step to include
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LDraw, meta = (ClampMin = 0))
	int32 FirstStep = 0;

	// Last step to include. INDEX_NONE includes every step from FirstStep onwards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LDraw)
	int32 LastStep = INDEX_NONE;
};

/**
 * Flattens the assembly hierarchy of a parsed model into a list of parts.
 *
 * This is where the two things the format deliberately keeps unresolved are
 * applied: color inheritance and the LDraw to Unreal coordinate conversion.
 */
class LDRAW_API FLDrawResolver
{
public:
	/**
	 * Flattens the selected part of a model.
	 *
	 * @param Model    The parsed model
	 * @param Filter   Which assembly and which steps to include
	 * @param OutParts The flattened list of parts
	 * @param OutError Error message if flattening fails
	 * @return true if successful
	 */
	static bool Flatten(
		const FLDrawModelData& Model,
		const FLDrawFlattenFilter& Filter,
		TArray<FLDrawResolvedPart>& OutParts,
		FString& OutError
	);

	/** Flattens the whole root assembly. */
	static bool Flatten(
		const FLDrawModelData& Model,
		TArray<FLDrawResolvedPart>& OutParts,
		FString& OutError
	);

private:
	/**
	 * Walks one assembly and resolves its placements and sub-assemblies.
	 *
	 * @param AssemblyStack Assemblies currently being walked, to break reference cycles
	 */
	static void FlattenAssembly(
		const FLDrawModelData& Model,
		int32 AssemblyIndex,
		int32 FirstStep,
		int32 LastStep,
		const FMatrix& ParentTransform,
		int32 ParentColor,
		TArray<int32>& AssemblyStack,
		TArray<FLDrawResolvedPart>& OutParts
	);
};
