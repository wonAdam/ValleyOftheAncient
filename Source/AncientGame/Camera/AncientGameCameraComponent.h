// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AncientGameCameraModeStack.h"
#include "Camera/CameraComponent.h"
#include "Containers/Array.h"
#include "HAL/Platform.h"
#include "Templates/SubclassOf.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "AncientGameCameraComponent.generated.h"

class AActor;
class UAncientGameCameraComponent;
class UAncientGameCameraMode;
class UObject;
struct FFrame;
struct FMinimalViewInfo;

USTRUCT(BlueprintType)
struct FAncientGameCameraModeHandle
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	void Reset();

private:
	friend class UAncientGameCameraComponent;

	UPROPERTY()
	TWeakObjectPtr<UAncientGameCameraComponent> Owner;

	UPROPERTY()
	int32 HandleId = 0;
};

USTRUCT()
struct FCameraModeStackEntry
{
	GENERATED_BODY()

	int32 HandleId = 0;
	int32 Priority = 0;

	UPROPERTY()
	UAncientGameCameraMode* CameraMode = nullptr;
};

UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class ANCIENTGAME_API UAncientGameCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "AncientGame|Camera")
	TSubclassOf<UAncientGameCameraMode> DefaultCameraMode;

public:
	UAncientGameCameraComponent();

	//~ Begin UActorComponent interface
	virtual void InitializeComponent() override;
	//~ End UActorComponent interface

	//~ Begin UCameraComponent interface
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
	//~ End UCameraComponent interface

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	FAncientGameCameraModeHandle PushCameraMode(TSubclassOf<UAncientGameCameraMode> CameraModeType, int32 Priority = 0);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	FAncientGameCameraModeHandle PushCameraModeUsingInstance(UAncientGameCameraMode* CameraModeInstance, int32 Priority = 0);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	bool PullCameraMode(UPARAM(ref) FAncientGameCameraModeHandle& ModeHandle);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	bool PullCameraModeInstance(UAncientGameCameraMode* CameraMode);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	UAncientGameCameraMode* GetActiveCameraMode() const;
	
	
	UFUNCTION(BlueprintPure, Category = "AncientGame|Camera")
	static bool IsValid(const FAncientGameCameraModeHandle& ModeHandle);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	static FAncientGameCameraModeHandle PushCameraModeForActor(const AActor* Actor, TSubclassOf<UAncientGameCameraMode> CameraModeType, int32 Priority = 0);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	static FAncientGameCameraModeHandle PushCameraModeForActorUsingInstance(const AActor* Actor, UAncientGameCameraMode* CameraModeInstance, int32 Priority = 0);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	static bool PullCameraModeByHandle(UPARAM(ref) FAncientGameCameraModeHandle& ModeHandle);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	static bool PullCameraModeInstanceFromActor(const AActor* Actor, UAncientGameCameraMode* CameraMode);

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Camera")
	static UAncientGameCameraMode* GetActiveCameraModeForActor(const AActor* Actor);

protected:
	void UpdateBlendingStack();
	UAncientGameCameraMode* GetPooledCameraModeInstance(TSubclassOf<UAncientGameCameraMode> CameraModeClass);

	bool PullCameraModeAtIndex(int32 Index);

	// Stack of active camera modes, sorted in priority order.
	UPROPERTY()
	TArray<FCameraModeStackEntry> CameraModePriorityStack;

	// Pool of unique camera modes for reuse (to cut down on re-allocating the same mode over and over).
	UPROPERTY()
	TArray<UAncientGameCameraMode*> CameraModeInstancePool;

	// Stack used to blend the camera modes.
	UPROPERTY()
	FAncientGameCameraModeStack BlendingStack;
};
