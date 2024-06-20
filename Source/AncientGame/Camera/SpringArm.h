// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/MathFwd.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

#include "SpringArm.generated.h"

class AActor;
class UObject;
class UWorld;
struct FFrame;

/**
 * This structure maintain location at a fixed from a pivot point, but but will retract if there is a collision, and spring back when there is no collision.
 *
 * Example: Use as a 'camera boom' to keep the follow camera for a player from colliding into the world.
 */

USTRUCT(Blueprintable)
struct ANCIENTGAME_API FSpringArm
{
	GENERATED_BODY()
public:
	/** Resets the dynamic state of the spring arm */
	void Initialize();

	/** Updates the spring arm using the supplied information**/
	void Tick(const UWorld* WorldContext, const AActor* IgnoreActor, const FTransform& InitialTransform, const FVector OffsetLocation);
	void Tick(const UWorld* WorldContext, const TArray<const AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation);

	/** Returns the current camera transform**/
	const FTransform& GetCameraTransform() const;

	/** Natural length of the spring arm when there are no collisions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float TargetArmLength = 300.0f;

	/** How big should the query probe sphere be (in unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	float ProbeSize = 12.0f;

	/** Collision channel of the query probe (defaults to ECC_Camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	TEnumAsByte<ECollisionChannel> ProbeChannel = ECC_Camera;

	/** If true, do a collision test using ProbeChannel and ProbeSize to prevent camera clipping into level.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision)
	bool bDoCollisionTest = true;

	/** Should we inherit pitch from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritPitch = true;

	/** Should we inherit yaw from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritYaw = true;

	/** Should we inherit roll from parent component. Does nothing if using Absolute Rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritRoll = true;

	/** Get the position where the camera should be without applying the Collision Test displacement */
	FVector GetUnfixedCameraPosition() const;

	/** Is the Collision Test displacement being applied? */
	bool IsCollisionFixApplied() const;

private:

	/** Updates the desired arm location, calling BlendLocations to do the actual blending if a trace is done */
	void UpdateDesiredArmLocation(const UWorld* WorldContext, const TArray<const AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation, bool bDoTrace);
	
	/**
	 * This function allows subclasses to blend the trace hit location with the desired arm location;
	 * by default it returns bHitSomething ? TraceHitLocation : DesiredArmLocation
	 */
	FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething);

	FTransform CameraTransform;

	/** Temporary variables when applying Collision Test displacement to notify if its being applied and by how much */
	bool bIsCameraFixed = false;
	bool StateIsValid = false;
	FVector UnfixedCameraPosition;
};

UCLASS(meta = (BlueprintThreadSafe, ScriptName = "SpringArmLibrary"))
class ANCIENTGAME_API USpringArmBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = SpringArm)
	static void InitializeSpringArm(UPARAM(ref) FSpringArm& SpringArm);

	/** Get the position where the camera should be without applying the Collision Test displacement */
	UFUNCTION(BlueprintCallable, Category = SpringArm)
	static void TickSpringArm(UPARAM(ref) FSpringArm& SpringArm, const UWorld* WorldContext, const AActor* IgnoreActor, const FTransform& InitialTransform, const FVector OffsetLocation);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = SpringArm)
	static FTransform GetCameraTransform(const FSpringArm& SpringArm);

	/** Get the position where the camera should be without applying the Collision Test displacement */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = SpringArmCollision)
	static FVector GetSpringArmUnfixedCameraPosition(const FSpringArm& SpringArm);

	/** Is the Collision Test displacement being applied? */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = SpringArmCollision)
	static bool IsSpringArmCollisionFixApplied(const FSpringArm& SpringArm);
};
