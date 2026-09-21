// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LDrawPart.h"
#include "LDrawModel.h"

class LDRAWEDITOR_API LDrawResolver
{
public:
	/**
	 * Takes the result of the LDrawParser and flattens the model hierarchy into a list of parts
	 *
	 * @param RootModelName Name of the model containing all submodels
	 * @param Models  Parsed models
	 * @param OutParts The flattened list of parts
	 * @param OutError   Error message if resolving fails
	 * @return true if successful
	 */
	bool Resolve(
		const FString& RootModelName,
		TMap<FString, FLDrawModel> Models,
		TArray<FLDrawPart>& OutParts,
		FString& OutError
	);

private:
	/**
	* Goes recursively through a submodel and resolves all attached parts and submodels
	*
	* @param Model   The model to go through and look for parts and submodels
	* @param Models
	* @param OutModels  Parsed models
	* @param OutError   Error message if parsing fails
	* @return true if successful
	*/
	void ResolveRecursive(
		const FLDrawModel& Model,
		const TMap<FString, FLDrawModel>& Models,
		const FMatrix& ParentTransform,
		int32 ParentColor,
		TArray<FLDrawPart>& OutParts
	);
};
