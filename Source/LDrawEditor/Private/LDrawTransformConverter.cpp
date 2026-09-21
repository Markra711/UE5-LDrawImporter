// Fill out your copyright notice in the Description page of Project Settings.


#include "LDrawTransformConverter.h"
#include "Kismet/KismetMathLibrary.h"

FTransform LDrawTransformConverter::Convert(const FMatrix& Transform)
{
    float X = -Transform.M[2][3];
    float Y = -Transform.M[0][3];
    float Z = -Transform.M[1][3];
    FVector PartLocation = FVector(X, Y, Z);

    FVector Forward(Transform.M[0][0], Transform.M[0][1], Transform.M[0][2]);
    FVector Right(Transform.M[1][0], Transform.M[1][1], Transform.M[1][2]);
    FVector Up(Transform.M[2][0], Transform.M[2][1], Transform.M[2][2]);

    FVector ForwardCopy(Up.X, Forward.X, Right.X);
    FVector RightCopy(Up.Y, Forward.Y, Right.Y);
    FVector UpCopy(Up.Z, Forward.Z, Right.Z);

    FRotator Rotation = UKismetMathLibrary::MakeRotationFromAxes(UpCopy, ForwardCopy, RightCopy);
    FRotator Corrector = UKismetMathLibrary::MakeRotationFromAxes(FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1));

    FRotator PartRotation = FRotator(FQuat(Rotation) * FQuat(Corrector));

    FVector PartScale = FVector(1.f, 1.f, 1.f);

    return FTransform(PartRotation, PartLocation, PartScale);
}
