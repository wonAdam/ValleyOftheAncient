// Copyright Epic Games, Inc. All Rights Reserved.

#include "LoadingUtilLibrary.h"

#include "Engine/CoreSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AssertionMacros.h"
#include "UObject/Object.h"

#include <cfloat>

bool ULoadingUtilLibrary::HasCapturedDefaults = false;
float ULoadingUtilLibrary::DefaultLevelStreamingComponentsRegistrationGranularity;
float ULoadingUtilLibrary::DefaultLevelStreamingActorsUpdateTimeLimit;
float ULoadingUtilLibrary::DefaultAsyncLoadingTimeLimit;

void ULoadingUtilLibrary::ApplyDefaultPriorityLoading(const UObject* WorldContextObject)
{
	// Call first, just in case defaults have not been captured yet
	CaptureDefaultLoadingSettings();
	ApplyCustomPriorityLoading(WorldContextObject, false, DefaultAsyncLoadingTimeLimit, DefaultLevelStreamingActorsUpdateTimeLimit);
}

void ULoadingUtilLibrary::ApplyStreamingPriorityLoading(const UObject* WorldContextObject)
{	
	ApplyCustomPriorityLoading(WorldContextObject, false, 10.0f, 10.0f);
}

void ULoadingUtilLibrary::ApplyHighestPriorityLoading(const UObject* WorldContextObject)
{
	ApplyCustomPriorityLoading(WorldContextObject, true, FLT_MAX, FLT_MAX);
}

void ULoadingUtilLibrary::ApplyCustomPriorityLoading(const UObject* WorldContextObject, bool UseHighPriorityLoading, float MaxAsyncLoadingMilliSeconds, float MaxActorUpdateMilliSeconds)
{
	CaptureDefaultLoadingSettings();

	if (!ensure(WorldContextObject != nullptr))
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();

	if (!ensure(World != nullptr))
	{
		return;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();

	if (!ensure(WorldSettings != nullptr))
	{
		return;
	}

	WorldSettings->bHighPriorityLoadingLocal = UseHighPriorityLoading;
	GLevelStreamingActorsUpdateTimeLimit = MaxActorUpdateMilliSeconds;
	GLevelStreamingComponentsRegistrationGranularity = DefaultLevelStreamingComponentsRegistrationGranularity;
	GAsyncLoadingUseFullTimeLimit = UseHighPriorityLoading;
	GAsyncLoadingTimeLimit = MaxAsyncLoadingMilliSeconds;
}

void ULoadingUtilLibrary::FlushLevelStreaming(const UObject* WorldContextObject)
{
	if (!ensure(WorldContextObject != nullptr))
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();

	if (!ensure(World != nullptr))
	{
		return;
	}

	GEngine->BlockTillLevelStreamingCompleted(World);
}

void ULoadingUtilLibrary::ForceGarbageCollection()
{
#if WITH_EDITOR
	GEngine->ForceGarbageCollection(false);
#else
	GEngine->ForceGarbageCollection(true);
#endif
}

void ULoadingUtilLibrary::CaptureDefaultLoadingSettings()
{
	if (!HasCapturedDefaults)
	{
		DefaultLevelStreamingComponentsRegistrationGranularity = GLevelStreamingComponentsRegistrationGranularity;
		DefaultLevelStreamingActorsUpdateTimeLimit = GLevelStreamingActorsUpdateTimeLimit;
		DefaultAsyncLoadingTimeLimit = GAsyncLoadingTimeLimit;
		HasCapturedDefaults = true;
	}
}
