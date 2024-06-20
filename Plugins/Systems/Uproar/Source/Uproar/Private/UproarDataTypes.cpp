// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarDataTypes.h"

#include "HAL/Platform.h"

int32 UproarFunctionLibrary::GenerateUproarSoundDefinitionKey(
	const TEnumAsByte<EPhysicalSurface> SurfaceType
	, const EUproarPhysicsEventType EventType
	, const EUproarMagnitude Magnitude
	, const EUproarSpeed Speed
)
{
	constexpr int32 SurfaceTypeDomain = EPhysicalSurface::SurfaceType_Max;
	constexpr int32 EventTypeDomain = SurfaceTypeDomain * (int32)EUproarPhysicsEventType::EType_MAX;
	constexpr int32 MagnitudeDomain = EventTypeDomain * (int32)EUproarMagnitude::EType_MAX;

	/** 
	* This function collapses multi dimensional coordinates into a 1 dimensional space. For each input is given its own domain space
	* by using previous max values as spacers. For a simplified example, let's use three inputs (A, B, and C) and a domain size 
	* of 10 possible values for each input type using the following 3 dimensional variant formula:
	* 
	* Key = A + B * Max_A + C * Max_A * Max_B
	* 
	* If the input is A = 0, B = 0, C = 0; then we can calculate the hash key as 000
	* If the input is A = 5, B = 0, C = 0; then we can calculate the hash key as 005
	* If the input is A = 5, B = 3, C = 7; then we can calculate the hash key as 735
	* 
	* In this way, each domain size creates a new value range on our 1D number line, always resulting in a unique ID.
	*/

	return (int32)SurfaceType +
		((int32)EventType	* SurfaceTypeDomain) 
		+ ((int32)Magnitude	* EventTypeDomain) 
		+ ((int32)Speed		* MagnitudeDomain) 
		;
}