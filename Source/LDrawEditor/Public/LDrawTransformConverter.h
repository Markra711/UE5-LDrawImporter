// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class LDrawTransformConverter
{
public:
	/**
	* Converts LDraw world-space matrices into Unreal Engine transforms.
	*
	* @param LDrawWorldMatrix
	* @return FTransform
	*/
	FTransform Convert(const FMatrix& LDrawWorldMatrix);
};
