// Copyright Epic Games, Inc. All Rights Reserved.

#include "AncientGamePlayerController.h"

#include "UObject/ObjectPtr.h"

void AAncientGamePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	OnRecievedPlayer.Broadcast(Player);
}
