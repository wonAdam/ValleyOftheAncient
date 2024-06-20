// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "HAL/Platform.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"
#include "UnderscoreCue.h"
#include "UnderscoreSection.h"

#include "UnderscoreCueBehavior.generated.h"

class UAudioComponent;
class UQuartzClockHandle;
class USoundBase;
class UUnderscoreSubsystem;
struct FFrame;

DECLARE_DYNAMIC_DELEGATE(FUnderscoreTransportEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnderscoreCueEvent, UUnderscoreCue*, UnderscoreCue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUnderscoreSectionStartedEvent, UUnderscoreCue*, Cue, class UUnderscoreSection*, Section);

USTRUCT()
struct FUnderscoreTransportEventHandle
{
	GENERATED_BODY()

	FUnderscoreTransportEvent Event;
	FUnderscoreTransport SyncPoint;
	bool bExecuteOnceOnly = true;
};

UENUM()
enum class EUnderscoreCueBehaviorPlayState : uint8
{
	Stopped,
	Queued,
	Playing,
	Transitioning,
	Paused
};

// Scheduling helper, since we don't want to queue up stingers way out in advance in case of a state change
struct FUnderscoreScheduledStinger
{
	FUnderscoreTransport StartTime;
	FGameplayTagQuery PlayCondition;

	FUnderscoreSectionStinger Stinger;
};

// An object that contains the logic to control a Cue
// Able to overridden with blueprint subclasses on a Per-Cue basis
UCLASS(Blueprintable)
class UNDERSCORE_API UUnderscoreCueBehavior : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly)
	UQuartzClockHandle* ClockHandle = nullptr;

	UPROPERTY(BlueprintAssignable)
	FUnderscoreCueEvent CueFinished;

	UPROPERTY()
	FOnQuartzMetronomeEventBP OnQuartzBeatEvent;

	UFUNCTION(BlueprintNativeEvent)
	void StartCue(UUnderscoreCue* InCue);

	UFUNCTION(BlueprintNativeEvent)
	void Stop(float FadeTime = 0.f);

	UFUNCTION(BlueprintNativeEvent)
	void Pause();

	UFUNCTION(BlueprintNativeEvent)
	void Resume();

	// Send a name to the Cue Manager and check for what stingers to play, if any
	UFUNCTION(BlueprintNativeEvent)
	void TriggerEvent(FName EventName);

	UFUNCTION(BlueprintCallable)
	void SetState(FGameplayTag InState);

	UFUNCTION(BlueprintCallable)
	void ClearState(FGameplayTag InState);

	UFUNCTION(BlueprintNativeEvent)
	void OnStateChanged();

	UFUNCTION(BlueprintImplementableEvent)
	void OnBeat(int32 Beat, int32 Bar);

	UFUNCTION(BlueprintCallable)
	bool IsPlaying() const;

	UFUNCTION(BlueprintCallable)
	float GetBPM() const { return Cue != nullptr ? Cue->BPM : 0; }

	// Get the length of the currently playing section in bars, which is also the number of bars before the Transport loops back to 1
	UFUNCTION(BlueprintCallable)
	int32 GetSectionLength() const { return CurrentTransport.WrapLength; }

	UFUNCTION(BlueprintCallable)
	FQuartzTimeSignature GetTimeSignature() const { return Cue != nullptr ? Cue->TimeSignature : FQuartzTimeSignature(); }

	UFUNCTION(BlueprintCallable)
	UUnderscoreCue* GetCue() const { return Cue; }

	UFUNCTION(BlueprintCallable)
	EUnderscoreCueBehaviorPlayState GetPlayState() { return PlayState; }

	UFUNCTION(BlueprintCallable)
	void SubscribeEventToTransport(const FUnderscoreTransport& InTransport, const FUnderscoreTransportEvent& InEvent, bool bExecuteOnceOnly = true);

	UFUNCTION(BlueprintNativeEvent)
	void OnQuartzBeat(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);

	void HandleAudioFinished(UAudioComponent* Component);

	void SetSubsystem(UUnderscoreSubsystem* InSubsystem) { Subsystem = InSubsystem; }

protected:
	// Which Cue we are playing
	UPROPERTY(Transient, BlueprintReadOnly)
	UUnderscoreCue* Cue = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly)
	EUnderscoreCueBehaviorPlayState PlayState = EUnderscoreCueBehaviorPlayState::Stopped;

	UPROPERTY(Transient, BlueprintReadOnly)
	UUnderscoreSection* CurrentSection = nullptr;

	// What time we are at Musically
	UPROPERTY(Transient, BlueprintReadOnly)
	FUnderscoreTransport CurrentTransport;

	// The point at which we will need to start playing a new section
	UPROPERTY(Transient, BlueprintReadOnly)
	FUnderscoreTransport NextSectionStartTime;

	// The set of conditions that caused the current section to play
	UPROPERTY(Transient, BlueprintReadOnly)
	FGameplayTagQuery CurrentSectionCondition;

	UPROPERTY(Transient)
	UUnderscoreSubsystem* Subsystem;

	TArray<FUnderscoreScheduledStinger> PendingStingers;

	UPROPERTY(Transient)
	TArray<FUnderscoreMarker> PendingMarkers;

	UPROPERTY(Transient)
	TArray<FUnderscoreSectionLayer> ActiveLayers;

	UPROPERTY(Transient, BlueprintReadOnly)
	UUnderscoreSection* NextSection = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly)
	FUnderscoreTransition ActiveTransition;

	UPROPERTY(Transient, BlueprintReadOnly)
	bool bPendingTransition;

	// Are we waiting for the next 1|1?
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bPreRoll;

	// State we will automatically update at the end of this section, if nothing else happens
	UPROPERTY(Transient, BlueprintReadOnly)
	FGameplayTag DestinationState;

	void UpdateStingers();
	void AddStingerMarkers(const TArray<FUnderscoreMarker>& InMarkers);

	void UpdateSection();
	void UpdateMarkers();
	void UpdateTransportEvents();

	// Trigger the OnMarker event on the Subsystem
	UFUNCTION(BlueprintCallable)
	void BroadcastMarker(FName MarkerName);

	UFUNCTION(BlueprintCallable)
	UAudioComponent* ScheduleClipNextBeat(USoundBase* Sound, const float Volume = 1.f);

	bool ShouldPlayLayer(const FUnderscoreSectionLayer& Layer) const;

	// return true if the current section condition is no longer valid due to state changes
	bool NeedsTransition() const;
	void PlayTransition();
	void PlayLayers(TArray<FUnderscoreSectionLayer>& InLayers);

	void FadeOutLayers(TArray<FUnderscoreSectionLayer>& InLayers, float InFadeOutTime);

	bool NeedsToQueueSection() const;

	// Reset the state of the current Transport so that next beat is 1 | 1. Can optionally offset a number of beats before or after that point, ex. to facilitate pickups.
	UFUNCTION(BlueprintCallable)
	void ResetTransport(const int32 BeatOverride = 0);

	// Set how many bars until the Transport loops back to bar 1
	UFUNCTION(BlueprintCallable)
	void SetWrapLength(const int32 BarsBeforeTrasportWrap);

	// Evaluate current states and pick the first section whose condition is satisfied
	UFUNCTION(BlueprintCallable)
	UUnderscoreSection* GetNextSection();

	FUnderscoreTransition* GetTransitionForPendingSection(FUnderscoreTransport& OutStartTime);

	// Picks the next section to be played, and either adds it to the Queue or plays it immediately
	void QueueNextSection();

	// This Section will play next
	UFUNCTION(BlueprintCallable)
	void SetNextSection(UUnderscoreSection* Section, const int32 StartBar, const int32 StartBeat);

	TArray<UAudioComponent*> ActiveComponents;

	// Any events that need to be broadcast at a specific Bar/Beat;
	UPROPERTY()
	TArray<FUnderscoreTransportEventHandle> TransportEvents;
};