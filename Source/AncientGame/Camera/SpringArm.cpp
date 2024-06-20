// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/SpringArm.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "Engine/World.h"
#include "HAL/PlatformCrt.h"
#include "Math/Matrix.inl"
#include "Math/Quat.h"
#include "Math/RotationMatrix.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector4.h"
#include "Misc/AssertionMacros.h"
#include "Stats/Stats2.h"

class AActor;

//////////////////////////////////////////////////////////////////////////
// FSpringArm

void FSpringArm::UpdateDesiredArmLocation(const UWorld* WorldContext, const TArray<const AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation, bool bDoTrace)
{
	FVector PivotLocation = InitialTransform.GetLocation();
	FRotator DesiredRot = InitialTransform.Rotator();

	// Recoverable error if the world is not provided, but an error should occur
	bool NeedsWorld = bDoTrace;
	ensureMsgf(!NeedsWorld || WorldContext != nullptr, TEXT("World is required for spring arm to trace against"));

	bDoTrace = bDoTrace && WorldContext != nullptr;
	
	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = PivotLocation;
	FVector DesiredLoc = ArmOrigin;
	
	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(OffsetLocation);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f) && WorldContext != nullptr)
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false);
		QueryParams.AddIgnoredActors(IgnoreActors);

		FHitResult Result;
		WorldContext->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);
		
		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit);

		if (ResultLoc == DesiredLoc) 
		{	
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	CameraTransform.SetLocation(ResultLoc);
	CameraTransform.SetRotation(DesiredRot.Quaternion());

	StateIsValid = true;
}

FVector FSpringArm::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething)
{
	return bHitSomething ? TraceHitLocation : DesiredArmLocation;
}

void FSpringArm::Initialize()
{
	bIsCameraFixed = false;
	StateIsValid = false;
}

void FSpringArm::Tick(const UWorld* WorldContext, const AActor* IgnoreActor, const FTransform& InitialTransform, const FVector OffsetLocation)
{
	TArray<const AActor*> IgnoreActorArray;
	IgnoreActorArray.Add(IgnoreActor);
	Tick(WorldContext, IgnoreActorArray, InitialTransform, OffsetLocation);
}

void FSpringArm::Tick(const UWorld* WorldContext, const TArray<const AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation)
{
	UpdateDesiredArmLocation(WorldContext, IgnoreActors, InitialTransform, OffsetLocation, bDoCollisionTest);
}

const FTransform& FSpringArm::GetCameraTransform() const
{
	ensure(StateIsValid);
	return CameraTransform;
}

FVector FSpringArm::GetUnfixedCameraPosition() const
{
	ensure(StateIsValid);
	return UnfixedCameraPosition;
}

bool FSpringArm::IsCollisionFixApplied() const
{
	return bIsCameraFixed;
}

void USpringArmBlueprintLibrary::InitializeSpringArm(FSpringArm& SpringArm)
{
	SpringArm.Initialize();
}

void USpringArmBlueprintLibrary::TickSpringArm(FSpringArm& SpringArm, const UWorld* WorldContext, const AActor* IgnoreActor, const FTransform& InitialTransform, const FVector OffsetLocation)
{
	SpringArm.Tick(WorldContext, IgnoreActor, InitialTransform, OffsetLocation);
}

FTransform USpringArmBlueprintLibrary::GetCameraTransform(const FSpringArm& SpringArm)
{
	return SpringArm.GetCameraTransform();
}

FVector USpringArmBlueprintLibrary::GetSpringArmUnfixedCameraPosition(const FSpringArm& SpringArm)
{
	return SpringArm.GetUnfixedCameraPosition();
}

bool USpringArmBlueprintLibrary::IsSpringArmCollisionFixApplied(const FSpringArm& SpringArm)
{
	return SpringArm.IsCollisionFixApplied();
}