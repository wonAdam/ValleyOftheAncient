// Copyright Epic Games, Inc. All Rights Reserved.

#include "AncientGameCameraComponent.h"

#include "AncientGameCameraMode.h"
#include "Camera/AncientGameCameraModeStack.h"
#include "Camera/AncientGameCameraModeView.h"
#include "Camera/CameraTypes.h"
#include "Containers/EnumAsByte.h"
#include "Engine/Scene.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformCrt.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "UObject/UnrealNames.h"

/* FAncientGameCameraModeHandle
 *****************************************************************************/

namespace AncientGameCameraModeHandle_Impl
{
	static int32 LastHandleId = 0;
	static int32 GetNextQueuedHandleIdForUse() { return ++LastHandleId; }
}

bool FAncientGameCameraModeHandle::IsValid() const
{
	return Owner.IsValid() && HandleId != 0;
}

void FAncientGameCameraModeHandle::Reset()
{
	Owner.Reset();
	HandleId = 0;
}


/* UAncientGameCameraComponent
 *****************************************************************************/

UAncientGameCameraComponent::UAncientGameCameraComponent()
	: DefaultCameraMode(UAncientGameCameraMode::StaticClass())
{
	bWantsInitializeComponent = true;
}

void UAncientGameCameraComponent::InitializeComponent()
{
	Super::InitializeComponent();
	PushCameraMode(DefaultCameraMode);
}

void UAncientGameCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	AActor* TargetActor = GetOwner();

	FAncientGameCameraModeView CameraModeView;
	if (!BlendingStack.EvaluateStack(DeltaTime, TargetActor, CameraModeView))
	{
		Super::GetCameraView(DeltaTime, DesiredView);
		return;
	}

	// Keep camera component in sync with the latest view.
	FieldOfView = CameraModeView.FieldOfView;

	// Keep player controller in sync with the latest view.
	if (APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (APlayerController* PC = TargetPawn->GetController<APlayerController>())
		{
			PC->SetControlRotation(CameraModeView.ControlRotation);
		}
	}

	// Keep camera component in sync with the latest view.
	SetWorldLocationAndRotation(CameraModeView.Location, CameraModeView.Rotation);
	FieldOfView = CameraModeView.FieldOfView;

	// Fill in desired view.
	DesiredView.Location = CameraModeView.Location;
	DesiredView.Rotation = CameraModeView.Rotation;
	DesiredView.FOV = CameraModeView.FieldOfView;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;

	// See if the CameraActor wants to override the PostProcess settings used.
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}
}

FAncientGameCameraModeHandle UAncientGameCameraComponent::PushCameraMode(TSubclassOf<UAncientGameCameraMode> CameraModeClass, int32 Priority)
{
	return PushCameraModeUsingInstance(GetPooledCameraModeInstance(CameraModeClass), Priority);
}

FAncientGameCameraModeHandle UAncientGameCameraComponent::PushCameraModeUsingInstance(UAncientGameCameraMode* CameraModeInstance, int32 Priority)
{
	FAncientGameCameraModeHandle ModeHandle;
	ModeHandle.Owner = this;
	ModeHandle.HandleId = AncientGameCameraModeHandle_Impl::GetNextQueuedHandleIdForUse();

	int32 StackIndex = 0;
	for (; StackIndex < CameraModePriorityStack.Num(); ++StackIndex)
	{
		if (CameraModePriorityStack[StackIndex].Priority > Priority)
		{
			break;
		}
	}

	CameraModePriorityStack.Insert({ ModeHandle.HandleId, Priority, CameraModeInstance }, StackIndex);
	UpdateBlendingStack();

	return ModeHandle;
}

bool UAncientGameCameraComponent::PullCameraMode(FAncientGameCameraModeHandle& ModeHandle)
{
	bool bSuccess = false;
	if (ModeHandle.IsValid() && ModeHandle.Owner == this)
	{
		const int32 HandleId = ModeHandle.HandleId;
		const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate([HandleId](const FCameraModeStackEntry& CameraModeEntry)
			{
				return (CameraModeEntry.HandleId == HandleId);
			});
		bSuccess = PullCameraModeAtIndex(FoundIndex);
		ModeHandle.Reset();
	}
	return bSuccess;
}

bool UAncientGameCameraComponent::PullCameraModeInstance(UAncientGameCameraMode* CameraMode)
{
	const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate([CameraMode](const FCameraModeStackEntry& CameraModeEntry)
		{
			return (CameraModeEntry.CameraMode == CameraMode);
		});
	return PullCameraModeAtIndex(FoundIndex);
}

UAncientGameCameraMode* UAncientGameCameraComponent::GetActiveCameraMode() const
{
	UAncientGameCameraMode* ActiveCamera = nullptr;
	if (ensureAlways(CameraModePriorityStack.Num() > 0))
	{
		ActiveCamera = CameraModePriorityStack.Top().CameraMode;
	}
	return ActiveCamera;
}

bool UAncientGameCameraComponent::IsValid(const FAncientGameCameraModeHandle& ModeHandle)
{
	return ModeHandle.IsValid();
}

FAncientGameCameraModeHandle UAncientGameCameraComponent::PushCameraModeForActor(const AActor* Actor, TSubclassOf<UAncientGameCameraMode> CameraModeType, int32 Priority)
{
	FAncientGameCameraModeHandle ModeHandle;

	if (Actor)
	{
		if (UAncientGameCameraComponent* CamComponent = Actor->FindComponentByClass<UAncientGameCameraComponent>())
		{
			ModeHandle = CamComponent->PushCameraMode(CameraModeType, Priority);
		}
	}
	return ModeHandle;
}

FAncientGameCameraModeHandle UAncientGameCameraComponent::PushCameraModeForActorUsingInstance(const AActor* Actor, UAncientGameCameraMode* CameraModeInstance, int32 Priority)
{
	FAncientGameCameraModeHandle ModeHandle;

	if (Actor)
	{
		if (UAncientGameCameraComponent* CamComponent = Actor->FindComponentByClass<UAncientGameCameraComponent>())
		{
			ModeHandle = CamComponent->PushCameraModeUsingInstance(CameraModeInstance, Priority);
		}
	}
	return ModeHandle;
}

bool UAncientGameCameraComponent::PullCameraModeByHandle(FAncientGameCameraModeHandle& ModeHandle)
{
	if (ModeHandle.IsValid())
	{
		return ModeHandle.Owner->PullCameraMode(ModeHandle);
	}
	return false;
}

bool UAncientGameCameraComponent::PullCameraModeInstanceFromActor(const AActor* Actor, UAncientGameCameraMode* CameraMode)
{
	if (UAncientGameCameraComponent* CamComponent = Actor->FindComponentByClass<UAncientGameCameraComponent>())
	{
		return CamComponent->PullCameraModeInstance(CameraMode);
	}
	return false;
}

UAncientGameCameraMode* UAncientGameCameraComponent::GetActiveCameraModeForActor(const AActor* Actor)
{
	UAncientGameCameraMode* ActiveCamera = nullptr;
	if (Actor)
	{
		if (UAncientGameCameraComponent* CamComponent = Actor->FindComponentByClass<UAncientGameCameraComponent>())
		{
			ActiveCamera = CamComponent->GetActiveCameraMode();
		}
	}
	return ActiveCamera;
}

void UAncientGameCameraComponent::UpdateBlendingStack()
{
	if (!ensureAlways(CameraModePriorityStack.Num() > 0))
	{
		PushCameraMode(DefaultCameraMode);
	}

	AActor* TargetActor = GetOwner();
	BlendingStack.PushCameraMode(CameraModePriorityStack.Top().CameraMode, TargetActor);
}

UAncientGameCameraMode* UAncientGameCameraComponent::GetPooledCameraModeInstance(TSubclassOf<UAncientGameCameraMode> CameraModeClass)
{
	check(CameraModeClass);

	// First see if we already created one.
	for (UAncientGameCameraMode* CameraMode : CameraModeInstancePool)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}

	// Not found, so we need to create it.
	UAncientGameCameraMode* NewCameraMode = NewObject<UAncientGameCameraMode>(this, CameraModeClass, NAME_None, RF_NoFlags);
	check(NewCameraMode);

	CameraModeInstancePool.Add(NewCameraMode);

	return NewCameraMode;
}

bool UAncientGameCameraComponent::PullCameraModeAtIndex(int32 Index)
{
	if (CameraModePriorityStack.IsValidIndex(Index))
	{
		CameraModePriorityStack.RemoveAt(Index);
		UpdateBlendingStack();

		return true;
	}
	return false;
}