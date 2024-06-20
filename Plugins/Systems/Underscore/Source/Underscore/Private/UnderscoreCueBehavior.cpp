// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreCueBehavior.h"

#include "Components/AudioComponent.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "HAL/PlatformCrt.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Math/NumericLimits.h"
#include "Sound/SoundBase.h"
#include "Templates/Tuple.h"
#include "Trace/Detail/Channel.h"
#include "Underscore.h"
#include "UnderscoreCue.h"
#include "UnderscoreSubsystem.h"

// if any sound on a component is set to a volume below SMALL_NUMBER, it will stop no matter what its virtualization settings are
// This is equivalent to -70db, so effectively inaudible, but it won't be killed
constexpr float SilentLayerVolume = 1.e-7f;

void UUnderscoreCueBehavior::SubscribeEventToTransport(const FUnderscoreTransport& InTransport, const FUnderscoreTransportEvent& InEvent, bool bExecuteOnceOnly /*= true*/)
{
	TransportEvents.Add({ InEvent, InTransport, bExecuteOnceOnly } );
}

void UUnderscoreCueBehavior::OnQuartzBeat_Implementation(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	CurrentTransport.Beat++;
	CurrentTransport.Wrap();

	if (CurrentTransport.Bar == 1 && CurrentTransport.Beat == 1)
	{
		bPreRoll = false;
	}

	UE_LOG(LogUnderscore, VeryVerbose, TEXT("Current Transport: %i | %i"), CurrentTransport.Bar, CurrentTransport.Beat);

	UpdateTransportEvents();

	UpdateStingers();

	UpdateMarkers();
	
	if ((bPreRoll == false) && (NextSectionStartTime.ToBeats() <= CurrentTransport.ToBeats()))
	{
		if (DestinationState.IsValid())
		{
			Subsystem->SetState(DestinationState);
			DestinationState = FGameplayTag();
		}

		if (bPendingTransition)
		{
			PlayTransition();
		}
		else
		{
			UpdateSection();
		}
	}

	OnBeat(CurrentTransport.Beat, CurrentTransport.Bar);
}

// check if a stinger needs to happen, and play any that do
void UUnderscoreCueBehavior::UpdateStingers()
{
	for (auto StingerIt = PendingStingers.CreateIterator(); StingerIt; ++StingerIt)
	{
		if (StingerIt->StartTime == CurrentTransport)
		{
			StingerIt->Stinger.AudioComponents.Reset(StingerIt->Stinger.AudioComponents.Num());

			for (TPair<USoundBase*, FGameplayTagQuery>& StingerLayer : StingerIt->Stinger.Sounds)
			{
				if (Subsystem->IsStateConditionValid(StingerLayer.Value) == false)
				{
					continue;
				}

				if (UAudioComponent* Component = ScheduleClipNextBeat(StingerLayer.Key))
				{
					StingerIt->Stinger.AudioComponents.Add(Component);
				}
			}

			AddStingerMarkers(StingerIt->Stinger.Markers);
			StingerIt.RemoveCurrent();
		}
	}
}

void UUnderscoreCueBehavior::AddStingerMarkers(const TArray<FUnderscoreMarker>& InMarkers)
{
	for (FUnderscoreMarker Marker : InMarkers)
	{
		// Play marker Bar | Beat from now, instead of at exactly Bar | Beat
		if (Marker.Bar == 0 && Marker.Beat == 0)
		{
			Subsystem->OnMarker.Broadcast(Marker.MarkerName);
			UE_LOG(LogUnderscore, Verbose, TEXT("Broadcasting Marker %s immediately"), *Marker.MarkerName.ToString());
		}
		else
		{
			FUnderscoreTransport Transport = CurrentTransport;
			Transport.Bar += Marker.Bar;
			// Add one beat to compensate for the 1-beat offset of stinger queue times
			Transport.Beat += Marker.Beat + 1;
			Transport.Wrap();

			Marker.Bar = Transport.Bar;
			Marker.Beat = Transport.Beat;

			PendingMarkers.Add(Marker);

			UE_LOG(LogUnderscore, Verbose, TEXT("Queueing Marker %s on %i | %i"), *Marker.MarkerName.ToString(), Marker.Bar, Marker.Beat);
		}
	}
}

// Check if a transition or loop needs to happen
void UUnderscoreCueBehavior::UpdateSection()
{
	if (NeedsToQueueSection())
	{
		QueueNextSection();
	}

	if (NextSection == nullptr)
	{
		return;
	}

	PlayLayers(NextSection->Layers);

	CurrentSection = NextSection;
	NextSection = nullptr;
	ActiveTransition = FUnderscoreTransition();

	if (CurrentTransport.WrapLength != CurrentSection->Length)
	{
		// Set to PickupLength beats before 1 | 1
		ResetTransport(-CurrentSection->PickupLength);
		CurrentTransport.WrapLength = CurrentSection->Length;
	}

	if (CurrentSection->Markers.Num() > 0)
	{
		PendingMarkers.Append(CurrentSection->Markers);
	}

	if (CurrentSection->DestinationSection)
	{
		NextSection = CurrentSection->DestinationSection;
		NextSectionStartTime.Beat = -NextSection->PickupLength;

		// todo: transitionstarttime
		FUnderscoreTransport TransitionStartTime;
		if (FUnderscoreTransition* Transition = GetTransitionForPendingSection(TransitionStartTime))
		{
			bPendingTransition = true;
			ActiveTransition = *Transition;
		}
	}
	else
	{
		NextSectionStartTime.Beat = -CurrentSection->PickupLength;
	}

	if (CurrentSection->DestinationState.IsValid())
	{
		DestinationState = CurrentSection->DestinationState;
	}

	NextSectionStartTime.Bar = 1 + CurrentSection->Length;
	NextSectionStartTime.TimeSignature = Cue->TimeSignature;
	NextSectionStartTime.WrapLength = CurrentTransport.WrapLength;
	NextSectionStartTime.Wrap();

	UE_LOG(LogUnderscore, Verbose, TEXT("Next Section Expected at %i | %i"), NextSectionStartTime.Bar, NextSectionStartTime.Beat);

	if (PlayState == EUnderscoreCueBehaviorPlayState::Transitioning)
	{
		PlayState = EUnderscoreCueBehaviorPlayState::Playing;
	}
}

void UUnderscoreCueBehavior::UpdateMarkers()
{
	// check for any markers that need to broadcast this beat
	for (auto MarkerIt = PendingMarkers.CreateIterator(); MarkerIt; ++MarkerIt)
	{
		if (MarkerIt->Bar == CurrentTransport.Bar && MarkerIt->Beat == CurrentTransport.Beat)
		{
			BroadcastMarker(MarkerIt->MarkerName);
			MarkerIt.RemoveCurrent();
		}
	}
}

void UUnderscoreCueBehavior::UpdateTransportEvents()
{
	for (auto EventIt = TransportEvents.CreateIterator(); EventIt; ++EventIt)
	{
		if (CurrentTransport == EventIt->SyncPoint)
		{
			EventIt->Event.ExecuteIfBound();

			if (EventIt->bExecuteOnceOnly)
			{
				EventIt.RemoveCurrent();
			}
		}
	}
}

void UUnderscoreCueBehavior::BroadcastMarker(FName MarkerName)
{
	if (Subsystem)
	{
		Subsystem->OnMarker.Broadcast(MarkerName);
	}
}

void UUnderscoreCueBehavior::QueueNextSection()
{
	if (Subsystem == nullptr)
	{
		return;
	}

	NextSection = GetNextSection();

	if (NextSection == nullptr)
	{
		UE_LOG(LogUnderscore, Verbose, TEXT("No Section to Queue"));

		if (CurrentSection)
		{
			for (FUnderscoreSectionLayer& Layer : CurrentSection->Layers)
			{
				if (Layer.AudioComponent != nullptr)
				{
					Layer.AudioComponent->OnAudioFinishedNative.AddUObject(this, &ThisClass::HandleAudioFinished);
					return;
				}
			}
		}

		return;
	}
}

void UUnderscoreCueBehavior::SetNextSection(UUnderscoreSection* Section, const int32 StartBar, const int32 StartBeat)
{
	NextSection = Section;

	NextSectionStartTime.Bar = StartBar;
	NextSectionStartTime.Beat = StartBeat - 1;
	NextSectionStartTime.WrapLength = CurrentTransport.WrapLength;
	NextSectionStartTime.TimeSignature = CurrentTransport.TimeSignature;
	NextSectionStartTime.Wrap();
}

void UUnderscoreCueBehavior::HandleAudioFinished(UAudioComponent* Component)
{
	ActiveComponents.Remove(Component);
	Component->OnAudioFinishedNative.RemoveAll(this);

	if (ActiveComponents.Num() >= 0)
	{
		return;
	}

	if (Subsystem)
	{
		Subsystem->OnCueFinishedEvent.Broadcast(Cue);
	}

	CueFinished.Broadcast(Cue);
}

void UUnderscoreCueBehavior::OnStateChanged_Implementation()
{
	if (Subsystem == nullptr)
	{
		return;
	}

	// Remove newly invalidated Stingers from queue
	for (auto StingerIt = PendingStingers.CreateIterator(); StingerIt; ++StingerIt)
	{
		if (Subsystem->IsStateConditionValid(StingerIt->PlayCondition) == false)
		{
			StingerIt.RemoveCurrent();
		}
	}

	// Update Crossfades for any active layers
	for (FUnderscoreSectionLayer& Layer : ActiveLayers)
	{
		if (Layer.bCrossfade == false || Layer.AudioComponent == nullptr)
		{
			continue;
		}

		if (Subsystem->IsStateConditionValid(Layer.PlayCondition) != Layer.bPlaying)
		{
			const float TargetVolume = Layer.bPlaying ? SilentLayerVolume : 1.f;
			Layer.bPlaying = !Layer.bPlaying;
			Layer.AudioComponent->AdjustVolume(Layer.CrossfadeTime, TargetVolume);
		}
	}

	// Do we need to transition to a new section?
	if (Subsystem->IsStateConditionValid(CurrentSectionCondition) == false)
	{
		NextSection = nullptr;
		QueueNextSection();

		DestinationState = FGameplayTag();

		FUnderscoreTransport TransitionStartTime;
		if (FUnderscoreTransition* Transition = GetTransitionForPendingSection(TransitionStartTime))
		{
			// set target time to the beat before it starts so we can play it with quantization
			--TransitionStartTime.Beat;
			TransitionStartTime.Wrap();

			ActiveTransition = *Transition;

			if (TransitionStartTime == CurrentTransport)
			{
				PlayTransition();
			}
			else
			{
				bPendingTransition = true;
				NextSectionStartTime = TransitionStartTime;
			}
		}
		else
		{
			bPendingTransition = false;
			ActiveTransition = FUnderscoreTransition();

			// no transition necessary, just play the next section
			NextSectionStartTime.Bar = 1 + CurrentTransport.WrapLength;
			NextSectionStartTime.Beat = -NextSection->PickupLength;
			NextSectionStartTime.WrapLength = CurrentTransport.WrapLength;
			NextSectionStartTime.TimeSignature = Cue->TimeSignature;
			NextSectionStartTime.Wrap();
		}
	}
}

void UUnderscoreCueBehavior::StartCue_Implementation(UUnderscoreCue* InCue)
{
	if (InCue == nullptr || InCue->Sections.Num() == 0 || Cue != nullptr)
	{
		UE_LOG(LogUnderscore, Warning, TEXT("Underscore tried to play with invalid Cue!"));
		return;
	}

	Cue = InCue;

	OnQuartzBeatEvent.BindDynamic(this, &ThisClass::OnQuartzBeat);

	if (Subsystem)
	{
		Subsystem->SubscribeToQuantizationEvent(EQuartzCommandQuantization::Beat, OnQuartzBeatEvent);

		for (const FGameplayTag& State : Cue->InitialStates)
		{
			Subsystem->SetState(State);
		}
	}

	CurrentTransport.TimeSignature = Cue->TimeSignature;
	PlayState = EUnderscoreCueBehaviorPlayState::Playing;

	QueueNextSection();
}

void UUnderscoreCueBehavior::Stop_Implementation(float FadeTime)
{
	if (PlayState != EUnderscoreCueBehaviorPlayState::Stopped && CurrentSection != nullptr)
	{
		for (FUnderscoreSectionLayer& Layer : CurrentSection->Layers)
		{
			if (Layer.AudioComponent != nullptr)
			{
				Layer.AudioComponent->FadeOut(FadeTime, 0.f);
			}
		}
	}
}

void UUnderscoreCueBehavior::Pause_Implementation()
{
	if (PlayState == EUnderscoreCueBehaviorPlayState::Playing)
	{
		PlayState = EUnderscoreCueBehaviorPlayState::Paused;

		if (CurrentSection == nullptr)
		{
			return;
		}

		for (FUnderscoreSectionLayer& Layer : CurrentSection->Layers)
		{
			if (Layer.AudioComponent != nullptr)
			{
				Layer.AudioComponent->SetPaused(true);
			}
		}
	}
}

void UUnderscoreCueBehavior::Resume_Implementation()
{
	if (PlayState == EUnderscoreCueBehaviorPlayState::Paused)
	{
		PlayState = EUnderscoreCueBehaviorPlayState::Playing;

		if (CurrentSection == nullptr)
		{
			return;
		}

		for (FUnderscoreSectionLayer& Layer : CurrentSection->Layers)
		{
			if (Layer.AudioComponent != nullptr)
			{
				Layer.AudioComponent->SetPaused(false);
			}
		}
	}
}

void UUnderscoreCueBehavior::SetState(FGameplayTag InState)
{
	if (Subsystem)
	{
		Subsystem->SetState(InState);
	}
}

void UUnderscoreCueBehavior::ClearState(FGameplayTag InState)
{
	if (Subsystem)
	{
		Subsystem->ClearState(InState);
	}
}

bool UUnderscoreCueBehavior::IsPlaying() const
{
	return PlayState == EUnderscoreCueBehaviorPlayState::Playing;
}

void UUnderscoreCueBehavior::TriggerEvent_Implementation(FName EventName)
{
	if (CurrentSection == nullptr 
		|| PlayState != EUnderscoreCueBehaviorPlayState::Playing 
		|| Subsystem == nullptr
		|| Cue == nullptr)
	{
		return;
	}

	int32 NearestSyncPointBeats = TNumericLimits<int32>::Max();
	FUnderscoreSectionStinger* BestStinger = nullptr;

	FUnderscoreTransport EarliestStartPoint = CurrentTransport;
	EarliestStartPoint.Increment();

	FUnderscoreTransport BestSyncPoint;

	for (FUnderscoreSectionStinger& Stinger : CurrentSection->Stingers)
	{
		if (Stinger.PlayEvent != EventName 
			|| Stinger.Sounds.Num() == 0 
			|| Subsystem->IsStateConditionValid(Stinger.PlayCondition) == false)
		{
			continue;
		}

		FUnderscoreTransport OutTransport;
		if (Stinger.PlayRules.GetNextTriggerPoint(EarliestStartPoint, OutTransport))
		{
			UE_LOG(LogUnderscore, Verbose, TEXT("Next Trigger Point for Stinger %s is %i | %i"), *Stinger.PlayEvent.ToString(), OutTransport.Bar, OutTransport.Beat);

			int32 TotalBeats = OutTransport.ToBeats() - EarliestStartPoint.ToBeats();

			if (TotalBeats < 0)
			{
				TotalBeats += (EarliestStartPoint.WrapLength * Cue->TimeSignature.NumBeats);
			}

			if (TotalBeats < NearestSyncPointBeats)
			{
				NearestSyncPointBeats = TotalBeats;
				BestSyncPoint = OutTransport;
				BestStinger = &Stinger;
			}
		}
	}

	if (BestStinger)
	{
		// set target time to the beat before it starts so we can play it with quantization
		BestSyncPoint.Beat--;
		BestSyncPoint.Wrap();

		BestStinger->AudioComponents.Reset(BestStinger->AudioComponents.Num());

		if (BestSyncPoint == CurrentTransport)
		{
			for (TPair<USoundBase*, FGameplayTagQuery>& StingerLayer : BestStinger->Sounds)
			{
				if (Subsystem->IsStateConditionValid(StingerLayer.Value) == false)
				{
					continue;
				}

				//play ASAP
				if (UAudioComponent* Component = ScheduleClipNextBeat(StingerLayer.Key))
				{
					BestStinger->AudioComponents.Add(Component);
				}
			}

			AddStingerMarkers(BestStinger->Markers);
		}
		else
		{
			UE_LOG(LogUnderscore, Verbose, TEXT("Queueing Stinger for Event %s at %i | %i"), *BestStinger->PlayEvent.ToString(), BestSyncPoint.Bar, BestSyncPoint.Beat);

			FUnderscoreScheduledStinger NewStinger;
			NewStinger.PlayCondition = BestStinger->PlayCondition;
			NewStinger.Stinger = *BestStinger;
			NewStinger.StartTime = BestSyncPoint;

			PendingStingers.Add(NewStinger);
		}
	}
}

UAudioComponent* UUnderscoreCueBehavior::ScheduleClipNextBeat(USoundBase* Sound, const float Volume /* = 1.f*/)
{
	if (Sound == nullptr)
	{
		return nullptr;
	}

	if (UAudioComponent* Component = Subsystem->PrepareComponent(Sound))
	{
		static FQuartzQuantizationBoundary NextBeatBoundary = { EQuartzCommandQuantization::Beat, 1.f, EQuarztQuantizationReference::CurrentTimeRelative };
		Component->PlayQuantized(Component, ClockHandle, NextBeatBoundary, FOnQuartzCommandEventBP(), 0.f, 0.f, Volume);

		Component->OnAudioFinishedNative.AddUObject(this, &ThisClass::HandleAudioFinished);
		ActiveComponents.Add(Component);

		UE_LOG(LogUnderscore, Verbose, TEXT("Playing Clip Next Beat"), Sound != nullptr ? *Sound->GetName() : TEXT("None"));
		
		return Component;
	}
	else
	{
		UE_LOG(LogUnderscore, Warning, TEXT("Underscore Subsystem Failed to Create AudioComponent"));
	}

	return nullptr;
}

bool UUnderscoreCueBehavior::ShouldPlayLayer(const FUnderscoreSectionLayer& Layer) const
{
	if (Subsystem == nullptr)
	{
		return false;
	}

	// No assigned gameplay tag is equivalent to "Always Play This"
	if (Layer.PlayCondition.IsEmpty())
	{
		return true;
	}

	return Subsystem->IsStateConditionValid(Layer.PlayCondition);
}

bool UUnderscoreCueBehavior::NeedsTransition() const
{
	if (Subsystem == nullptr)
	{
		return false;
	}

	return CurrentSectionCondition.IsEmpty() || !(Subsystem->IsStateConditionValid(CurrentSectionCondition));
}

void UUnderscoreCueBehavior::PlayTransition()
{
	if (CurrentSection)
	{
		FadeOutLayers(CurrentSection->Layers, ActiveTransition.FadeTime);
	}

	PlayLayers(ActiveTransition.Layers);

	bPendingTransition = false;
	PlayState = EUnderscoreCueBehaviorPlayState::Transitioning;

	ResetTransport(-ActiveTransition.PickupLength);

	if (NextSection)
	{
		CurrentTransport.WrapLength = ActiveTransition.Length;

		NextSectionStartTime.Bar = 1 + CurrentTransport.WrapLength;
		NextSectionStartTime.Beat = -NextSection->PickupLength;
		NextSectionStartTime.WrapLength = CurrentTransport.WrapLength;
		NextSectionStartTime.TimeSignature = Cue->TimeSignature;
		NextSectionStartTime.Wrap();
	}
}

void UUnderscoreCueBehavior::PlayLayers(TArray<FUnderscoreSectionLayer>& InLayers)
{
	ActiveLayers.Reset(ActiveLayers.Num());

	for (FUnderscoreSectionLayer& Layer : InLayers)
	{
		Layer.bPlaying = false;

		if (ShouldPlayLayer(Layer) == false)
		{
			if (Layer.bCrossfade)
			{
				// Don't skip, but play silently in case we need to fade in
				if (UAudioComponent* Component = ScheduleClipNextBeat(Layer.Sound, SilentLayerVolume))
				{
					Layer.AudioComponent = Component;
					ActiveLayers.Add(Layer);
				}
			}
			else
			{
				UE_LOG(LogUnderscore, Verbose, TEXT("Skipping Layer %s"), Layer.Sound != nullptr ? *Layer.Sound->GetName() : TEXT("None"));
			}

			continue;
		}

		if (UAudioComponent* Component = ScheduleClipNextBeat(Layer.Sound, 1.f))
		{
			Layer.bPlaying = true;
			Layer.AudioComponent = Component;
			ActiveLayers.Add(Layer);
		}
	}
}

void UUnderscoreCueBehavior::FadeOutLayers(TArray<FUnderscoreSectionLayer>& InLayers, float InFadeOutTime)
{
	for (FUnderscoreSectionLayer& Layer : InLayers)
	{
		if (Layer.bPlaying && Layer.AudioComponent && InFadeOutTime >= 0.f)
		{
			Layer.AudioComponent->FadeOut(InFadeOutTime, 0.f);
		}
	}
}

bool UUnderscoreCueBehavior::NeedsToQueueSection() const
{
	return (PlayState == EUnderscoreCueBehaviorPlayState::Playing && NextSection == nullptr) || NeedsTransition();
}

void UUnderscoreCueBehavior::ResetTransport(const int32 BeatOverride/*= 0*/)
{
	// Quartz transport counts from 1. Set so that the next beat is 1 | 1
	CurrentTransport.Bar = 1;
	CurrentTransport.Beat = BeatOverride;

	if (BeatOverride <= 0)
	{
		bPreRoll = true;
	}
}

void UUnderscoreCueBehavior::SetWrapLength(const int32 BarsBeforeTrasportWrap)
{
	CurrentTransport.WrapLength = BarsBeforeTrasportWrap;
}

UUnderscoreSection* UUnderscoreCueBehavior::GetNextSection()
{
	if (Subsystem == nullptr)
	{
		return nullptr;
	}

	if (NeedsTransition())
	{
		for (auto SectionIt : Cue->Sections)
		{
			if (Subsystem->IsStateConditionValid(SectionIt.Value))
			{
				UE_LOG(LogUnderscore, Verbose, TEXT("Underscore Section Transition required: New Section: %s"), SectionIt.Key != nullptr ? *SectionIt.Key->GetName() : TEXT("None"));

				CurrentSectionCondition = SectionIt.Value;
				return SectionIt.Key;
			}
		}
	}

	if (CurrentSection)
	{
		if (CurrentSection->bLoop)
		{
			UE_LOG(LogUnderscore, Verbose, TEXT("Looping Section"));
			return CurrentSection;
		}

		if (CurrentSection->DestinationSection)
		{
			return CurrentSection->DestinationSection;
		}
	}

	return nullptr;
}

FUnderscoreTransition* UUnderscoreCueBehavior::GetTransitionForPendingSection(FUnderscoreTransport& OutStartTime)
{
	if (Subsystem == nullptr || NextSection == nullptr)
	{
		return nullptr;
	}

	FUnderscoreTransition* BestTranstition = nullptr;

	// Look for Transitions start Next beat so it can start on a musical boundary
	FUnderscoreTransport EarliestStartPoint = CurrentTransport;
	EarliestStartPoint.Increment();

	FUnderscoreTransport OutTransport;

	int32 NearestSyncPointBeats = TNumericLimits<int32>::Max();
	FUnderscoreTransport BestSyncPoint;

	for (FUnderscoreTransition& Transition : Cue->Transitions)
	{
		if ((Transition.To && Transition.To != NextSection) 
			|| (Transition.From && Transition.From != CurrentSection))
		{
			continue;
		}

		if (Transition.PlayRules.GetNextTriggerPoint(EarliestStartPoint, OutTransport))
		{
			UE_LOG(LogUnderscore, Verbose, TEXT("Next Trigger Point for Transition is %i | %i"), OutTransport.Bar, OutTransport.Beat);

			int32 TotalBeats = OutTransport.ToBeats() - EarliestStartPoint.ToBeats();

			if (TotalBeats < 0)
			{
				TotalBeats += (EarliestStartPoint.WrapLength * Cue->TimeSignature.NumBeats);
			}

			if (TotalBeats < NearestSyncPointBeats)
			{
				NearestSyncPointBeats = TotalBeats;
				BestSyncPoint = OutTransport;
				BestTranstition = &Transition;
			}
		}
	}

	OutStartTime = BestSyncPoint;
	return BestTranstition;
}
