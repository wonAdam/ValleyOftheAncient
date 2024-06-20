// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"

#include "AncientGameInstance.generated.h"

class UObject;


UCLASS(Config = Game)
class UAncientGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

protected:
	//~ Begin UGameInstance interface
	virtual void Init() override;
	virtual void Shutdown() override;
	//~ End UGameInstance interface
};
