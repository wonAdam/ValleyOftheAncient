// Copyright Epic Games, Inc. All Rights Reserved.
#include "Camera/SpringArmCameraMode.h"

#include "Camera/Interpolators.h"
#include "Containers/Array.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformCrt.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "UObject/Object.h"

USpringArmCameraMode::USpringArmCameraMode()
{
	SpringArm.bDoCollisionTest = true;
}

void USpringArmCameraMode::OnActivation(AActor* TargetActor)
{
	Super::OnActivation(TargetActor);
	SpringArm.Initialize();
	LocationSpringInterpolator.Reset();
	RotationSpringInterpolator.Reset();
}

FAncientGameCameraModeView USpringArmCameraMode::UpdateView_Implementation(float DeltaTime, AActor* TargetActor)
{
	UObject* WorldContext = this;
	TArray<const AActor*> AllIgnoreActors;

	if (TargetActor)
	{
		WorldContext = TargetActor;

		// Skip this actor and any attached actors when testing for collision
		AllIgnoreActors.Add(TargetActor);
		TargetActor->ForEachAttachedActors([&AllIgnoreActors](AActor* Actor) { AllIgnoreActors.AddUnique(Actor); return true; });
	}

	FAncientGameCameraModeView NewView = Super::UpdateView_Implementation(DeltaTime, TargetActor);

	// update springs
	FVector GoalCameraLoc = NewView.PivotLocation + PivotOffset;
	FVector SmoothedLocation = LocationSpringInterpolator.Eval(GoalCameraLoc, DeltaTime);
	FRotator GoalCameraRot = NewView.Rotation;
	FRotator SmoothedRotation = RotationSpringInterpolator.Eval(GoalCameraRot, DeltaTime);

	// Update the spring arm using the Pivot and offset provided
	FTransform CameraTransform(SmoothedRotation, SmoothedLocation);
	SpringArm.Tick(WorldContext->GetWorld(), AllIgnoreActors, CameraTransform, Offset);

	CameraTransform = SpringArm.GetCameraTransform();
	NewView.Location = CameraTransform.GetLocation();
	NewView.Rotation = CameraTransform.Rotator();

	return NewView;
}
