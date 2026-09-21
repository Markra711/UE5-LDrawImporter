// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawParser.h"

// #include "Misc/FileHelper.h"
// #include "Misc/Paths.h"

bool LDrawParser::ParseFile(const FString& FilePath, FString& RootModelName, TMap<FString, FLDrawModel>& OutModels, FString& OutError)
{
	OutError.Empty();

	// Example file content 
	/*
	0 Name:  Brick_1x1
	1 15 2.000000 4.000000 6.000000 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 0.000000 0.000000 1.000000 3005.dat
	*/
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

	/*
	Lines[0] = "0 Name:  Brick_1x1"
	Lines[1] = "1 15 2.000000 4.000000 6.000000 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 0.000000 0.000000 1.000000 3005.dat"
	*/

	FString CurrentModelName;

	for (FString Line : Lines)
	{
		Line = Line.TrimStartAndEnd();

		TArray<FString> Tokens;
		Line.ParseIntoArrayWS(Tokens);

		/*
		1. Tokens = "0", "Name:", "Brick_1x1"
		2. Tokens = "1", "15", "2.000000", "4.000000", "6.000000", "1.000000", "0.000000", "0.000000", "0.000000", "1.000000", "0.000000", "0.000000", "0.000000", "1.000000", "3005.dat"
		*/

		const FString& Type = Tokens[0];

		/*
		1. Type = "0"
		2. Type = "1"
		*/

		// Model or Submodel description
		if (Type == TEXT("0"))
		{
			if (Tokens[1] == TEXT("Name:"))
			{
				FLDrawModel Model;
				Model.Name = Tokens[2]; // Model.Name = "Brick_1x1"

				// Check if the model name has whitespaces which we just combine e.g. "Brick", "1x1" -> "Brick 1x1"
				if (Tokens.Num() > 2)
				{
					for (int32 i = 3; i < Tokens.Num(); i++)
					{
						Model.Name.Append(TEXT(" ") + Tokens[i]);
					}
				}

				// Every part description that occurs after this line will be connected to the "CurrentModelName"
				CurrentModelName = Model.Name;

				// This is for deciding if the current model is a submodel or the (root)model
				if (RootModelName.IsEmpty())
				{
					RootModelName = CurrentModelName;
				}

				OutModels.Add(CurrentModelName, Model);
			}
		}
		// Part description
		else if (Type == TEXT("1"))
		{
			int32 Color = FCString::Atoi(*Tokens[1]); // Color = 15

			FVector Location = FVector::ZeroVector;

			Location.X = FCString::Atof(*Tokens[2]); // Location.X = 2
			Location.Y = FCString::Atof(*Tokens[3]); // Location.Y = 4
			Location.Z = FCString::Atof(*Tokens[4]); // Location.Z = 6

			FMatrix Rotation = FMatrix::Identity;

			Rotation.M[0][0] = FCString::Atof(*Tokens[5]); // Rotation.M[0][0] = 1.0
			Rotation.M[0][1] = FCString::Atof(*Tokens[6]); // Rotation.M[0][1] = 0.0
			Rotation.M[0][2] = FCString::Atof(*Tokens[7]); // Rotation.M[0][2] = 0.0

			Rotation.M[1][0] = FCString::Atof(*Tokens[8]); // Rotation.M[1][0] = 0.0
			Rotation.M[1][1] = FCString::Atof(*Tokens[9]); // Rotation.M[1][1] = 1.0
			Rotation.M[1][2] = FCString::Atof(*Tokens[10]); // Rotation.M[1][2] = 0.0

			Rotation.M[2][0] = FCString::Atof(*Tokens[11]); // Rotation.M[2][0] = 0.0
			Rotation.M[2][1] = FCString::Atof(*Tokens[12]); // Rotation.M[2][1] = 0.0
			Rotation.M[2][2] = FCString::Atof(*Tokens[13]); // Rotation.M[2][2] = 1.0

			/*
			Given a part description of generic form: 1 <colour> x y z a b c d e f g h i <file>
			LDraw's Transform matrix looks like this:

				/ a b c x \
				| d e f y |
				| g h i z |
				\ 0 0 0 1 /

			The problem is that Unreal uses a different form of transformation matrix for it's matrix operations
			like multiplication which we need, to correctly resolve the model hierarchy into a standalone parts list.
			Given the same symbols as above the Unreal transformation matrix looks like:

				/ a d g 0 \
				| b e h 0 |
				| c f i 0 |
				\ x y z 1 /

			The reason behind the difference is not completely clear to me, but the LDraw Matrix apparently is the classic mathematic way of doing
			transformation matrices and Unreal uses it's own notation... It's not really important and we just have to transpose the LDraw matrix to
			get the Unreal matrix.
			*/

			FMatrix Transform = Rotation.GetTransposed();
			Transform.SetOrigin(Location);

			FString Name = Tokens[14]; // Name = "3005.dat"

			// In case the part description is actually a submodel we have to check if the submodel name has whitespaces and fix it. e.g. "brick", "1x1" -> "brick 1x1"
			if (Tokens.Num() > 14)
			{
				for (int32 i = 15; i < Tokens.Num(); i++)
				{
					Name.Append(TEXT(" ") + Tokens[i]);
				}
			}

			// Assemble the Part and attach it to the correct model or submodel
			FLDrawPart Part;
			Part.Color = Color;
			Part.Transform = Transform;
			Part.Name = Name;

			FLDrawModel* Model = OutModels.Find(CurrentModelName);

			Model->Parts.Add(Part);
		}
	}

	return true;
}
