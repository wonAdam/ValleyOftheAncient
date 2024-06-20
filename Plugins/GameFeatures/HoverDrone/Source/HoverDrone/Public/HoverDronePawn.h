// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Rotator.h"
#include "ModularPawn.h"
#include "UObject/UObjectGlobals.h"

#include "HoverDronePawn.generated.h"

class UCurveVector;
class UObject;
class UPawnMovementComponent;
class USphereComponent;
struct FFrame;
struct FMinimalViewInfo;

UCLASS()
class AHoverDronePawn : public AModularPawn
{
	GENERATED_BODY()

public:
	AHoverDronePawn(const FObjectInitializer& ObjectInitializer);

	//~ Begin APawn interface
	virtual FRotator GetViewRotation() const override;
	virtual void PawnClientRestart() override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;
	//~ End APawn interface

	//~ Begin AActor interface
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	//~ End AActor interface

	/** Returns drone's current height above the ground. */
	UFUNCTION(BlueprintCallable, Category = HoverDrone)
	float GetAltitude() const;

	void OnZoomInput(float Val);

	void UpdateAutoFocus(FMinimalViewInfo& OutPOV, float DeltaTime);
	void UpdateSceneFringe(FMinimalViewInfo& OutPOV);
protected:

	/** Controls how far to tilt based on acceleration. Higher is more tilt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float MaxAccelToGravRatio = 0.05f;

	/** How quickly/aggressively to interp into the tilted position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float DroneTiltInterpSpeed_Accel = 1.f;

	/** How quickly/aggressively to interp back to neutral when decelerating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float DroneTiltInterpSpeed_Decel = 2.f;

	/** How fast to zoom, in FOV degrees per sec. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float ZoomRate = 30.f;

	/** How fast to interpolate FOV (for smoothness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float FOVInterpSpeed = 7.f;

	/** Max FOV for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float MaxFOV = 160.f;

	/** Min FOV for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDrone)
	float MinFOV = 5.f;

	/** True to turn on depth of field and auto focusing, false otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AutoFocus)
	bool bAutoFocus = true;

	/** Controls how quickly the focal distance converges on the depth at the center of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AutoFocus)
	float AutoFocusInterpSpeed = 12.f;

	/** When autofocus is on, 1/Fstop for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AutoFocus)
	float CameraApertureFStop = 4.f;

	/** Curve that controls screen fringe intensity as a function of fov. X is intensity, Y is saturation*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ScreenFringe)
	UCurveVector* ScreenFringeFOVCurve;

private:

	/** HoverDronePawn collision component */
	UPROPERTY(Category = Pawn, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** HoverDronePawn movement component */
	UPROPERTY(Category = Pawn, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UPawnMovementComponent* MovementComponent;

	void AddRotationInput(const FRotator Input);

	/** For interpolating the tilt. */
	FRotator LastTiltedDroneRot;

	UPROPERTY(EditDefaultsOnly, Category = HoverDrone)
	float FieldOfView = 90.0f;

	float GoalFOV = 90.0f;		// for interpolating
	float AutoFocusDistance = 1000.f;



};



