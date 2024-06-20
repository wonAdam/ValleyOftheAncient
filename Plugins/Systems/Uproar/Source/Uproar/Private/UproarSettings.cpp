// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarSettings.h"

struct FPropertyChangedEvent;

UUproarProjectSettings::UUproarProjectSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
void UUproarProjectSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif