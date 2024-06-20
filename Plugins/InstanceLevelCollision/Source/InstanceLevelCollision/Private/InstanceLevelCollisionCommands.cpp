// Copyright Epic Games, Inc. All Rights Reserved.

#include "InstanceLevelCollisionCommands.h"

#define LOCTEXT_NAMESPACE "InstanceLevelCollisionModule"

void InstanceLevelCollisionCommands::RegisterCommands()
{
	UI_COMMAND(InstanceLevelCollisionWidget, "Generate Collision", "Generate Collision Widget", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE