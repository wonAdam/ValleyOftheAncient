// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDronePawn.h"

#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Containers/EnumAsByte.h"
#include "Curves/CurveVector.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HoverDroneMovementComponent.h"
#include "Math/Axis.h"
#include "Math/Matrix.h"
#include "Math/RotationMatrix.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Stats/Stats2.h"
#include "Templates/Casts.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"


AHoverDronePawn::AHoverDronePawn(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	LastTiltedDroneRot(ForceInitToZero)
{
	SetCanBeDamaged(false);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(FName(TEXT("CollisionComponent")));
	CollisionComponent->InitSphereRadius(35.0f);
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	CollisionComponent->SetShouldUpdatePhysicsVolume(true);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->bDynamicObstacle = true;

	RootComponent = CollisionComponent;

	MovementComponent = CreateDefaultSubobject<UHoverDroneMovementComponent>(FName(TEXT("MovementComponent")));
	MovementComponent->UpdatedComponent = CollisionComponent;
}

FRotator AHoverDronePawn::GetViewRotation() const
{
	// pawn rotation dictates camera rotation
	float const Pitch = GetActorRotation().Pitch;
	float const Yaw = GetActorRotation().Yaw;
	return FRotator(Pitch, Yaw, 0.f);
}

void AHoverDronePawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	// start with first person
	GetActorEyesViewPoint(OutResult.Location, OutResult.Rotation);

	// now apply a tilt to simulate motion
	{
		UHoverDroneMovementComponent const* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(GetMovementComponent());
		if (HoverMoveComponent)
		{
			FMatrix const OldCamToWorld = FRotationMatrix(OutResult.Rotation);
			FMatrix const UntiltedDroneToWorld = FRotationMatrix::MakeFromZX(FVector::UpVector, OldCamToWorld.GetUnitAxis(EAxis::X));

			FMatrix const OldCamToDrone = OldCamToWorld * UntiltedDroneToWorld.Inverse();

			FVector const TiltedUpVector = (HoverMoveComponent->GetLastControlAcceleration() * MaxAccelToGravRatio + FVector::UpVector).GetSafeNormal();
			FMatrix TiltedDroneToWorld = FRotationMatrix::MakeFromZX(TiltedUpVector, UntiltedDroneToWorld.GetUnitAxis(EAxis::X));

			// interpolate drone tilt to smooth it out
			// only interpolating pitch and roll though!
			FRotator GoalTiltedDroneRot = TiltedDroneToWorld.Rotator();
			GoalTiltedDroneRot.Yaw = 0.f;
			float const InterpSpeed = GoalTiltedDroneRot.IsZero() ? DroneTiltInterpSpeed_Decel : DroneTiltInterpSpeed_Accel;
			FRotator InterpedTiltedDroneRot = FMath::RInterpTo(LastTiltedDroneRot, GoalTiltedDroneRot, DeltaTime, InterpSpeed);
			LastTiltedDroneRot = InterpedTiltedDroneRot;
			InterpedTiltedDroneRot.Yaw = OutResult.Rotation.Yaw; // keep original Yaw
			TiltedDroneToWorld = FRotationMatrix(InterpedTiltedDroneRot);

			FMatrix const NewCamToWorld = OldCamToDrone * TiltedDroneToWorld;

			OutResult.Rotation = NewCamToWorld.Rotator();
		}
	}

	// and also apply FOV settings for zoom
	FieldOfView = FMath::FInterpTo(FieldOfView, GoalFOV, DeltaTime, FOVInterpSpeed);
	FieldOfView = FMath::Clamp(FieldOfView, MinFOV, MaxFOV);
	OutResult.FOV = FieldOfView;

	UpdateAutoFocus(OutResult, DeltaTime);

	// Update fringe based on FOV
	UpdateSceneFringe(OutResult);
}

void AHoverDronePawn::AddControllerYawInput(float Val)
{
	Super::AddControllerYawInput(Val);
	AddRotationInput(FRotator(0, Val, 0));
}

void AHoverDronePawn::AddControllerPitchInput(float Val)
{
	Super::AddControllerPitchInput(Val);
	AddRotationInput(FRotator(-Val, 0, 0));
}

void AHoverDronePawn::AddRotationInput(const FRotator Input)
{
	if (!Controller || Controller->IsLookInputIgnored() || Input.IsZero())
	{
		return;
	}

	UHoverDroneMovementComponent* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(GetMovementComponent());
	if (HoverMoveComponent)
	{
		HoverMoveComponent->AddRotationInput(Input);
	}
}

void AHoverDronePawn::OnZoomInput(float Val)
{
	// adjust goal fov.  interpolation in CalcCamera will make actual value chase this value.
	GoalFOV -= ZoomRate * GetWorld()->GetDeltaSeconds() * Val;
	GoalFOV = FMath::Clamp(GoalFOV, MinFOV, MaxFOV);
}

void AHoverDronePawn::UpdateAutoFocus(FMinimalViewInfo& OutPOV, float DeltaTime)
{
	if (bAutoFocus)
	{
		// trace to get depth at center of screen
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(UpdateAutoFocus), true, this);
		FHitResult Hit;

		static float TraceDist = 1000000.f;
		FVector const TraceStart = OutPOV.Location;
		FVector const TraceEnd = TraceStart + OutPOV.Rotation.Vector() * TraceDist;
		bool const bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
		float MeasuredDistance = bHit ? (TraceStart - Hit.ImpactPoint).Size() : TraceDist;

		// make sure it's sane
		static float MinFocusDistance = 40.f;
		MeasuredDistance = FMath::Max(MeasuredDistance, MinFocusDistance);

		AutoFocusDistance = FMath::FInterpTo(AutoFocusDistance, MeasuredDistance, DeltaTime, AutoFocusInterpSpeed);

		OutPOV.PostProcessBlendWeight = 1.f;
		OutPOV.PostProcessSettings.bOverride_DepthOfFieldFstop = true;
		OutPOV.PostProcessSettings.DepthOfFieldFstop = CameraApertureFStop;

		OutPOV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
		OutPOV.PostProcessSettings.DepthOfFieldFocalDistance = AutoFocusDistance;
	}
	else
	{
		OutPOV.PostProcessSettings.bOverride_DepthOfFieldFstop = false;
		OutPOV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = false;
	}
}

void AHoverDronePawn::UpdateSceneFringe(FMinimalViewInfo& OutPOV)
{
	if (ScreenFringeFOVCurve != nullptr)
	{
		// Evaluate curve to get intensity and saturation values for current FOV
		const FVector FringeParams = ScreenFringeFOVCurve->GetVectorValue(FieldOfView);

		OutPOV.PostProcessSettings.bOverride_SceneFringeIntensity = true;
		OutPOV.PostProcessSettings.SceneFringeIntensity = FMath::Clamp(FringeParams.X, 0.f, 5.f);
	}
	else
	{
		OutPOV.PostProcessSettings.bOverride_SceneFringeIntensity = false;
	}
}

void AHoverDronePawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	// copy camera settings for a smooth transition
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC != nullptr && PC->PlayerCameraManager != nullptr)
	{
		FieldOfView = GetDefault<AHoverDronePawn>(GetClass())->FieldOfView;
		GoalFOV = FieldOfView;
	}
}

float AHoverDronePawn::GetAltitude() const
{
	UHoverDroneMovementComponent const* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(GetMovementComponent());
	if (HoverMoveComponent)
	{
		float Altitude = HoverMoveComponent->GetAltitude();
		if(Altitude > 0.0f)
		{
			return Altitude;
		}
	}

	return 0.f;
}