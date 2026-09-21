// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawResolver.h"

bool LDrawResolver::Resolve(
	const FString& RootModelName,
	TMap<FString, FLDrawModel> Models,
	TArray<FLDrawPart>& OutParts,
	FString& OutError
)
{
	OutParts.Reset();
	OutError.Empty();

	const FLDrawModel* RootModel = Models.Find(RootModelName);
	if (!RootModel)
	{
		OutError = FString::Printf(
			TEXT("Root model '%s' not found"),
			*RootModelName
		);
		return false;
	}

	/*
	0 Name:  Tower
	1 <colour> x y z a b c d e f g h i body

	0 Name:  Body
	...

	* For some reason the corresponding name of a sumbmodel
	in a line type 1 is in lowercase... which means that we have to
	make a little hot fix by making every model name in "Models" also
	lowercase.
	*/
	for (auto& Model : Models)
	{
		Model.Key = Model.Key.ToLower();
	}

	ResolveRecursive(
		*RootModel,
		Models,
		FMatrix::Identity, // Start in world space
		16,                // LDraw default: current color
		OutParts
	);

	return true;
}

void LDrawResolver::ResolveRecursive(
	const FLDrawModel& Model,
	const TMap<FString, FLDrawModel>& Models,
	const FMatrix& ParentTransform,
	int32 ParentColor,
	TArray<FLDrawPart>& OutParts
)
{
	for (const FLDrawPart& Part : Model.Parts)
	{
		const int32 ResolvedColor = (Part.Color == 16) ? ParentColor : Part.Color;

		// Convert local part transform to global submodel transform
		FMatrix WorldTransform = Part.Transform * ParentTransform;

		// We check if another submodel lies beneath the current one
		const FLDrawModel* SubModel = Models.Find(Part.Name);

		if (SubModel)
		{
			ResolveRecursive(
				*SubModel,
				Models,
				WorldTransform,
				ResolvedColor,
				OutParts
			);
		}
		else
		{
			FLDrawPart ResolvedPart;
			ResolvedPart.Name = Part.Name;
			ResolvedPart.Color = ResolvedColor;

			/*
			We have to transpose our Unreal transform matrix back into the LDraw transform matrix
			in order to convert to the final FTransform.
			*/
			ResolvedPart.Transform = WorldTransform.GetTransposed();

			OutParts.Add(ResolvedPart);
		}
	}
}

