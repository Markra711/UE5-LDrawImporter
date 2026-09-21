// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct LDRAWEDITOR_API FLDrawPart
{
	// LDraw Color ID (e.g. 15)
	int32 Color;

	/*
		LDraw Transform of form:

		| R00 R01 R02 LX |
		| R10 R11 R12 LY |
		| R20 R21 R22 LZ |
		|  0   0   0   1 |
	*/
	FMatrix Transform;

	// LDraw Part Name (e.g. "3005.dat")
	FString Name;
};
