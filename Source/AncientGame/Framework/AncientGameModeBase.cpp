// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/AncientGameModeBase.h"
#include "GameFeaturesSubsystem.h"
#include "Engine/Engine.h"

void AAncientGameModeBase::StartPlay()
{
	// Make sure level streaming is up to date before triggering NotifyMatchStarted
	GEngine->BlockTillLevelStreamingCompleted(GetWorld());
	Super::StartPlay();
}

void AAncientGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	PluginsToUnloadPreWorldTick.Empty();
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
}

void AAncientGameModeBase::ToggleGameFeaturePlugin(FString GameFeaturePluginName, bool bEnable)
{
	FString PluginURL;
	UGameFeaturesSubsystem::Get().GetPluginURLByName(GameFeaturePluginName, PluginURL);

	if (bEnable)
	{
		UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete());
	}
	else
	{
		// Unloading plugins cause garbagae collection. Delay the unload to avoid garbage collecting during a world tick.
		PluginsToUnloadPreWorldTick.Add(PluginURL);
		if (!FWorldDelegates::OnWorldTickStart.IsBoundToObject(this))
		{
			FWorldDelegates::OnWorldTickStart.AddUObject(this, &AAncientGameModeBase::UnloadPluginsPreWorldTick);
		}
	}
}

void AAncientGameModeBase::UnloadPluginsPreWorldTick(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	for (const FString& PluginToUnload : PluginsToUnloadPreWorldTick)
	{
		UGameFeaturesSubsystem::Get().UnloadGameFeaturePlugin(PluginToUnload);
	}
	
	PluginsToUnloadPreWorldTick.Empty();

	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
}