// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarChaosCollisionEventSettings.h"

#include "Math/UnrealMathSSE.h"

struct FPropertyChangedEvent;

UUproarChaosCollisionEventSettings::UUproarChaosCollisionEventSettings()
	: MinimumMassForTinyClassification(0.01f)
	, MinimumMassForSmallClassification(0.1f)
	, MinimumMassForMediumClassification(10.0f)
	, MinimumMassForLargeClassification(100.0f)
	, MinimumMassForEpicClassification(1000.0f)
	, MinimumVelocityForSlowClassification(0.01f)
	, MinimumVelocityForMidSpeedClassification(10.0f)
	, MinimumVelocityForQuickClassification(100.0f)
	, MinimumVelocityForEpicSpeedClassification(1000.0f)
	, bOverrideSurfaceType(false)
	, OverrideSurfaceType(EPhysicalSurface::SurfaceType_Default)
	, bOverrideMassEvaluation(false)
	, OverrideMagnitude(EUproarMagnitude::TINY)
	, MinimumValidMass(0.01f)
	, bOverrideVelocityEvaluation(false)
	, OverrideSpeed(EUproarSpeed::SLOW)
	, MinimumValidVelocity(0.01f)
	, bEnableSpeedModulationOfVolume(false)
	, VolumeRangeForSlow(0.1f, 0.3f)
	, VolumeRangeForMidSpeed(0.3f, 0.6f)
	, VolumeRangeForQuick(0.6f, 0.8f)
	, VolumeRangeForEpic(0.8f, 1.0f)
{
	MinimumVelocityThresholds.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	MinimumVelocityThresholds[(int32)EUproarSpeed::SLOW] = MinimumVelocityForSlowClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::MID_SPEED] = MinimumVelocityForMidSpeedClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::QUICK] = MinimumVelocityForQuickClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::EPIC] = MinimumVelocityForEpicSpeedClassification;

	MinimumMassThresholds.SetNumZeroed((int32)EUproarMagnitude::EType_MAX);
	MinimumMassThresholds[(int32)EUproarMagnitude::TINY] = MinimumMassForTinyClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::SMALL] = MinimumMassForSmallClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::MEDIUM] = MinimumMassForMediumClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::LARGE] = MinimumMassForLargeClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::EPIC] = MinimumMassForEpicClassification;

	ModulationSpeedRanges.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	ModulationSpeedRanges[(int32)EUproarSpeed::SLOW] = VolumeRangeForSlow;
	ModulationSpeedRanges[(int32)EUproarSpeed::MID_SPEED] = VolumeRangeForMidSpeed;
	ModulationSpeedRanges[(int32)EUproarSpeed::QUICK] = VolumeRangeForQuick;
	ModulationSpeedRanges[(int32)EUproarSpeed::EPIC] = VolumeRangeForEpic;
}

#if WITH_EDITOR
void UUproarChaosCollisionEventSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	MinimumVelocityThresholds.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	MinimumVelocityThresholds[(int32)EUproarSpeed::SLOW] = MinimumVelocityForSlowClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::MID_SPEED] = MinimumVelocityForMidSpeedClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::QUICK] = MinimumVelocityForQuickClassification;
	MinimumVelocityThresholds[(int32)EUproarSpeed::EPIC] = MinimumVelocityForEpicSpeedClassification;

	MinimumMassThresholds.SetNumZeroed((int32)EUproarMagnitude::EType_MAX);
	MinimumMassThresholds[(int32)EUproarMagnitude::TINY] = MinimumMassForTinyClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::SMALL] = MinimumMassForSmallClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::MEDIUM] = MinimumMassForMediumClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::LARGE] = MinimumMassForLargeClassification;
	MinimumMassThresholds[(int32)EUproarMagnitude::EPIC] = MinimumMassForEpicClassification;

	ModulationSpeedRanges.SetNumZeroed((int32)EUproarSpeed::EType_MAX);
	ModulationSpeedRanges[(int32)EUproarSpeed::SLOW] = VolumeRangeForSlow;
	ModulationSpeedRanges[(int32)EUproarSpeed::MID_SPEED] = VolumeRangeForMidSpeed;
	ModulationSpeedRanges[(int32)EUproarSpeed::QUICK] = VolumeRangeForQuick;
	ModulationSpeedRanges[(int32)EUproarSpeed::EPIC] = VolumeRangeForEpic;

}
#endif

bool UUproarChaosCollisionEventSettings::GetCollisionEventMagnitude(const float InMass, EUproarMagnitude& OutMagnitude)
{
	// Set initial state
	bool bMagnitudeFound = false;

	// See if settings have a mass override
	if (bOverrideMassEvaluation && InMass > MinimumValidMass)
	{
		// Set OutMagnitude
		OutMagnitude = OverrideMagnitude;

		// Successfully retrieved an OutMagnitude
		bMagnitudeFound = true;
	}

	// Check against classification 
	if (bOverrideMassEvaluation == false && InMass > MinimumMassForTinyClassification)
	{
		for (int32 i = 0; i < (int32)EUproarMagnitude::EType_MAX; ++i)
		{
			if (InMass > MinimumMassThresholds[i])
			{
				// Update OutMagnitude
				OutMagnitude = (EUproarMagnitude)i;

				// We've hit our max, return true
				bMagnitudeFound = true;
			}
			else
			{
				break;
			}
		}
	}

	// No match was found, return false
	return bMagnitudeFound;
}

bool UUproarChaosCollisionEventSettings::GetCollisionEventSpeed(const float InSpeed, EUproarSpeed& OutSpeed, float& OutVolumeMod)
{
	// Set initial state
	bool bSpeedFound = false;

	// Set volume mod to base level
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
			if (InSpeed > MinimumVelocityThresholds[i])
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

TEnumAsByte<EPhysicalSurface> UUproarChaosCollisionEventSettings::GetCollisionEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType)
{
	// If the settings override surface, return overridden surface, otherwise just feed back the input
	if (bOverrideSurfaceType)
	{
		return OverrideSurfaceType;
	}

	return InSurfaceType;
}


