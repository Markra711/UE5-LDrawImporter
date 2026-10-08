// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawResolver.h"

#include "LDrawLog.h"
#include "LDrawTransformConverter.h"

bool FLDrawResolver::Flatten(
	const FLDrawModelData& Model,
	TArray<FLDrawResolvedPart>& OutParts,
	FString& OutError
)
{
	return Flatten(Model, FLDrawFlattenFilter(), OutParts, OutError);
}

bool FLDrawResolver::Flatten(
	const FLDrawModelData& Model,
	const FLDrawFlattenFilter& Filter,
	TArray<FLDrawResolvedPart>& OutParts,
	FString& OutError
)
{
	OutParts.Reset();
	OutError.Empty();

	const int32 AssemblyIndex = (Filter.AssemblyIndex == INDEX_NONE)
		? Model.RootAssemblyIndex
		: Filter.AssemblyIndex;

	if (!Model.IsValidIndex(AssemblyIndex))
	{
		OutError = FString::Printf(
			TEXT("Assembly index %d does not exist, the model holds %d"),
			AssemblyIndex,
			Model.Assemblies.Num()
		);
		return false;
	}

	const FLDrawAssembly& Assembly = Model.Assemblies[AssemblyIndex];

	const int32 FirstStep = FMath::Max(0, Filter.FirstStep);
	const int32 LastStep = (Filter.LastStep == INDEX_NONE)
		? Assembly.Steps.Num() - 1
		: FMath::Min(Filter.LastStep, Assembly.Steps.Num() - 1);

	if (FirstStep > LastStep)
	{
		OutError = FString::Printf(
			TEXT("Step range %d to %d is empty, assembly '%s' holds %d steps"),
			Filter.FirstStep,
			Filter.LastStep,
			*Assembly.Name,
			Assembly.Steps.Num()
		);
		return false;
	}

	TArray<int32> AssemblyStack;

	FlattenAssembly(
		Model,
		AssemblyIndex,
		FirstStep,
		LastStep,
		FMatrix::Identity,       // Start in model space
		LDRAW_COLOR_INHERIT,     // LDraw default: current color
		AssemblyStack,
		OutParts
	);

	return true;
}

void FLDrawResolver::FlattenAssembly(
	const FLDrawModelData& Model,
	int32 AssemblyIndex,
	int32 FirstStep,
	int32 LastStep,
	const FMatrix& ParentTransform,
	int32 ParentColor,
	TArray<int32>& AssemblyStack,
	TArray<FLDrawResolvedPart>& OutParts
)
{
	/*
	A submodel that references itself or one of its own ancestors would recurse
	until the stack runs out. Such files are easy to produce by copy and paste in
	an editor, so the chain being walked is tracked and a repeat is dropped.
	*/
	if (AssemblyStack.Contains(AssemblyIndex))
	{
		UE_LOG(LogLDraw, Error,
			TEXT("LDrawResolver: Assembly '%s' references itself, skipping to break the cycle"),
			*Model.Assemblies[AssemblyIndex].Name
		);
		return;
	}

	AssemblyStack.Push(AssemblyIndex);

	const FLDrawAssembly& Assembly = Model.Assemblies[AssemblyIndex];

	for (int32 StepIndex = FirstStep; StepIndex <= LastStep; StepIndex++)
	{
		const FLDrawStep& Step = Assembly.Steps[StepIndex];

		for (int32 PlacementIndex = 0; PlacementIndex < Step.Placements.Num(); PlacementIndex++)
		{
			const FLDrawPlacement& Placement = Step.Placements[PlacementIndex];

			// Color 16 takes the color of whoever placed this assembly
			const int32 ResolvedColor = (Placement.Color == LDRAW_COLOR_INHERIT)
				? ParentColor
				: Placement.Color;

			// Compose the local transform into the parent assembly
			const FMatrix WorldTransform = Placement.Transform * ParentTransform;

			if (Placement.IsSubAssembly())
			{
				/*
				A sub-assembly is finished when it is placed, so it contributes all of
				its own steps regardless of the step range of its parent.
				*/
				const FLDrawAssembly& SubAssembly = Model.Assemblies[Placement.AssemblyIndex];

				FlattenAssembly(
					Model,
					Placement.AssemblyIndex,
					0,
					SubAssembly.Steps.Num() - 1,
					WorldTransform,
					ResolvedColor,
					AssemblyStack,
					OutParts
				);
			}
			else
			{
				FLDrawResolvedPart ResolvedPart;

				ResolvedPart.PartID = Placement.PartID;
				ResolvedPart.Color = ResolvedColor;

				/*
				The composed matrix is transposed back into LDraw notation, which is
				what the converter reads, and only then turned into Unreal space.
				*/
				ResolvedPart.Transform = FLDrawTransformConverter::Convert(WorldTransform.GetTransposed());

				ResolvedPart.AssemblyIndex = AssemblyIndex;
				ResolvedPart.StepIndex = StepIndex;
				ResolvedPart.PlacementIndex = PlacementIndex;
				ResolvedPart.SourceLine = Placement.SourceLine;

				OutParts.Add(ResolvedPart);
			}
		}
	}

	AssemblyStack.Pop();
}
