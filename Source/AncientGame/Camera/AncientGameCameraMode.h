// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/AncientGameCameraModeView.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"

#include "AncientGameCameraMode.generated.h"

class AActor;
struct FFrame;

/**
 * EAncientGameCameraModeBlendFunction
 *
 *	Blend function used for transitioning between camera modes.
 */
UENUM(BlueprintType)
enum class EAncientGameCameraModeBlendFunction : uint8
{
	// Does a simple linear interpolation.
	Linear,

	// Immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by the exponent.
	EaseIn,

	// Smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by the exponent.
	EaseOut,

	// Smoothly accelerates and decelerates.  Ease amount controlled by the exponent.
	EaseInOut,

	Cubic,

	COUNT	UMETA(Hidden)
};

/**
 * UAncientGameCameraMode
 *
 *	Base class for all camera modes.
 */
UCLASS(Blueprintable)
class ANCIENTGAME_API UAncientGameCameraMode : public UObject
{
	GENERATED_BODY()

protected:
	// The horizontal field of view (in degrees).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView = 80.f;

	// Minimum view pitch (in degrees).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin = -89.f;

	// Maximum view pitch (in degrees).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax = 89.f;

	// How long it takes to blend in this mode.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blending", meta = (ExposeOnSpawn))
	float BlendTime = 0.5f;

	// Function used for blending.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blending", meta = (ExposeOnSpawn))
	EAncientGameCameraModeBlendFunction BlendFunction = EAncientGameCameraModeBlendFunction::EaseOut;

	// Exponent used by blend functions to control the shape of the curve.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blending", meta = (ExposeOnSpawn))
	float BlendExponent = 4.f;

public:
	// Called when this camera mode is activated on the camera mode stack.
	virtual void OnActivation(AActor* TargetActor);
	// Called when this camera mode is deactivated on the camera mode stack.
	virtual void OnDeactivation();

	void UpdateCameraMode(float DeltaTime, AActor* TargetActor);

	const FAncientGameCameraModeView& GetCameraModeView() const { return View; }

	UFUNCTION(BlueprintPure, Category = "AncientGame|Camera")
	float GetBlendTime() const   { return BlendTime; }
	UFUNCTION(BlueprintPure, Category = "AncientGame|Camera")
	float GetBlendWeight() const { return BlendWeight; }

	void SetBlendWeight(float Weight);

protected:
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnActivation", Category = "AncientGame|Camera")
	void ReceiveActivation(AActor* TargetActor);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnDeactivation", Category = "AncientGame|Camera")
	void ReceiveDeactivation();

	UFUNCTION(BlueprintNativeEvent, DisplayName = "UpdateView", Category = "AncientGame|Camera")
	FAncientGameCameraModeView UpdateView(float DeltaTime, AActor* TargetActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "AncientGame|Camera", meta = (BlueprintProtected))
	FVector GetPivotLocation(AActor* TargetActor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "AncientGame|Camera", meta = (BlueprintProtected))
	FRotator GetPivotRotation(AActor* TargetActor) const;
	
	void UpdateBlending(float DeltaTime);

protected:
	// View output produced by the camera mode.
	FAncientGameCameraModeView View;

	// Linear blend alpha used to determine the blend weight.
	float BlendAlpha = 1.f;

	// Blend weight calculated using the blend alpha and function.
	float BlendWeight = 1.f;

private:
	void ActivateInternal(AActor* TargetActor);
	void DeactivateInternal();
	bool bIsActivated = false;

	friend struct FAncientGameCameraModeStack;
};
