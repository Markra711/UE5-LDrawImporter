// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class LDRAW_API FLDrawTransformConverter
{
public:
	/**
	* Converts an LDraw world-space matrix into an Unreal transform.
	*
	* @param LDrawWorldMatrix Composed transform in LDraw notation and coordinates
	* @return The equivalent Unreal transform
	*/
	static FTransform Convert(const FMatrix& LDrawWorldMatrix);
};
