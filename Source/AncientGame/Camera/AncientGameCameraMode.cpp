// Copyright Epic Games, Inc. All Rights Reserved.

#include "AncientGameCameraMode.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Math/Vector.h"
#include "Math/VectorRegister.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "UObject/Class.h"

/* FAncientGameCameraModeView
 *****************************************************************************/

FAncientGameCameraModeView::FAncientGameCameraModeView()
	: PivotLocation(ForceInit)
	, Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(80.f)
{
}

void FAncientGameCameraModeView::Blend(const FAncientGameCameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.0f)
	{
		return;
	}
	else if (OtherWeight >= 1.0f)
	{
		*this = Other;
		return;
	}

	// Rotate the direction from the camera location to the pivot.  
	FRotator OffsetRotation = (Location - PivotLocation).GetSafeNormal().Rotation();
	FRotator OtherOffsetRotation = (Other.Location - Other.PivotLocation).GetSafeNormal().Rotation();
	FRotator DiffRotation = FMath::Lerp(OffsetRotation, OtherOffsetRotation, OtherWeight);

	// Calculate the distance the camera should be, using the Location and the Pivot location
	// and blend those together
	float Distance = FVector::Distance(Location, PivotLocation);
	float OtherDistance = FVector::Distance(Other.Location, Other.PivotLocation);
	float NewDistance = FMath::Lerp(Distance, OtherDistance, OtherWeight);

	PivotLocation = FMath::Lerp(PivotLocation, Other.PivotLocation, OtherWeight);

	// Use the blended pivot location, blended rotated offset, and the blended distance to calculate the new location
	Location = PivotLocation + DiffRotation.Vector() * NewDistance;

	const FRotator DeltaRotation = (Other.Rotation - Rotation).GetNormalized();
	Rotation = Rotation + (OtherWeight * DeltaRotation);

	const FRotator DeltaControlRotation = (Other.ControlRotation - ControlRotation).GetNormalized();
	ControlRotation = ControlRotation + (OtherWeight * DeltaControlRotation);

	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
}


/* UAncientGameCameraMode
 *****************************************************************************/

void UAncientGameCameraMode::OnActivation(AActor* TargetActor)
{
	ReceiveActivation(TargetActor);
}

void UAncientGameCameraMode::OnDeactivation()
{
	ReceiveDeactivation();
}

void UAncientGameCameraMode::UpdateCameraMode(float DeltaTime, AActor* TargetActor)
{
	View = UpdateView(DeltaTime, TargetActor);
	UpdateBlending(DeltaTime);
}

void UAncientGameCameraMode::SetBlendWeight(float Weight)
{
	BlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);

	// Since we're setting the blend weight directly, we need to calculate the blend alpha to account for the blend function.
	const float InvExponent = (BlendExponent > 0.0f) ? (1.0f / BlendExponent) : 1.0f;

	switch (BlendFunction)
	{
	case EAncientGameCameraModeBlendFunction::Linear:
		BlendAlpha = BlendWeight;
		break;

	case EAncientGameCameraModeBlendFunction::EaseIn:
		BlendAlpha = FMath::InterpEaseIn(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	case EAncientGameCameraModeBlendFunction::EaseOut:
		BlendAlpha = FMath::InterpEaseOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	case EAncientGameCameraModeBlendFunction::EaseInOut:
		BlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendWeight, InvExponent);
		break;

	case EAncientGameCameraModeBlendFunction::Cubic:
		BlendAlpha = FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, BlendWeight);
		break;

	default:
		checkf(false, TEXT("SetBlendWeight: Invalid BlendFunction [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

FAncientGameCameraModeView UAncientGameCameraMode::UpdateView_Implementation(float DeltaTime, AActor* TargetActor)
{
	FAncientGameCameraModeView NewView;
	NewView.Location = NewView.PivotLocation = GetPivotLocation(TargetActor);
	NewView.Rotation = GetPivotRotation(TargetActor);
	NewView.Rotation.Pitch = FMath::ClampAngle(NewView.Rotation.Pitch, ViewPitchMin, ViewPitchMax);
	NewView.FieldOfView = FieldOfView;

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		NewView.ControlRotation = TargetPawn->GetControlRotation();
	}
	else
	{
		NewView.ControlRotation = NewView.Rotation;
	}

	return NewView;
}

FVector UAncientGameCameraMode::GetPivotLocation_Implementation(AActor* TargetActor) const
{
	FVector ViewLocation(ForceInit);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		// Height adjustments for characters to account for crouching.
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetPawn))
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			check(TargetCharacterCDO);

			const UCapsuleComponent* CapsuleComp = TargetCharacter->GetCapsuleComponent();
			check(CapsuleComp);

			const UCapsuleComponent* CapsuleCompCDO = TargetCharacterCDO->GetCapsuleComponent();
			check(CapsuleCompCDO);

			const float DefaultHalfHeight = CapsuleCompCDO->GetUnscaledCapsuleHalfHeight();
			const float ActualHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
			const float HeightAdjustment = (DefaultHalfHeight - ActualHalfHeight) + TargetCharacterCDO->BaseEyeHeight;

			ViewLocation = TargetCharacter->GetActorLocation() + (FVector::UpVector * HeightAdjustment);
		}
		else
		{
			ViewLocation = TargetPawn->GetPawnViewLocation();
		}
	}
	else if (TargetActor)
	{
		ViewLocation = TargetActor->GetActorLocation();
	}

	return ViewLocation;
}

FRotator UAncientGameCameraMode::GetPivotRotation_Implementation(AActor* TargetActor) const
{
	FRotator ViewRotation(ForceInit);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		ViewRotation = TargetPawn->GetViewRotation();
	}
	else if (TargetActor)
	{
		ViewRotation = TargetActor->GetActorRotation();
	}

	ViewRotation.Pitch = FMath::ClampAngle(ViewRotation.Pitch, ViewPitchMin, ViewPitchMax);
	return ViewRotation;
}

void UAncientGameCameraMode::UpdateBlending(float DeltaTime)
{
	if (BlendTime > 0.0f)
	{
		BlendAlpha += (DeltaTime / BlendTime);
		BlendAlpha = FMath::Min(BlendAlpha, 1.0f);
	}
	else
	{
		BlendAlpha = 1.0f;
	}

	const float Exponent = (BlendExponent > 0.0f) ? BlendExponent : 1.0f;

	switch (BlendFunction)
	{
	case EAncientGameCameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha;
		break;

	case EAncientGameCameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EAncientGameCameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EAncientGameCameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EAncientGameCameraModeBlendFunction::Cubic:
		BlendWeight = FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, BlendAlpha);
		break;

	default:
		checkf(false, TEXT("UpdateBlending: Invalid BlendFunction [%d]\n"), (uint8)BlendFunction);
		break;
	}
}


void UAncientGameCameraMode::ActivateInternal(AActor* TargetActor)
{
	OnActivation(TargetActor);
	bIsActivated = true;
}

void UAncientGameCameraMode::DeactivateInternal()
{
	OnDeactivation();
	bIsActivated = false;
}
