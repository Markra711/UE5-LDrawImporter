// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawParser.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/**
	 * Joins Tokens[StartIndex] up to the last token with single spaces.
	 *
	 * LDraw names may contain whitespace, so everything after the fixed-width
	 * portion of a line belongs to the name.
	 */
	FString JoinTokens(const TArray<FString>& Tokens, int32 StartIndex)
	{
		FString Result;

		for (int32 i = StartIndex; i < Tokens.Num(); i++)
		{
			if (i > StartIndex)
			{
				Result.AppendChar(TEXT(' '));
			}

			Result.Append(Tokens[i]);
		}

		return Result;
	}
}

bool FLDrawParser::ParseFile(const FString& FilePath, FLDrawModelData& OutModel, FString& OutError)
{
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		OutError = FString::Printf(
			TEXT("Failed to load file to string, FilePath: '%s'"),
			*FilePath
		);
		return false;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	return ParseLines(Lines, FPaths::GetBaseFilename(FilePath), OutModel, OutError);
}

bool FLDrawParser::ParseLines(const TArray<FString>& Lines, const FString& FallbackModelName, FLDrawModelData& OutModel, FString& OutError)
{
	OutError.Empty();
	OutModel = FLDrawModelData();

	// Normalised assembly name -> index into OutModel.Assemblies
	TMap<FString, int32> AssemblyLookup;

	// Assembly that the following lines belong to
	int32 CurrentAssembly = INDEX_NONE;

	// Registers an additional name under which an assembly can be referenced
	auto AddAlias = [&AssemblyLookup](int32 AssemblyIndex, const FString& Name)
	{
		const FString Key = FLDrawModelData::MakeKey(Name);
		if (!AssemblyLookup.Contains(Key))
		{
			AssemblyLookup.Add(Key, AssemblyIndex);
		}
	};

	// Opens an assembly, reusing it if the name was already declared
	auto BeginAssembly = [&OutModel, &AssemblyLookup, &AddAlias](const FString& Name) -> int32
	{
		if (const int32* Existing = AssemblyLookup.Find(FLDrawModelData::MakeKey(Name)))
		{
			/*
			A name declared twice continues the assembly that already exists instead
			of shadowing it, which would silently drop everything declared first.
			*/
			UE_LOG(LogLDraw, Warning,
				TEXT("LDrawParser: Assembly '%s' is declared more than once, continuing the existing one"),
				*Name
			);
			return *Existing;
		}

		FLDrawAssembly Assembly;
		Assembly.Name = Name;

		// An assembly always holds at least one step, so consumers need no special case
		Assembly.Steps.AddDefaulted();

		const int32 AssemblyIndex = OutModel.Assemblies.Add(MoveTemp(Assembly));
		AddAlias(AssemblyIndex, Name);

		// The first assembly in the file is the root, all others are submodels
		if (OutModel.RootAssemblyIndex == INDEX_NONE)
		{
			OutModel.RootAssemblyIndex = AssemblyIndex;
		}

		return AssemblyIndex;
	};

	// --------------------------------------------------
	// Pass 1: read assemblies, steps and placements
	// --------------------------------------------------
	for (int32 LineIndex = 0; LineIndex < Lines.Num(); LineIndex++)
	{
		const FString& Line = Lines[LineIndex];

		TArray<FString> Tokens;

		// ParseIntoArrayWS culls empty tokens, so whitespace needs no trimming
		Line.ParseIntoArrayWS(Tokens);

		// Blank and whitespace-only lines carry no tokens at all
		if (Tokens.Num() == 0)
		{
			continue;
		}

		const FString& Type = Tokens[0];

		// Meta command
		if (Type == TEXT("0"))
		{
			// A bare "0" is how LDraw files write a blank line
			if (Tokens.Num() < 2)
			{
				continue;
			}

			const FString& Meta = Tokens[1];

			/*
			"0 FILE <name>" is the MPD block delimiter that Stud.io and LDCad write,
			and it is what type 1 lines reference, so it always opens an assembly.
			*/
			if (Meta == TEXT("FILE") && Tokens.Num() >= 3)
			{
				CurrentAssembly = BeginAssembly(JoinTokens(Tokens, 2));
			}
			/*
			"0 Name: <name>" opens an assembly in files without "0 FILE". Inside an
			MPD block it only describes the block that "0 FILE" already opened, so it
			is registered as an alias instead of opening a second assembly. An
			assembly that already holds placements means the file uses "0 Name:" as
			its delimiter, so the next one starts a new assembly.
			*/
			else if (Meta == TEXT("Name:") && Tokens.Num() >= 3)
			{
				const FString Name = JoinTokens(Tokens, 2);

				if (CurrentAssembly == INDEX_NONE || OutModel.Assemblies[CurrentAssembly].CountPlacements() > 0)
				{
					CurrentAssembly = BeginAssembly(Name);
				}
				else
				{
					AddAlias(CurrentAssembly, Name);
				}
			}
			// "0 STEP" ends the current build step of the current assembly
			else if (Meta == TEXT("STEP"))
			{
				if (CurrentAssembly != INDEX_NONE)
				{
					FLDrawAssembly& Assembly = OutModel.Assemblies[CurrentAssembly];

					// Consecutive STEP lines must not produce empty steps
					if (Assembly.Steps.Last().Placements.Num() > 0)
					{
						Assembly.Steps.AddDefaulted();
					}
				}
			}
		}
		// Part or submodel placement: 1 <colour> x y z a b c d e f g h i <file>
		else if (Type == TEXT("1"))
		{
			// 14 fixed tokens plus at least one name token
			if (Tokens.Num() < 15)
			{
				UE_LOG(LogLDraw, Warning,
					TEXT("LDrawParser: Skipping malformed part line %d, expected at least 15 tokens but got %d: '%s'"),
					LineIndex + 1,
					Tokens.Num(),
					*Line
				);
				continue;
			}

			/*
			A file holding a single model needs no name meta at all, in which case the
			placements belong to an implicit assembly named after the file.
			*/
			if (CurrentAssembly == INDEX_NONE)
			{
				CurrentAssembly = BeginAssembly(FallbackModelName);
			}

			FVector Location = FVector::ZeroVector;

			Location.X = FCString::Atof(*Tokens[2]);
			Location.Y = FCString::Atof(*Tokens[3]);
			Location.Z = FCString::Atof(*Tokens[4]);

			FMatrix Rotation = FMatrix::Identity;

			Rotation.M[0][0] = FCString::Atof(*Tokens[5]);
			Rotation.M[0][1] = FCString::Atof(*Tokens[6]);
			Rotation.M[0][2] = FCString::Atof(*Tokens[7]);

			Rotation.M[1][0] = FCString::Atof(*Tokens[8]);
			Rotation.M[1][1] = FCString::Atof(*Tokens[9]);
			Rotation.M[1][2] = FCString::Atof(*Tokens[10]);

			Rotation.M[2][0] = FCString::Atof(*Tokens[11]);
			Rotation.M[2][1] = FCString::Atof(*Tokens[12]);
			Rotation.M[2][2] = FCString::Atof(*Tokens[13]);

			/*
			Given a part description of generic form: 1 <colour> x y z a b c d e f g h i <file>
			the LDraw transform matrix looks like this:

				/ a b c x \
				| d e f y |
				| g h i z |
				\ 0 0 0 1 /

			Unreal uses the transposed form for its matrix operations, which is what is
			needed to compose the hierarchy:

				/ a d g 0 \
				| b e h 0 |
				| c f i 0 |
				\ x y z 1 /

			The LDraw matrix is the classic mathematical notation while Unreal works
			with row vectors, so transposing is all it takes. The matrix stays in LDraw
			coordinates, FLDrawResolver converts to Unreal space once the hierarchy is
			composed.
			*/
			FLDrawPlacement Placement;

			Placement.Transform = Rotation.GetTransposed();
			Placement.Transform.SetOrigin(Location);

			// Colors stay unresolved, FLDrawResolver applies the inheritance
			Placement.Color = FCString::Atoi(*Tokens[1]);

			Placement.SourceLine = LineIndex + 1;

			/*
			The reference may be a part file or a submodel and may contain whitespace.
			Pass 2 decides which it is, so it is parked in PartID for now.
			*/
			Placement.PartID = FName(*JoinTokens(Tokens, 14));

			OutModel.Assemblies[CurrentAssembly].Steps.Last().Placements.Add(Placement);
		}
	}

	if (OutModel.RootAssemblyIndex == INDEX_NONE)
	{
		OutError = TEXT("File declares no model");
		return false;
	}

	// --------------------------------------------------
	// Pass 2: resolve references and drop empty trailing steps
	// --------------------------------------------------
	/*
	References can only be resolved once every assembly is known, because a submodel
	is usually declared after the model that places it.
	*/
	for (FLDrawAssembly& Assembly : OutModel.Assemblies)
	{
		for (FLDrawStep& Step : Assembly.Steps)
		{
			for (FLDrawPlacement& Placement : Step.Placements)
			{
				const FString Reference = Placement.PartID.ToString();

				if (const int32* SubAssembly = AssemblyLookup.Find(FLDrawModelData::MakeKey(Reference)))
				{
					Placement.AssemblyIndex = *SubAssembly;
					Placement.PartID = NAME_None;
				}
				else
				{
					/*
					A part reference carries an extension and may carry a directory for
					subparts and primitives, for example s\3005s01.dat. The part library
					is keyed by the bare ID.
					*/
					Placement.PartID = FName(*FPaths::GetBaseFilename(Reference));
				}
			}
		}

		// A STEP line at the end of an assembly leaves an empty step behind
		if (Assembly.Steps.Num() > 1 && Assembly.Steps.Last().Placements.IsEmpty())
		{
			Assembly.Steps.Pop();
		}
	}

	return true;
}
