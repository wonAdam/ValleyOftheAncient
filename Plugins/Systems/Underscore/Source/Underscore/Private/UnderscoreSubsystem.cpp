// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreSubsystem.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformCrt.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Quartz/QuartzSubsystem.h"
#include "Templates/SubclassOf.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Class.h"
#include "Underscore.h"
#include "UnderscoreCue.h"
#include "UnderscoreCueBehavior.h"

class FSubsystemCollectionBase;
class UObject;
class USoundBase;

bool UUnderscoreSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GEngine->UseSound();
}

void UUnderscoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::OnWorldBeginTeardown);
}

void UUnderscoreSubsystem::Deinitialize()
{
	ClockHandle = nullptr;
}

void UUnderscoreSubsystem::StartCue(UUnderscoreCue* InCue)
{
	if (InCue == nullptr)
	{
		return;
	}

	if (CueManager)
	{
		CueManager->Stop();
	}

	bool bSupported = true;
	UWorld* World = GetWorldChecked(bSupported);

	UClass* CueManagerClass = InCue->ManagerClassOverride != nullptr ? *InCue->ManagerClassOverride : UUnderscoreCueBehavior::StaticClass();
	CueManager = NewObject<UUnderscoreCueBehavior>(this, CueManagerClass);

	if (CueManager == nullptr)
	{
		UE_LOG(LogUnderscore, Error, TEXT("Underscore Failed to create instance of Underscore Cue Manager %s"), *CueManagerClass->GetName());
		return;
	}

	if (ClockHandle == nullptr)
	{
		if (UQuartzSubsystem* Quartz = World->GetSubsystem<UQuartzSubsystem>())
		{
			ClockHandle = Quartz->CreateNewClock(World, Underscore::ClockName, { InCue->TimeSignature, true }, true);

			if (ClockHandle == nullptr)
			{
				return;
			}

			ClockHandle->SetBeatsPerMinute(World, FQuartzQuantizationBoundary(), {}, ClockHandle, InCue->BPM);
		}
	}

	CueManager->ClockHandle = ClockHandle;
	CueManager->SetSubsystem(this);
	CueManager->StartCue(InCue);

	if (ClockHandle->IsClockRunning(World) == false)
	{
		ClockHandle->ResumeClock(World, ClockHandle);
	}
}

void UUnderscoreSubsystem::Stop(float FadeTime)
{
	if (CueManager)
	{
		CueManager->Stop(FadeTime);
		CueManager = nullptr;
	}

	// destroy the clock, so that the next clock can start rightaway
	if (UQuartzSubsystem* Quartz = GetWorld()->GetSubsystem<UQuartzSubsystem>())
	{
		Quartz->DeleteClockByHandle(GetWorld(), ClockHandle);
	}
}

void UUnderscoreSubsystem::TriggerEvent(FName EventName)
{
	if (CueManager)
	{
		CueManager->TriggerEvent(EventName);
	}
}

void UUnderscoreSubsystem::Pause()
{
	if (ClockHandle == nullptr)
	{
		return;
	}
	//pause clock + cue
	ClockHandle->PauseClock(GetWorld(), ClockHandle);

	if (CueManager)
	{
		CueManager->Pause();
	}
}

void UUnderscoreSubsystem::Resume()
{
	if (ClockHandle == nullptr)
	{
		return;
	}

	//resume clock and cue
	ClockHandle->ResumeClock(GetWorld(), ClockHandle);

	if (CueManager)
	{
		CueManager->Resume();
	}
}

bool UUnderscoreSubsystem::IsPlaying() const
{
	if (CueManager)
	{
		return CueManager->IsPlaying();
	}

	return false;
}

float UUnderscoreSubsystem::GetBPM() const
{
	if (CueManager)
	{
		return CueManager->GetBPM();
	}

	return 0.f;
}

void UUnderscoreSubsystem::SetState(const FGameplayTag InState)
{
	if (InState.IsValid() == false)
	{
		return;
	}

	FGameplayTagContainer TagsToClear = ActiveStates.Filter(InState.RequestDirectParent().GetSingleTagContainer());
	ActiveStates.RemoveTags(TagsToClear);

	ActiveStates.AddLeafTag(InState);

	if (CueManager)
	{
		CueManager->OnStateChanged();
	}

	UE_LOG(LogUnderscore, Verbose, TEXT("ActiveStates Updated: %s"), *ActiveStates.ToStringSimple(true));
}

void UUnderscoreSubsystem::ClearState(const FGameplayTag InState)
{
	FGameplayTagContainer TagsToClear = ActiveStates.Filter(InState.GetSingleTagContainer());
	ActiveStates.RemoveTags(TagsToClear);

	if (CueManager)
	{
		CueManager->OnStateChanged();
	}

	UE_LOG(LogUnderscore, Verbose, TEXT("ActiveStates Updated: %s"), *ActiveStates.ToStringSimple(true));
}

void UUnderscoreSubsystem::ResetStates()
{
	ActiveStates.Reset();
}

bool UUnderscoreSubsystem::IsStateActive(const FGameplayTag& InState) const
{
	return ActiveStates.HasTag(InState);
}

bool UUnderscoreSubsystem::IsStateConditionValid(const FGameplayTagQuery& InCondition) const
{
	return InCondition.IsEmpty() || InCondition.Matches(ActiveStates);
}

FQuartzTimeSignature UUnderscoreSubsystem::GetTimeSignature() const
{
	if (CueManager)
	{
		return CueManager->GetTimeSignature();
	}

	return FQuartzTimeSignature();
}

UQuartzClockHandle* UUnderscoreSubsystem::GetClock() const
{
	return ClockHandle;
}

void UUnderscoreSubsystem::SubscribeToQuantizationEvent(EQuartzCommandQuantization InQuantizationBoundary, const FOnQuartzMetronomeEventBP& OnQuantizationEvent)
{
	if (ClockHandle != nullptr)
	{
		ClockHandle->SubscribeToQuantizationEvent(GetWorld(), InQuantizationBoundary, OnQuantizationEvent, ClockHandle);
	}
}

void UUnderscoreSubsystem::OnWorldBeginTeardown(UWorld* World)
{
	if (World == GetWorld())
	{
		ClockHandle = nullptr;
	}
}

UAudioComponent* UUnderscoreSubsystem::CreateNewAudioComponent(USoundBase* Sound)
{
	// GameInstance should have a consistent dummy-world outside of editor / PIE edge cases
	if (UWorld* World = GetWorld())
	{
		const FAudioDevice::FCreateComponentParams Params = FAudioDevice::FCreateComponentParams(World);
		UAudioComponent* AudioComponent = FAudioDevice::CreateComponent(Sound, Params);

		if (AudioComponent == nullptr)
		{
			return nullptr;
		}

		AudioComponent->bAutoDestroy = false;
		AudioComponent->bIsUISound = true;
		AudioComponent->bAllowSpatialization = false;
		AudioComponent->bReverb = false;
		AudioComponent->bCenterChannelOnly = false;
		AudioComponent->bIsPreviewSound = false;

		return AudioComponent;
	}

	return nullptr;
}

UAudioComponent* UUnderscoreSubsystem::PrepareComponent(USoundBase* Sound)
{
	for (UAudioComponent* Component : ComponentPool)
	{
		if (Component && Component->IsPlaying() == false)
		{
			Component->SetSound(Sound);
			return Component;
		}
	}

	if (UAudioComponent* NewComponent = CreateNewAudioComponent(Sound))
	{
		ComponentPool.Add(NewComponent);
		return NewComponent;
	}

	return nullptr;
}