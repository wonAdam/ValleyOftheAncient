// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "Math/Vector2D.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UproarDataTypes.h"

#include "UproarChaosCollisionEventSettings.generated.h"

struct FPropertyChangedEvent;



/**
 * 
 */
UCLASS(BlueprintType)
class UPROAR_API UUproarChaosCollisionEventSettings : public UObject
{
	GENERATED_BODY()
public:
	UUproarChaosCollisionEventSettings();

#if WITH_EDITOR
	// ~Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// ~End UObject Interface
#endif

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
	TEnumAsByte<EPhysicalSurface> OverrideSurfaceType = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides")
	bool bOverrideMassEvaluation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideMassEvaluation))
	EUproarMagnitude OverrideMagnitude = EUproarMagnitude::TINY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideMassEvaluation))
	float MinimumValidMass = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides")
	bool bOverrideVelocityEvaluation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideVelocityEvaluation))
	EUproarSpeed OverrideSpeed = EUproarSpeed::SLOW;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = bOverrideVelocityEvaluation))
	float MinimumValidVelocity = 0.01f;

	// When enabled, Volume will be modulated based on the incoming speed values regardless of override values
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Modulation Overrides"))
	bool bEnableSpeedModulationOfVolume = false;

	// Random Volume Range when Slow
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Modulation Overrides", EditCondition = "bEnableSpeedModulationOfVolume"))
	FVector2D VolumeRangeForSlow = FVector2D(0.1f, 0.3f);

	// Random Volume Range when MidSpeed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Modulation Overrides", EditCondition = "bEnableSpeedModulationOfVolume"))
	FVector2D VolumeRangeForMidSpeed = FVector2D(0.3f, 0.6f);

	// Random Volume Range when Quick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Modulation Overrides", EditCondition = "bEnableSpeedModulationOfVolume"))
	FVector2D VolumeRangeForQuick = FVector2D(0.6f, 0.8f);

	// Random Volume Range when Epic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Modulation Overrides", EditCondition = "bEnableSpeedModulationOfVolume"))
	FVector2D VolumeRangeForEpic = FVector2D(0.8f, 1.0f);


public:
	bool GetCollisionEventMagnitude(const float InMass, EUproarMagnitude& OutMagnitude);

	bool GetCollisionEventSpeed(const float InSpeed, EUproarSpeed& OutSpeed, float& OutVolumeMod);

	TEnumAsByte<EPhysicalSurface> GetCollisionEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType);

private:

	// Cache thresholds for faster looping on threshold checks
	TArray<float> MinimumVelocityThresholds;
	TArray<float> MinimumMassThresholds;

	// Cache modulation ranges
	TArray<FVector2D> ModulationSpeedRanges;

};

