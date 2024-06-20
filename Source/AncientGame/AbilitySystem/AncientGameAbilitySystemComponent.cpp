// Copyright Epic Games, Inc. All Rights Reserved.

#include "AncientGameAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/AncientGameAbilityAttributeSet.h"
#include "Animation/AncientGameAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "AttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Delegates/Delegate.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformCrt.h"
#include "Templates/Casts.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

void UAncientGameAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (AbilityActorInfo)
	{
		if (AbilityActorInfo->AnimInstance == nullptr)
		{
			AbilityActorInfo->AnimInstance = AbilityActorInfo->GetAnimInstance();
		}

		if (UAncientGameAnimInstance* TopAnimInststance = Cast<UAncientGameAnimInstance>(AbilityActorInfo->AnimInstance))
		{
			TopAnimInststance->InitializeWithAbilitySystem(this);
		}

		if (AbilityActorInfo->SkeletalMeshComponent.IsValid())
		{
			const USkeletalMeshComponent* SkeletalMeshComponent = AbilityActorInfo->SkeletalMeshComponent.Get();

			const TArray<UAnimInstance*>& AnimLayers = SkeletalMeshComponent->GetLinkedAnimInstances();
			for (UAnimInstance* AnimLayer : AnimLayers)
			{
				if (UAncientGameAnimInstance* AncientGameAnimLayer = Cast<UAncientGameAnimInstance>(AnimLayer))
				{
					AncientGameAnimLayer->InitializeWithAbilitySystem(this);
				}
			}
		}

		if (UGameInstance* GameInstance = InOwnerActor->GetGameInstance())
		{
			// Sign up for possess/unpossess events so that we can update the cached AbilityActorInfo accordingly
			GameInstance->GetOnPawnControllerChanged().AddDynamic(this, &UAncientGameAbilitySystemComponent::OnPawnControllerChanged);
		}
	}

	GrantDefaultAbilitiesAndAttributes();
}

void UAncientGameAbilitySystemComponent::BeginDestroy()
{
	if (AbilityActorInfo && AbilityActorInfo->OwnerActor.IsValid())
	{
		if (UGameInstance* GameInstance = AbilityActorInfo->OwnerActor->GetGameInstance())
		{
			GameInstance->GetOnPawnControllerChanged().RemoveAll(this);
		}
	}

	Super::BeginDestroy();
}

FGameplayAbilitySpecHandle UAncientGameAbilitySystemComponent::GrantAbilityOfType(TSubclassOf<UGameplayAbility> AbilityType, bool bRemoveAfterActivation)
{
	FGameplayAbilitySpecHandle AbilityHandle;
	if (AbilityType)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityType);
		AbilitySpec.RemoveAfterActivation = bRemoveAfterActivation;

		AbilityHandle = GiveAbility(AbilitySpec);
	}
	return AbilityHandle;
}

void UAncientGameAbilitySystemComponent::GrantDefaultAbilitiesAndAttributes()
{
	// Reset/Remove abilities if we had already added them
	{
		for (UAttributeSet* AttribSetInstance : AddedAttributes)
		{
			RemoveSpawnedAttribute(AttribSetInstance);
		}

		for (FGameplayAbilitySpecHandle AbilityHandle : DefaultAbilityHandles)
		{
			SetRemoveAbilityOnEnd(AbilityHandle);
		}

		AddedAttributes.Empty(DefaultAttributes.Num());
		DefaultAbilityHandles.Empty(DefaultAbilities.Num());
	}

	// Default abilities
	{
		DefaultAbilityHandles.Reserve(DefaultAbilities.Num());
		for (const TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
		{
			if (*Ability)
			{
				DefaultAbilityHandles.Add(GiveAbility(FGameplayAbilitySpec(Ability)));
			}
		}
	}

	// Default attributes
	{
		for (const FAncientGameAttributeApplication& Attributes : DefaultAttributes)
		{
			if (Attributes.AttributeSetType)
			{
				UAttributeSet* NewAttribSet = NewObject<UAttributeSet>(this, Attributes.AttributeSetType);
				if (Attributes.InitializationData)
				{
					NewAttribSet->InitFromMetaDataTable(Attributes.InitializationData);
				}
				AddedAttributes.Add(NewAttribSet);
				AddAttributeSetSubobject(NewAttribSet);
			}
		}
	}
}

void UAncientGameAbilitySystemComponent::OnPawnControllerChanged(APawn* Pawn, AController* NewController)
{
	if (AbilityActorInfo && AbilityActorInfo->OwnerActor == Pawn && AbilityActorInfo->PlayerController != NewController)
	{
		// Reinit the cached ability actor info (specifically the player controller)
		AbilityActorInfo->InitFromActor(AbilityActorInfo->OwnerActor.Get(), AbilityActorInfo->AvatarActor.Get(), this);
	}
}
