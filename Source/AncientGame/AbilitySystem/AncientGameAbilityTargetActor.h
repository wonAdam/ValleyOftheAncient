// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/GameplayAbilityTargetTypes.h" // for FGameplayAbilityTargetDataHandle
#include "UObject/UObjectGlobals.h"

#include "AncientGameAbilityTargetActor.generated.h"

class UGameplayAbility;
class UObject;
struct FFrame;

UCLASS(Abstract)
class AAncientGameAbilityTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	//~ Begin AGameplayAbilityTargetActor interface
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual bool IsConfirmTargetingAllowed() override;
	virtual void ConfirmTargetingAndContinue() override;
	//~ End AGameplayAbilityTargetActor interface

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "StartTargeting")
	void RecieveStartTargeting(UGameplayAbility* Ability);

	UFUNCTION(BlueprintNativeEvent, DisplayName = "IsConfirmTargetingAllowed")
	bool K2_IsConfirmTargetingAllowed();

	UFUNCTION(BlueprintImplementableEvent)
	FGameplayAbilityTargetDataHandle MakeTargetData();
};