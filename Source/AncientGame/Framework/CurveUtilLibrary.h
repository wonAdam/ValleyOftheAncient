// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectGlobals.h"

#include "CurveUtilLibrary.generated.h"

class UObject;
struct FFrame;
struct FRuntimeFloatCurve;

UCLASS()
class UCurveUtilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Curve")
	static float EvaluateCurve(UPARAM(ref) const FRuntimeFloatCurve& RuntimeCurve, float InTime);

	UFUNCTION(BlueprintPure, Category = "Curve")
	static float FInterpCubic(float A, float B, float Alpha);
};
