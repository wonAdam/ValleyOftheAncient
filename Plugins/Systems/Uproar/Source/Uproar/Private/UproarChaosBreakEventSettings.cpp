// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarChaosBreakEventSettings.h"

bool UUproarChaosBreakEventSettings::GetBreakEventMagnitude(const float InMass, EUproarMagnitude& OutMagnitude)
{
	// See if settings have a mass override
	if (bOverrideMassEvaluation)
	{
		// Set OutMagnitude
		OutMagnitude = OverrideMagnitude;

		// Successfully retrieved an OutMagnitude
		return true;
	}

	// Check against classification 
	if (InMass > MinimumMassForTinyClassification)
	{
		OutMagnitude = EUproarMagnitude::TINY;

		if (InMass > MinimumMassForSmallClassification)
		{
			OutMagnitude = EUproarMagnitude::SMALL;

			if (InMass > MinimumMassForMediumClassification)
			{
				OutMagnitude = EUproarMagnitude::MEDIUM;

				if (InMass > MinimumMassForLargeClassification)
				{
					OutMagnitude = EUproarMagnitude::LARGE;

					if (InMass > MinimumMassForEpicClassification)
					{
						OutMagnitude = EUproarMagnitude::EPIC;
					}
				}
			}
		}

		// Mass evaluation succeeded to match a classification
		return true;
	}

	// No match was found, return false
	return false;
}

bool UUproarChaosBreakEventSettings::GetBreakEventSpeed(const float InSpeed, EUproarSpeed & OutSpeed)
{
	// See if settings have a speed override
	if (bOverrideVelocityEvaluation)
	{
		// Set OutSpeed
		OutSpeed = OverrideSpeed;

		// Successfully retrieved an OutSpeed
		return true;
	}

	// Check against classification 
	if (InSpeed > MinimumVelocityForSlowClassification)
	{
		OutSpeed = EUproarSpeed::SLOW;

		if (InSpeed > MinimumVelocityForMidSpeedClassification)
		{
			OutSpeed = EUproarSpeed::MID_SPEED;

			if (InSpeed > MinimumVelocityForQuickClassification)
			{
				OutSpeed = EUproarSpeed::QUICK;

				if (InSpeed > MinimumVelocityForEpicSpeedClassification)
				{
					OutSpeed = EUproarSpeed::EPIC;
				}
			}
		}

		// Velocity evaluation succeeded to match a classification
		return true;
	}

	// Velocity evaluation failed to match a classification
	return false;
}

TEnumAsByte<EPhysicalSurface> UUproarChaosBreakEventSettings::GetBreakEventSurfaceType(const TEnumAsByte<EPhysicalSurface> InSurfaceType)
{
	// If the settings override surface, return overridden surface, otherwise just feed back the input
	if (bOverrideSurfaceType)
	{
		return OverrideSurfaceType;
	}

	return InSurfaceType;
}

