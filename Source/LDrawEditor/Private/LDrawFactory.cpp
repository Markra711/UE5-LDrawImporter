// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawFactory.h"
#include "LDrawModelBuilder.h"

ULDrawFactory::ULDrawFactory()
{
	SupportedClass = UStaticMesh::StaticClass();
	Formats.Add(TEXT("ldr;"));
	bCreateNew = false;
	bEditorImport = true;
}

UObject* ULDrawFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	LDrawModelBuilder ModelBuilder;
	UStaticMesh* Mesh = ModelBuilder.BuildActor(Filename, InParent->GetName());

	return Mesh;
}




