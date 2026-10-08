// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawFactory.h"
#include "LDrawModelBuilder.h"

#include "EditorFramework/AssetImportData.h"
#include "LDrawLog.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"

namespace
{
	// File extension this factory imports, without the dot
	const TCHAR* LDrawFileExtension = TEXT("ldr");

	/**
	 * Returns the StaticMesh behind an object if it was imported from an LDraw file.
	 *
	 * Reimport handlers are asked about every object of their supported class, so the
	 * source file has to be checked as well. Claiming a mesh that came from somewhere
	 * else would shadow the handler that actually owns it.
	 */
	UStaticMesh* GetLDrawStaticMesh(UObject* Obj, FString& OutFilename)
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(Obj);
		if (!StaticMesh || !StaticMesh->GetAssetImportData())
		{
			return nullptr;
		}

		const FString Filename = StaticMesh->GetAssetImportData()->GetFirstFilename();
		if (Filename.IsEmpty() || !FPaths::GetExtension(Filename).Equals(LDrawFileExtension, ESearchCase::IgnoreCase))
		{
			return nullptr;
		}

		OutFilename = Filename;
		return StaticMesh;
	}
}

ULDrawFactory::ULDrawFactory()
{
	SupportedClass = UStaticMesh::StaticClass();
	Formats.Add(FString(LDrawFileExtension) + TEXT(";LDraw Model"));
	bCreateNew = false;
	bEditorImport = true;
}

UObject* ULDrawFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	LDrawModelBuilder ModelBuilder;
	UStaticMesh* Mesh = ModelBuilder.BuildStaticMesh(Filename, InParent, InName, Flags);

	if (!Mesh && Warn)
	{
		Warn->Logf(ELogVerbosity::Error,
			TEXT("Failed to import LDraw file '%s', see the log for details"),
			*Filename
		);
	}

	return Mesh;
}

bool ULDrawFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	FString Filename;
	if (!GetLDrawStaticMesh(Obj, Filename))
	{
		return false;
	}

	OutFilenames.Add(Filename);
	return true;
}

void ULDrawFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	FString Filename;
	UStaticMesh* StaticMesh = GetLDrawStaticMesh(Obj, Filename);

	if (StaticMesh && NewReimportPaths.Num() == 1)
	{
		StaticMesh->GetAssetImportData()->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type ULDrawFactory::Reimport(UObject* Obj)
{
	FString Filename;
	UStaticMesh* StaticMesh = GetLDrawStaticMesh(Obj, Filename);

	if (!StaticMesh)
	{
		return EReimportResult::Failed;
	}

	if (!FPaths::FileExists(Filename))
	{
		UE_LOG(LogLDraw, Error,
			TEXT("LDrawFactory: Source file '%s' of '%s' no longer exists"),
			*Filename,
			*StaticMesh->GetName()
		);
		return EReimportResult::Failed;
	}

	LDrawModelBuilder ModelBuilder;
	if (!ModelBuilder.FillStaticMesh(Filename, StaticMesh))
	{
		return EReimportResult::Failed;
	}

	return EReimportResult::Succeeded;
}

int32 ULDrawFactory::GetPriority() const
{
	return ImportPriority;
}
