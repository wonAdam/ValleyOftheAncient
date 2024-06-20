// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Input/PlayerControlsComponent.h"
#include "Math/Rotator.h"
#include "UObject/UObjectGlobals.h"

#include "HoverDroneControlsComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class UObject;
struct FInputActionValue;
struct FMinimalViewInfo;

UCLASS(Blueprintable, BlueprintType, Category = "Input", meta = (BlueprintSpawnableComponent))
class UHoverDroneControlsComponent : public UPlayerControlsComponent
{
	GENERATED_BODY()

public:
	UHoverDroneControlsComponent();

protected:
	//~ Begin UPlayerControlsComponent interface
	virtual void SetupPlayerControls_Implementation(UEnhancedInputComponent* PlayerInputComponent) override;
	//~ End UPlayerControlsComponent interface

	/** Movement input handlers */
	void HandleMoveForward(const FInputActionValue& InputActionValue);
	void HandleMoveRight(const FInputActionValue& InputActionValue);
	void HandleMoveUp(const FInputActionValue& InputActionValue);
	void HandleTurn(const FInputActionValue& InputActionValue);
	void HandleLookUp(const FInputActionValue& InputActionValue);
	void HandleStartTurbo(const FInputActionValue& InputActionValue);
	void HandleStopTurbo(const FInputActionValue& InputActionValue);

	void AddRotationInput(const FRotator Input);

	/** Updates camera autofocus */
	void UpdateAutoFocus(FMinimalViewInfo& OutPOV, float DeltaTime);

	/** Update screen fringe settings based on fov */
	void UpdateSceneFringe(FMinimalViewInfo& OutPOV);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* MoveForwardInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* LookUpInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* MoveRightInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* SlideUp_WorldInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* TurboInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Controls")
	UInputAction* TurnInputAction;

};
