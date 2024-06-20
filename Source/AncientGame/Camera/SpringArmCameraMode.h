// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/AncientGameCameraMode.h"
#include "Camera/AncientGameCameraModeView.h"
#include "Camera/SpringArm.h"
#include "Interpolators.h"
#include "Math/MathFwd.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

#include "SpringArmCameraMode.generated.h"

class AActor;
class UObject;

UCLASS(Blueprintable)
class ANCIENTGAME_API USpringArmCameraMode : public UAncientGameCameraMode
{
	GENERATED_BODY()

public:

	USpringArmCameraMode();

	//~ Begin ULyraCameraMode interface
	virtual void OnActivation(AActor* TargetActor) override;
	//~ End ULyraCameraMode interface

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FSpringArm SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FVector Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FVector PivotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FCritDampSpringInterpolatorVector LocationSpringInterpolator = FCritDampSpringInterpolatorVector(15.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FCritDampSpringInterpolatorRotator RotationSpringInterpolator = FCritDampSpringInterpolatorRotator(15.f);

protected:

	//~ Begin UAncientGameCameraMode interface
	virtual FAncientGameCameraModeView UpdateView_Implementation(float DeltaTime, AActor* TargetActor) override;
	//~ End ULyraCameraMode interface
};
