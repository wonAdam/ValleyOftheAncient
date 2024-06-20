// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Containers/Array.h"
#include "GameplayAbilitySpec.h"
#include "Templates/SubclassOf.h"
#include "UObject/UObjectGlobals.h"

#include "AncientGameAbilitySystemComponent.generated.h"

class AActor;
class AController;
class APawn;
class UAncientGameAbilityAttributeSet;
class UAttributeSet;
class UDataTable;
class UGameplayAbility;
class UObject;
struct FFrame;

USTRUCT()
struct FAncientGameAttributeApplication
{
	GENERATED_BODY()

	// Ability set to grant
	UPROPERTY(EditAnywhere)
	TSubclassOf<UAncientGameAbilityAttributeSet> AttributeSetType;

	// Data table reference to initialize the attributes with, if any (can be left unset)
	UPROPERTY(EditAnywhere)
	UDataTable* InitializationData = nullptr;
};

UCLASS(meta=(BlueprintSpawnableComponent))
class UAncientGameAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TArray<FAncientGameAttributeApplication> DefaultAttributes;

	//~ Begin UAbilitySystemComponent interface
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	//~ End UAbilitySystemComponent interface

	//~ Begin UObject interface
	virtual void BeginDestroy() override;
	//~ End UObject interface

	UFUNCTION(BlueprintCallable, Category = "AncientGame|Abilities")
	FGameplayAbilitySpecHandle GrantAbilityOfType(TSubclassOf<UGameplayAbility> AbilityType, bool bRemoveAfterActivation);

protected:
	void GrantDefaultAbilitiesAndAttributes();

	UFUNCTION() // UFunction to be able to bind with dynamic delegate
	void OnPawnControllerChanged(APawn* Pawn, AController* NewController);
	TArray<FGameplayAbilitySpecHandle> DefaultAbilityHandles;

	UPROPERTY(transient)
	TArray<UAttributeSet*> AddedAttributes;
};
