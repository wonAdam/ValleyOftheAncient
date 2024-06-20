// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDroneControlsComponent.h"

#include "Delegates/Delegate.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HoverDroneMovementComponent.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "Math/Axis.h"
#include "Math/Matrix.inl"
#include "Math/RotationMatrix.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "UObject/ObjectPtr.h"

class UEnhancedInputComponent;

UHoverDroneControlsComponent::UHoverDroneControlsComponent()
{
	// Specific actions must be bound in the blueprint
}

void UHoverDroneControlsComponent::SetupPlayerControls_Implementation(UEnhancedInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	BindInputAction(MoveForwardInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveForward);
	BindInputAction(MoveRightInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveRight);
	BindInputAction(SlideUp_WorldInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveUp);
	BindInputAction(TurnInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleTurn);
	BindInputAction(LookUpInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookUp);
	BindInputAction(TurboInputAction, ETriggerEvent::Started, this, &ThisClass::HandleStartTurbo);
	BindInputAction(TurboInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleStopTurbo);
}

void UHoverDroneControlsComponent::HandleMoveForward(const FInputActionValue& InputActionValue)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	float Val = InputActionValue.Get<FInputActionValue::Axis1D>();

	if (MyPawn->Controller == nullptr || Val == 0.0f)
	{
		return;
	}

	UHoverDroneMovementComponent* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(MyPawn->GetMovementComponent());
	if (HoverMoveComponent && HoverMoveComponent->IsTurbo())
	{
		// in Turbo, max input Value
		Val = (Val > 0.f) ? 1.f : -1.f;
	}

	FRotator const ControlSpaceRot = MyPawn->Controller->GetControlRotation();

	FVector WorldDir = FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::X);
	WorldDir.Z = 0.f;		// constrain right/forward movement to XY plane
	if (WorldDir.IsZero() == false)
	{
		// normalize so sliding speed isn't pitch-dependent
		WorldDir.Normalize();

		// apply function to Val for finer analog control
		float MassagedVal = FMath::Square(Val);
		MassagedVal = (Val < 0) ? -MassagedVal : MassagedVal;		// ensure correct sign

		// transform to world space and add it
		MyPawn->AddMovementInput(WorldDir, MassagedVal);
	}
}

void UHoverDroneControlsComponent::HandleMoveRight(const FInputActionValue& InputActionValue)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	float Val = InputActionValue.Get<FInputActionValue::Axis1D>();

	if (MyPawn->Controller == nullptr || Val == 0.0f)
	{
		return;
	}

	else if (MyPawn->Controller)
	{
		FRotator const ControlSpaceRot = MyPawn->Controller->GetControlRotation();

		// transform to world space and add it
		FVector WorldDir = FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::Y);
		WorldDir.Z = 0.f;		// constrain right/forward movement to XY plane
		if (WorldDir.IsZero() == false)
		{
			// normalize so sliding speed isn't pitch-dependent
			WorldDir.Normalize();

			// apply function to Val for finer analog control
			float MassagedVal = FMath::Square(Val);
			MassagedVal = (Val < 0) ? -MassagedVal : MassagedVal;		// ensure correct sign

			// transform to world space and add it
			MyPawn->AddMovementInput(WorldDir, MassagedVal);
		}
	}
}

void UHoverDroneControlsComponent::HandleMoveUp(const FInputActionValue& InputActionValue)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	float Val = InputActionValue.Get<FInputActionValue::Axis1D>();
	// apply function to Val for finer analog control
	static float Exp = 3.f;
	float MassagedVal = FMath::Pow(FMath::Abs(Val), Exp);
	MassagedVal = (Val < 0) ? -MassagedVal : MassagedVal;		// ensure correct sign

	if (MassagedVal != 0.f)
	{
		MyPawn->AddMovementInput(FVector::UpVector, MassagedVal);
	}
}

void UHoverDroneControlsComponent::HandleTurn(const FInputActionValue& InputActionValue)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	MyPawn->AddControllerYawInput(InputActionValue.Get<FInputActionValue::Axis1D>());
}

void UHoverDroneControlsComponent::HandleLookUp(const struct FInputActionValue& InputActionValue)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	MyPawn->AddControllerPitchInput(InputActionValue.Get<FInputActionValue::Axis1D>());
}

void UHoverDroneControlsComponent::HandleStartTurbo(const struct FInputActionValue& /*InputActionValue*/)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	UHoverDroneMovementComponent* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(MyPawn->GetMovementComponent());
	if (HoverMoveComponent)
	{
		HoverMoveComponent->SetTurbo(true);
	}
}

void UHoverDroneControlsComponent::HandleStopTurbo(const struct FInputActionValue& /*InputActionValue*/)
{
	APawn* MyPawn = GetPawnChecked<APawn>();
	UHoverDroneMovementComponent* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(MyPawn->GetMovementComponent());
	if (HoverMoveComponent)
	{
		HoverMoveComponent->SetTurbo(false);
	}
}