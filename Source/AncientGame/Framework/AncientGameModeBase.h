// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "Math/MathFwd.h"
#include "ModularGameMode.h"
#include "UObject/UObjectGlobals.h"

#include "AncientGameModeBase.generated.h"

class UObject;
struct FFrame;

/**
 * 
 */
UCLASS(BlueprintType)
class ANCIENTGAME_API AAncientGameModeBase : public AModularGameModeBase
{
	GENERATED_BODY()
public:

	//~ Begin AGameModeBase Interface
	virtual void StartPlay() override;
	//~ End AGameModeBase Interface

	//~ Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BeginDarkWorldTransition(FVector QuerySourceLocation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void PrefetchDarkWorld(FVector QuerySourceLocation);

	UFUNCTION(BlueprintCallable)
	void ToggleGameFeaturePlugin(FString GameFeaturePluginName, bool bEnable);

private:
	void UnloadPluginsPreWorldTick(UWorld* World, ELevelTick TickType, float DeltaTime);

	TSet<FString> PluginsToUnloadPreWorldTick;
};
