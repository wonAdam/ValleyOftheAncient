// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/AncientGameCameraModeStack.h"

#include "Camera/AncientGameCameraMode.h"
#include "Camera/AncientGameCameraModeView.h"
#include "CoreTypes.h"
#include "HAL/PlatformCrt.h"
#include "Misc/AssertionMacros.h"

/* FAncientGameCameraModeStack
 *****************************************************************************/

void FAncientGameCameraModeStack::PushCameraMode(UAncientGameCameraMode* CameraModeInstance, AActor* TargetActor)
{
	if (!CameraModeInstance)
	{
		return;
	}

	int32 StackSize = CameraModeStack.Num();

	if ((StackSize > 0) && (CameraModeStack[0] == CameraModeInstance))
	{
		// Already top of stack.
		return;
	}

	// See if it's already in the stack and remove it.
	// Figure out how much it was contributing to the stack.
	int32 ExistingStackIndex = INDEX_NONE;
	float ExistingStackContribution = 1.0f;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		if (CameraModeStack[StackIndex] == CameraModeInstance)
		{
			ExistingStackIndex = StackIndex;
			ExistingStackContribution *= CameraModeInstance->GetBlendWeight();
			break;
		}
		else
		{
			ExistingStackContribution *= (1.0f - CameraModeStack[StackIndex]->GetBlendWeight());
		}
	}

	if (ExistingStackIndex != INDEX_NONE)
	{
		CameraModeStack.RemoveAt(ExistingStackIndex);
		StackSize--;
	}
	else
	{
		ExistingStackContribution = 0.0f;
	}

	// Decide what initial weight to start with.
	const bool bShouldBlend = ((CameraModeInstance->GetBlendTime() > 0.0f) && (StackSize > 0));
	const float BlendWeight = (bShouldBlend ? ExistingStackContribution : 1.0f);

	CameraModeInstance->SetBlendWeight(BlendWeight);

	// Add new entry to top of stack.
	CameraModeStack.Insert(CameraModeInstance, 0);

	// Make sure stack bottom is always weighted 100%.
	CameraModeStack.Last()->SetBlendWeight(1.0f);

	// Let the camera mode know if it's being added to the stack.
	if (ExistingStackIndex == INDEX_NONE)
	{
		CameraModeInstance->ActivateInternal(TargetActor);
	}
}

bool FAncientGameCameraModeStack::EvaluateStack(float DeltaTime, AActor* TargetActor, FAncientGameCameraModeView& OutCameraModeView)
{
	if (!UpdateStack(DeltaTime, TargetActor))
	{
		return false;
	}

	BlendStack(OutCameraModeView);
	return true;
}

bool FAncientGameCameraModeStack::UpdateStack(float DeltaTime, AActor* TargetActor)
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return false;
	}

	int32 RemoveCount = 0;
	int32 RemoveIndex = INDEX_NONE;

	bool bHasValidCameraMode = false;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		UAncientGameCameraMode* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		// The camera mode could request the current view on activation, which would trigger updating the camera mode before it is activated
		// If the camera mode has not been activated yet, skip it.
		if (CameraMode->bIsActivated)
		{
			bHasValidCameraMode = true;
			CameraMode->UpdateCameraMode(DeltaTime, TargetActor);

			if (CameraMode->GetBlendWeight() >= 1.0f)
			{
				// Everything below this mode is now irrelevant and can be removed.
				RemoveIndex = (StackIndex + 1);
				RemoveCount = (StackSize - RemoveIndex);
				break;
			}
		}
	}

	if (RemoveCount > 0)
	{
		// Let the camera modes know they being removed from the stack.
		for (int32 StackIndex = RemoveIndex; StackIndex < StackSize; ++StackIndex)
		{
			UAncientGameCameraMode* CameraMode = CameraModeStack[StackIndex];
			check(CameraMode);

			CameraMode->DeactivateInternal();
		}

		CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
	}
	return bHasValidCameraMode;
}

void FAncientGameCameraModeStack::BlendStack(FAncientGameCameraModeView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	// Start at the bottom and blend up the stack
	const UAncientGameCameraMode* CameraMode = CameraModeStack[StackSize - 1];
	check(CameraMode);

	OutCameraModeView = CameraMode->GetCameraModeView();

	for (int32 StackIndex = (StackSize - 2); StackIndex >= 0; --StackIndex)
	{
		CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		OutCameraModeView.Blend(CameraMode->GetCameraModeView(), CameraMode->GetBlendWeight());
	}
}