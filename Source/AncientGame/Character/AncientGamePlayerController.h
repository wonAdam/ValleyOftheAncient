// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "ModularPlayerController.h"
#include "UObject/UObjectGlobals.h"

#include "AncientGamePlayerController.generated.h"

class UObject;
class UPlayer;

UCLASS()
class AAncientGamePlayerController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	//~ Begin APlayerController interface
	virtual void ReceivedPlayer() override;
	//~ End APlayerController interface

	FUNC_DECLARE_EVENT(AAncientGamePlayerController, FOnRecievedPlayerEvent, void, UPlayer*);
	FOnRecievedPlayerEvent OnRecievedPlayer;
};
