// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LDrawLog.h"
#include "LDrawModelData.h"

/**
 * Reads an LDraw file into FLDrawModelData.
 *
 * The parser keeps the structure of the file: the assembly hierarchy, the build
 * steps and the unresolved color IDs. It does not convert coordinate systems and
 * it does not flatten anything, that is FLDrawResolver's job.
 */
class LDRAW_API FLDrawParser
{
public:
	/**
	 * Parses an LDraw file.
	 *
	 * @param FilePath   Absolute path to .ldr file
	 * @param OutModel   The parsed model
	 * @param OutError   Error message if parsing fails
	 * @return true if successful
	 */
	static bool ParseFile(const FString& FilePath, FLDrawModelData& OutModel, FString& OutError);

	/**
	 * Parses already loaded LDraw lines.
	 *
	 * Separate from ParseFile so the parser can be exercised without touching the
	 * file system.
	 *
	 * @param Lines             The lines of an LDraw file
	 * @param FallbackModelName Name for the root assembly if the file declares none
	 * @param OutModel          The parsed model
	 * @param OutError          Error message if parsing fails
	 * @return true if successful
	 */
	static bool ParseLines(const TArray<FString>& Lines, const FString& FallbackModelName, FLDrawModelData& OutModel, FString& OutError);
};
