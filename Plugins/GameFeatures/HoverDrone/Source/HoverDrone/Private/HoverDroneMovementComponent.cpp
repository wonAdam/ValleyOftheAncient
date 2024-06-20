// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDroneMovementComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathSSE.h"
#include "Templates/Casts.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UnrealNames.h"

UHoverDroneMovementComponent::UHoverDroneMovementComponent(const FObjectInitializer& ObjectInitializer)	: 
	Super(ObjectInitializer),
	TurboAccelerationRange(8000.f, 22000.f),
	TurboAccelerationAltitudeRange(0.f, 25000.f)
{
	// UFloatingPawnMovement members
	MaxSpeed = 30000.f;
	Acceleration = 5000.f;
	Deceleration = 5000.f;
}

void UHoverDroneMovementComponent::ApplyControlInputToVelocity(float DeltaTime)
{
	RestrictDroneInput();
	
	FVector ControlAcceleration = GetPendingInputVector().GetClampedToMaxSize(1.f);
	
	// basic Z thrust exactly counteracts gravity
	float ZThrust = -GetGravityZ();

	float CurrentAccel = 0.f;
	float CurrentDecel = 0.f;
	if (bTurbo)
	{
		CurrentAccel = FMath::GetMappedRangeValueClamped(TurboAccelerationAltitudeRange, TurboAccelerationRange, CurrentAltitude);
		CurrentDecel = TurboDeceleration;
	}
	else
	{
		CurrentAccel = Acceleration;
		CurrentDecel = Deceleration;
	}

	// Apply various accelerations
	static float InvCoeffAirFriction = 3000.f;
	float const AirFrictionScalar = (InvCoeffAirFriction != 0) ? (Velocity.Size() / InvCoeffAirFriction) : 1.f;
	FVector const AntiVelocityDir = -(Velocity.GetSafeNormal());
	Velocity += AntiVelocityDir * CurrentDecel * DeltaTime * AirFrictionScalar;
	Velocity += ControlAcceleration * FMath::Abs(CurrentAccel) * DeltaTime;
	Velocity += FVector(0, 0, GetGravityZ() + ZThrust) * DeltaTime;

	ConsumeInputVector();

	LastControlAcceleration = ControlAcceleration;
}

bool UHoverDroneMovementComponent::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation)
{
	if (CurrentAltitude >= MinAltitude)
	{
		return Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);
	}

	FHitResult HitTmp(1.f);
	MoveUpdatedComponent(FVector(0.0f, 0.0f, -CurrentAltitude + 1.0f), NewRotation, false, &HitTmp, ETeleportType::None);
	return true;
}

// note: only dealing with yaw for now, since that's what we care about
void UHoverDroneMovementComponent::ApplyControlInputToRotation(float DeltaTime)
{
	static auto GetVFOV = [](float ViewportAspectRatio, float HFOVDegrees)
	{
		float HFOVRads = FMath::DegreesToRadians(HFOVDegrees);
		float InverseAspectRatio = 1.0f / ViewportAspectRatio;
		float VFOVRads = 2.0f * FMath::Atan(FMath::Tan(HFOVRads * 0.5f) * InverseAspectRatio);
		return FMath::RadiansToDegrees(VFOVRads);
	};

	// adjust rot accel and clamps for zoom
	static float AssumedDefaultHFOV = 90.f;		// the fov we tweaked for
	static float AssumedDefaultVFOV = GetVFOV(16.0f / 9.0f, AssumedDefaultHFOV);
	static FVector AssumedDefaultFOV = FRotator(AssumedDefaultVFOV, AssumedDefaultHFOV, 1.0f).Euler();

	const APlayerController* const PC = PawnOwner ? Cast<APlayerController>(PawnOwner->GetController()) : nullptr;

	if (PC == nullptr)
	{
		return;
	}

	int ViewportWidth, ViewportHeight;
	PC->GetViewportSize(ViewportWidth, ViewportHeight);
	const float AspectRatio = (float)ViewportWidth / (float)ViewportHeight;

	const float CurrentHFOV = (PC && PC->PlayerCameraManager) ? PC->PlayerCameraManager->GetFOVAngle() : AssumedDefaultHFOV;
	const float CurrentVFOV = GetVFOV(AspectRatio, CurrentHFOV);

	const FVector FOV = FRotator(CurrentVFOV, CurrentHFOV, 0.0f).Euler();
	const FVector AdjScalar = FOV / AssumedDefaultFOV;
	FVector RotVelocityVec(RotVelocity.Euler());
	
	if (RotationInput.IsZero())
	{
		// Decelerate towards zero!
		const float CurrentRotDeacceleration = bTurbo ? TurboRotDeceleration : RotDeceleration;
		const FVector AdjustedRotDecel = AdjScalar * CurrentRotDeacceleration;
		const FVector VelocityDeltaVec = AdjustedRotDecel * -RotVelocityVec.GetSignVector() * DeltaTime;
		const FVector VelocityMin = FVector::Min(FVector::ZeroVector, RotVelocityVec);
		const FVector VelocityMax = FVector::Max(FVector::ZeroVector, RotVelocityVec);
		RotVelocityVec = ClampVector(RotVelocityVec + VelocityDeltaVec, VelocityMin, VelocityMax);
	}
	else
	{
		// updating rotation to avoid overshooting badly on long frames!
		// don't let the delta take us out of bounds.
		// note that if we're already out of bounds, we'll stay there
		const FVector InputVec(RotationInput.Euler());
		const FRotator RotSpeed = GetMaxRotationSpeed();
		const FVector AdjustedMaxRotSpeed = RotSpeed.Euler() * AdjScalar;
		const float CurrentRotAcceleration = bTurbo ? TurboRotAcceleration : RotAcceleration;
		const FVector AdjustedRotAccel = AdjScalar * CurrentRotAcceleration;

		const FVector MaxVelMag = FVector::Min(FVector::OneVector, InputVec.GetAbs()) * AdjustedMaxRotSpeed;
		const FVector MaxDeltaVel = FVector::Max(FVector::ZeroVector, MaxVelMag - RotVelocityVec);
		const FVector MinDeltaVel = FVector::Min(FVector::ZeroVector, -(RotVelocityVec + MaxVelMag));
		const FVector DeltaVel = InputVec * AdjustedRotAccel * DeltaTime;
		RotVelocityVec += ClampVector(DeltaVel, MinDeltaVel, MaxDeltaVel);
	}

	RotVelocity = FRotator::MakeFromEuler(RotVelocityVec);
}

void UHoverDroneMovementComponent::AddRotationInput(FRotator NewRotInput)
{
	RotationInput += NewRotInput;
}

void UHoverDroneMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AController* Controller = PawnOwner->GetController();

	if (PawnOwner == nullptr || UpdatedComponent == nullptr || Controller == nullptr || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	float UnsimulatedTime = DeltaTime;
	float StepTime = MaxSimulationTimestep > 0.f ? MaxSimulationTimestep : DeltaTime;

	while (UnsimulatedTime > KINDA_SMALL_NUMBER)
	{
		// simulate!
		float SimTime = FMath::Min(UnsimulatedTime, StepTime);

		if (Controller->IsLocalPlayerController())
		{
			ApplyControlInputToRotation(SimTime);
		}

		FRotator RotDelta = RotVelocity * SimTime;

		// enforce pitch limits
		
		FRotator::FReal const CurrentPitch = UpdatedComponent->GetComponentRotation().Pitch;
		FRotator::FReal const MinDeltaPitch = MinPitch - CurrentPitch;
		FRotator::FReal const MaxDeltaPitch = MaxPitch - CurrentPitch;
		FRotator::FReal const OldPitch = RotDelta.Pitch;
		RotDelta.Pitch = FMath::Clamp(RotDelta.Pitch, MinDeltaPitch, MaxDeltaPitch);
		if (OldPitch != RotDelta.Pitch)
		{
			// if we got clamped, zero the pitch velocity
			RotVelocity.Pitch = FRotator::FReal(0.f);
		}

		if (!RotDelta.IsNearlyZero())
		{
			FRotator const NewRot = UpdatedComponent->GetComponentRotation() + RotDelta;

			FHitResult Hit(1.f);
			SafeMoveUpdatedComponent(FVector::ZeroVector, NewRot, false, Hit);
		}

		UnsimulatedTime -= SimTime;
	}

	if (Controller && Controller->IsLocalPlayerController())
	{
		Controller->SetControlRotation(UpdatedComponent->GetComponentRotation());
	}

	// cache altitude
	CurrentAltitude = MeasureAltitude(PawnOwner->GetActorLocation(), AltitudeTraceLength, bHasGround, LastGroundPosition);
	if (!bHasGround)
	{
		// Test upwards as well if the ground was not found
		CurrentAltitude = MeasureAltitude(PawnOwner->GetActorLocation() + FVector(0.0f, 0.0f, MaxAltitude), AltitudeTraceLength, bHasGround, LastGroundPosition) - MaxAltitude;
	}

	bLastGroundPositionValid = bLastGroundPositionValid || bHasGround;

	// clear out any input, we've handled it (or ignored it)
	RotationInput = FRotator::ZeroRotator;
}

void UHoverDroneMovementComponent::OnTeleported()
{
	Super::OnTeleported();

	if (PawnOwner)
	{
		CurrentAltitude = MeasureAltitude(PawnOwner->GetActorLocation(), AltitudeTraceLength, bHasGround, LastGroundPosition);
		bLastGroundPositionValid = bHasGround;
	}
}

float UHoverDroneMovementComponent::MeasureAltitude(FVector Location, float TestHeight, bool& bHitFoundOut, FVector& HitPositionOut) const
{
	FCollisionQueryParams TraceParams(NAME_None, FCollisionQueryParams::GetUnknownStatId(), true, PawnOwner);
	FHitResult Hit;

	FVector const TraceStart = Location;
	FVector const TraceEnd = TraceStart - FVector::UpVector * TestHeight;
	bHitFoundOut = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
	if (bHitFoundOut)
	{
		HitPositionOut = Hit.ImpactPoint;
		return (Hit.ImpactPoint - TraceStart).Size();
	}

	return Location.Z;
}

FRotator UHoverDroneMovementComponent::GetMaxRotationSpeed() const
{
	return bTurbo ? FRotator(TurboMaxPitchRotSpeed, TurboMaxYawRotSpeed, 0.0f) : FRotator(MaxPitchRotSpeed, MaxYawRotSpeed, 0.0f);
}

void UHoverDroneMovementComponent::RestrictDroneInput()
{
	FVector NewAccelInput(FVector::ZeroVector);
	bool UseNewAccelInput = false;
	FVector CurrentInputVector = PawnOwner->GetPendingMovementInputVector();

	if (RestrictOutOfBoundsWithCollision && !bHasGround && bLastGroundPositionValid)
	{	
		FVector OutOfBoundsInputVector = (LastGroundPosition - PawnOwner->GetActorLocation());
		float OutOfBoundsDistance = OutOfBoundsInputVector.Size2D();
		OutOfBoundsInputVector = OutOfBoundsInputVector.GetClampedToMaxSize(1.f);

		float Alpha = OutOfBoundsDistance / OffEdgeDistanceAllowance;
		NewAccelInput = FMath::Lerp(CurrentInputVector, OutOfBoundsInputVector, Alpha);
		NewAccelInput.Z = CurrentInputVector.Z;
		UseNewAccelInput = true;
	}

	if (CurrentAltitude < MinAltitude)
	{
		NewAccelInput.Z = 1.0f;
		UseNewAccelInput = true;
	}
	else if (CurrentAltitude > MaxAltitude)
	{
		NewAccelInput.Z = -1.0f;
		UseNewAccelInput = true;
	}

	if (UseNewAccelInput)
	{
		ConsumeInputVector();
		AddInputVector(NewAccelInput, true);
	}
}
