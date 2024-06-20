// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarStaticMeshHitEventSettings.h"

#include "Math/UnrealMathSSE.h"

struct FPropertyChangedEvent;

UUproarStaticMeshHitEventSettings::UUproarStaticMeshHitEventSettings()
	: StaticMeshMassClassification(EUproarMagnitude::SMALL)
	, MinimumVelocityForSlowClassification(0.01f)
	, MinimumVelocityForMidSpeedClassification(10.0f)
	, MinimumVelocityForQuickClassification(100.0f)
	, MinimumVelocityForEpicSpeedClassification(1000.0f)
	, bOverrideSurfaceType(false)
	, OverrideSurfaceType(EPhysicalSurface::SurfaceType_Default)
	, bOverrideVelocityEvaluation(false)
	, OverrideSpeed(EUproarSpeed::SLOW)
	, MinimumValidVelocity(0.01f)
	, bEnableSpeedModulationOfVolume(false)
	, VolumeRangeForSlow(0.1f, 0.3f)
	, VolumeRangeForMidSpeed(0.3f, 0.6f)
	, VolumeRangeForQuick(0.6f, 0.8f)
	, VolumeRangeForEpic(0.8f, 1.0f)
{
	MinimumThresholds.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	MinimumThresholds[(int32)EUproarSpeed::SLOW] = MinimumVelocityForSlowClassification;
	MinimumThresholds[(int32)EUproarSpeed::MID_SPEED] = MinimumVelocityForMidSpeedClassification;
	MinimumThresholds[(int32)EUproarSpeed::QUICK] = MinimumVelocityForQuickClassification;
	MinimumThresholds[(int32)EUproarSpeed::EPIC] = MinimumVelocityForEpicSpeedClassification;

	ModulationSpeedRanges.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	ModulationSpeedRanges[(int32)EUproarSpeed::SLOW] = VolumeRangeForSlow;
	ModulationSpeedRanges[(int32)EUproarSpeed::MID_SPEED] = VolumeRangeForMidSpeed;
	ModulationSpeedRanges[(int32)EUproarSpeed::QUICK] = VolumeRangeForQuick;
	ModulationSpeedRanges[(int32)EUproarSpeed::EPIC] = VolumeRangeForEpic;

}

#if WITH_EDITOR
void UUproarStaticMeshHitEventSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	MinimumThresholds.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	MinimumThresholds[(int32)EUproarSpeed::SLOW] = MinimumVelocityForSlowClassification;
	MinimumThresholds[(int32)EUproarSpeed::MID_SPEED] = MinimumVelocityForMidSpeedClassification;
	MinimumThresholds[(int32)EUproarSpeed::QUICK] = MinimumVelocityForQuickClassification;
	MinimumThresholds[(int32)EUproarSpeed::EPIC] = MinimumVelocityForEpicSpeedClassification;

	ModulationSpeedRanges.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	ModulationSpeedRanges[(int32)EUproarSpeed::SLOW] = VolumeRangeForSlow;
	ModulationSpeedRanges[(int32)EUproarSpeed::MID_SPEED] = VolumeRangeForMidSpeed;
	ModulationSpeedRanges[(int32)EUproarSpeed::QUICK] = VolumeRangeForQuick;
	ModulationSpeedRanges[(int32)EUproarSpeed::EPIC] = VolumeRangeForEpic;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

EUproarMagnitude& UUproarStaticMeshHitEventSettings::GetCollisionEventMagnitude()
{
	// Get OutMagnitude
	return StaticMeshMassClassification;
}

bool UUproarStaticMeshHitEventSettings::GetCollisionEventSpeed(const float InSpeed, EUproarSpeed& OutSpeed, float& OutVolumeMod)
{
	// Set initial state
	bool bSpeedFound = false;

	// Set volume mod to base value
	OutVolumeMod = 1.0f;

	// See if settings have a speed override
	if (bOverrideVelocityEvaluation && InSpeed > MinimumValidVelocity)
	{
		// Set OutSpeed
		OutSpeed = OverrideSpeed;

		// Set Volume Mod
		OutVolumeMod = bEnableSpeedModulationOfVolume ? FMath::FRandRange(ModulationSpeedRanges[(int32)OutSpeed].X, ModulationSpeedRanges[(int32)OutSpeed].Y) : 1.0f;

		// Successfully retrieved an OutSpeed
		bSpeedFound = true;
	}
	
	// Check against classification thresholds
	if (bOverrideVelocityEvaluation == false && InSpeed > MinimumVelocityForSlowClassification)
	{
		for (int32 i = 0; i < (int32)EUproarSpeed::EType_MAX; ++i)
		{
			if (InSpeed > MinimumThresholds[i])
			{
				// Update OutSpeed
				OutSpeed = (EUproarSpeed)i;

				// Set Volume Mod
				OutVolumeMod = bEnableSpeedModulationOfVolume ? FMath::FRandRange(ModulationSpeedRanges[(int32)OutSpeed].X, ModulationSpeedRanges[(int32)OutSpeed].Y) : 1.0f;

				// We've hit our max, return true
				bSpeedFound = true;
			}
			else
			{
				break;
			}
		}
	}

	// Velocity evaluation failed to match a classification
	return bSpeedFound;
}

TEnumAsByte<EPhysicalSurface> UUproarStaticMeshHitEventSettings::GetCollisionEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType)
{
	// If surface type is overridden in the settings, return that, otherwise pass through
	if (bOverrideSurfaceType)
	{
		return OverrideSurfaceType;
	}

	return InSurfaceType;
}


