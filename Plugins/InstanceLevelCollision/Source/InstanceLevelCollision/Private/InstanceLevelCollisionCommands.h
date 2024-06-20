// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"


class InstanceLevelCollisionCommands : public TCommands<InstanceLevelCollisionCommands>
{
public:

	InstanceLevelCollisionCommands()
		: TCommands<InstanceLevelCollisionCommands>
		(
			TEXT("Collision Tool"),
			NSLOCTEXT("CollisionTool", "Collision Tool", "CollisionTool plugin"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()
			) {}

	virtual void RegisterCommands() override;

public:

	TSharedPtr<FUICommandInfo> InstanceLevelCollisionWidget;
};