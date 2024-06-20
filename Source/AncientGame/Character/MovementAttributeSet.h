// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/AncientGameAbilityAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Misc/AssertionMacros.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#include "MovementAttributeSet.generated.h"

class UObject;

UCLASS()
class UMovementAttributeSet : public UAncientGameAbilityAttributeSet
{
	GENERATED_BODY()

public:
	ATTRIBUTE_ACCESSORS(UMovementAttributeSet, MoveSpeed)

private:
	//~ Begin UAttributeSet interface
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//~ End UAttributeSet interface

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MoveSpeed;
};