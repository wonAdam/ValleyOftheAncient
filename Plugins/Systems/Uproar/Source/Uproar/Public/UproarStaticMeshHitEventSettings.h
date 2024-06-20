// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "Math/Vector2D.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UproarDataTypes.h"

#include "UproarStaticMeshHitEventSettings.generated.h"

struct FPropertyChangedEvent;

/**
 * 
 */
UCLASS(BlueprintType)
class UPROAR_API UUproarStaticMeshHitEventSettings : public UObject
{
	GENERATED_BODY()
public:
	UUproarStaticMeshHitEventSettings();

#if WITH_EDITOR
	// ~Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// ~End UObject Interface
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagnitudeClassification")
	EUproarMagnitude StaticMeshMassClassification = EUproarMagnitude::SMALL;

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
	EUproarMagnitude& GetCollisionEventMagnitude();

	bool GetCollisionEventSpeed(const float InSpeed, EUproarSpeed& OutSpeed, float& OutVolumeMod);

	TEnumAsByte<EPhysicalSurface> GetCollisionEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType);

private:

	// Cache thresholds for faster looping on threshold checks
	TArray<float> MinimumThresholds;

	// Cache modulation range values for faster lookup
	TArray<FVector2D> ModulationSpeedRanges;

};

