// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AncientGameCameraModeView.generated.h"

/**
 * FAncientGameCameraModeView
 *
 *	View data produced by the camera mode that is used to blend camera modes.
 */
USTRUCT(BlueprintType)
struct FAncientGameCameraModeView
{
public:
	GENERATED_BODY()

	FAncientGameCameraModeView();

	void Blend(const FAncientGameCameraModeView& Other, float OtherWeight);

public:
	// The desired world-space pivot location of the camera view
	UPROPERTY(BlueprintReadWrite)
	FVector PivotLocation;

	// The desired world-space location of the camera view
	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	// The desired world-space rotation of the camera view
	UPROPERTY(BlueprintReadWrite)
	FRotator Rotation;

	// The desired world-space rotation to apply to the player controller (if one exists, associated with the target actor)
	UPROPERTY(BlueprintReadWrite)
	FRotator ControlRotation;

	// The desired field-of-view for the camera view
	UPROPERTY(BlueprintReadWrite)
	float FieldOfView = 80.f;
};
