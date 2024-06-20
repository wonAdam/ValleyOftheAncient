// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "AncientGameBoxComponentBase.generated.h"

/** Making UBoxComponent Blueprintable */
UCLASS(Abstract, Blueprintable, meta=(BlueprintSpawnableComponent))
class UAncientGameBoxComponentBase : public UBoxComponent
{
	GENERATED_BODY()

public:
	
};
