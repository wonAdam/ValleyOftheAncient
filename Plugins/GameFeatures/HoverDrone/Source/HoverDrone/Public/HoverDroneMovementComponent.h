// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineBaseTypes.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Math/MathFwd.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "UObject/UObjectGlobals.h"

#include "HoverDroneMovementComponent.generated.h"

class UObject;
struct FHitResult;

UCLASS()
class UHoverDroneMovementComponent : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:
	UHoverDroneMovementComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

	//~ Begin CharacterMovementComponent interface
	virtual void OnTeleported() override;
	//~ End CharacterMovementComponent interface

	void AddRotationInput(FRotator RotInput);

	/** Returns height above the ground. */
	float GetAltitude() const { return CurrentAltitude; };

	/** Turbo controls */
	void SetTurbo(bool bNewTurbo) { bTurbo = bNewTurbo; };
	bool IsTurbo() const { return bTurbo; };

	const FVector& GetLastControlAcceleration() const {return LastControlAcceleration;}

protected:

	//~ Begin UFloatingPawnMovement interface
	virtual void ApplyControlInputToVelocity(float DeltaTime) override;
	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
	//~ End UFloatingPawnMovement interface

	/** Rotational acceleration when turning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Normal Speed")
	float RotAcceleration = 150.f;

	/** Rotational deceleration when not turning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Normal Speed")
	float RotDeceleration = 150.f;

	/** Maximum rotational speed, pitch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Normal Speed")
	float MaxPitchRotSpeed = 70.f;

	/** Maximum rotational speed, yaw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Normal Speed")
	float MaxYawRotSpeed = 110.f;

	/** Rotational acceleration when turning, while in Turbo mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	float TurboRotAcceleration = 30.f;

	/** Rotational deceleration when not turning, while in Turbo mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	float TurboRotDeceleration = 75.f;

	/** Maximum rotational speed, pitch, while in Turbo mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
		float TurboMaxPitchRotSpeed = 35.f;

	/** Maximum rotational speed, yaw, while in Turbo mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	float TurboMaxYawRotSpeed = 30.f;

	/** Range of valid accelerations (mapped to TurboAccelerationAltitudeRange) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	FVector2D TurboAccelerationRange;

	/** Used to map Altitude to Acceleration (using TurboAccelerationRange) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	FVector2D TurboAccelerationAltitudeRange;

	/** Linear deceleration while in Turbo mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverDroneMovement | Turbo")
	float TurboDeceleration = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDroneMovement)
	bool RestrictOutOfBoundsWithCollision = true;

	/** Mag is between 0 and 1.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDroneMovement)
	FVector LastControlAcceleration;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDroneMovement)
	FRotator RotationInput;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HoverDroneMovement)
	FRotator RotVelocity;
	
private:

	void ApplyControlInputToRotation(float DeltaTime);
	float MeasureAltitude(FVector Location, float TestHeight, bool& bHitFoundOut, FVector& HitPositionOut) const;
	FRotator GetMaxRotationSpeed() const;
	void RestrictDroneInput();

	/** Mag is between 0 and 1.0 */
	// FVector LastControlAcceleration;
	// FRotator RotationInput;
	// FRotator RotVelocity;

	/** Max timestep to simulate in one go. Frames longer than this will do multiple simulations. */
	const float MaxSimulationTimestep = 1.f / 60.f;

	/** Valid Pitch range */
	const float MinPitch = -88.f;
	const float MaxPitch = 88.f;

	float CurrentAltitude = 0.0f;

	const float MaxAltitude = 100000.0f;
	const float MinAltitude = 0.0f;
	const float AltitudeTraceLength = MaxAltitude * 1.5f;
	bool bTurbo = false;

	const float OffEdgeDistanceAllowance = 1000.0f;
	FVector LastGroundPosition;
	bool bHasGround = true;
	bool bLastGroundPositionValid = false;
};



