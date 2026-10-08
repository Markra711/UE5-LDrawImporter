// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LDrawModelData.generated.h"

/** LDraw color ID that means "inherit the color of the placing assembly". */
#define LDRAW_COLOR_INHERIT 16

/**
 * One type 1 line: a part or a sub-assembly placed into an assembly.
 *
 * A placement is either a leaf (PartID set, AssemblyIndex INDEX_NONE) or a
 * sub-assembly (the other way round), never both.
 */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawPlacement
{
	GENERATED_BODY()

	/*
	Transform relative to the assembly this placement belongs to, in LDraw
	coordinates and stored transposed so that FMatrix multiplication composes the
	hierarchy in the right order.

	Deliberately not an Unreal FTransform: this format mirrors the source file, and
	converting here would both lose shear and spread the coordinate conversion over
	every placement. FLDrawResolver converts once, after composing, and hands out
	FLDrawResolvedPart with a proper Unreal transform.
	*/
	UPROPERTY(VisibleAnywhere, Category = LDraw)
	FMatrix Transform = FMatrix::Identity;

	/*
	LDraw color ID. Deliberately left unresolved: LDRAW_COLOR_INHERIT means the
	color comes from whoever places this assembly, which is what allows a whole
	sub-assembly to be recolored later. Resolving happens in FLDrawResolver.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 Color = LDRAW_COLOR_INHERIT;

	// Part ID without extension or directory, e.g. "3005". NAME_None for a sub-assembly.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	FName PartID;

	// Index into FLDrawModelData::Assemblies. INDEX_NONE for a part.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 AssemblyIndex = INDEX_NONE;

	// 1-based line in the source file this placement came from, for diagnostics
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 SourceLine = 0;

	bool IsSubAssembly() const { return AssemblyIndex != INDEX_NONE; }
};

/**
 * One build step of an assembly, as delimited by the "0 STEP" meta command.
 *
 * Steps are per assembly, not global: a real set of instructions finishes a
 * sub-assembly through its own steps before the step that places it.
 */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawStep
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	TArray<FLDrawPlacement> Placements;
};

/** A model or submodel: a named sequence of build steps. */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawAssembly
{
	GENERATED_BODY()

	// Name as written in the file, e.g. "Brick 1x1"
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	FString Name;

	/*
	Always holds at least one step. An assembly without a "0 STEP" line is a
	single step, so consumers never have to special-case step-less models.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	TArray<FLDrawStep> Steps;

	int32 CountPlacements() const
	{
		int32 Count = 0;
		for (const FLDrawStep& Step : Steps)
		{
			Count += Step.Placements.Num();
		}
		return Count;
	}
};

/**
 * A parsed LDraw file: every assembly it declares, plus which one is the root.
 *
 * This is the whole product of parsing and the input to every builder. It keeps
 * the hierarchy and the step structure, because flattening discards both and
 * neither can be recovered afterwards.
 */
USTRUCT(BlueprintType)
struct LDRAW_API FLDrawModelData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	TArray<FLDrawAssembly> Assemblies;

	// Index of the assembly that contains all others
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LDraw)
	int32 RootAssemblyIndex = INDEX_NONE;

	bool IsValidIndex(int32 AssemblyIndex) const { return Assemblies.IsValidIndex(AssemblyIndex); }

	const FLDrawAssembly* GetRootAssembly() const
	{
		return Assemblies.IsValidIndex(RootAssemblyIndex) ? &Assemblies[RootAssemblyIndex] : nullptr;
	}

	/**
	 * Normalised lookup key for an assembly name.
	 *
	 * A type 1 line references a submodel in lowercase, regardless of the casing
	 * the submodel uses to declare its own name, so every name comparison has to
	 * agree on one normalised form.
	 */
	static FString MakeKey(const FString& Name) { return Name.ToLower(); }
};
