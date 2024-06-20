// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"

#include "AncientGameCameraModeStack.generated.h"

class AActor;
class UAncientGameCameraMode;
struct FAncientGameCameraModeView;

/**
 * FAncientGameCameraModeStack
 *
 *	Stack used for blending camera modes.
 */
USTRUCT()
struct FAncientGameCameraModeStack
{
	GENERATED_BODY()

public:
	void PushCameraMode(UAncientGameCameraMode* CameraModeInstance, AActor* TargetActor);

	bool EvaluateStack(float DeltaTime, AActor* TargetActor, FAncientGameCameraModeView& OutCameraModeView);

protected:
	bool UpdateStack(float DeltaTime, AActor* TargetActor);
	void BlendStack(FAncientGameCameraModeView& OutCameraModeView) const;

protected:
	UPROPERTY()
	TArray<UAncientGameCameraMode*> CameraModeStack;
};
