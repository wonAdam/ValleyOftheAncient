// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
#include "Containers/EnumAsByte.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UproarDataTypes.h"

#include "UproarChaosBreakEventSettings.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class UPROAR_API UUproarChaosBreakEventSettings : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassifications")
	float MinimumMassForTinyClassification = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassifications")
	float MinimumMassForSmallClassification = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassifications")
	float MinimumMassForMediumClassification = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassifications")
	float MinimumMassForLargeClassification = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassifications")
	float MinimumMassForEpicClassification = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedClassifications")
	float MinimumVelocityForSlowClassification = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedClassifications")
	float MinimumVelocityForMidSpeedClassification = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedClassifications")
	float MinimumVelocityForQuickClassification = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpeedClassifications")
	float MinimumVelocityForEpicSpeedClassification = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides")
	bool bOverrideSurfaceType = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideSurfaceType, ScriptName = "OverrideSurfaceTypeValue"))
	TEnumAsByte<EPhysicalSurface> OverrideSurfaceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides")
	bool bOverrideMassEvaluation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideMassEvaluation))
	EUproarMagnitude OverrideMagnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides")
	bool bOverrideVelocityEvaluation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideVelocityEvaluation))
	EUproarSpeed OverrideSpeed;

public:
	bool GetBreakEventMagnitude(const float InMass, EUproarMagnitude& OutMagnitude);

	bool GetBreakEventSpeed(const float InSpeed, EUproarSpeed& OutSpeed);

	TEnumAsByte<EPhysicalSurface> GetBreakEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType);
};
