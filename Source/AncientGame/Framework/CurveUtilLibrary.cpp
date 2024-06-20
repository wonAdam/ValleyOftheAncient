// Copyright Epic Games, Inc. All Rights Reserved.

#include "CurveUtilLibrary.h"

#include "Curves/CurveFloat.h"
#include "Curves/RichCurve.h"
#include "Math/UnrealMathUtility.h"

float UCurveUtilLibrary::EvaluateCurve(const FRuntimeFloatCurve& RuntimeCurve, float InTime)
{
	return RuntimeCurve.GetRichCurveConst()->Eval(InTime);
}

float UCurveUtilLibrary::FInterpCubic(float A, float B, float Alpha)
{
	return FMath::CubicInterp(A, 0.f, B, 0.f, Alpha);
}