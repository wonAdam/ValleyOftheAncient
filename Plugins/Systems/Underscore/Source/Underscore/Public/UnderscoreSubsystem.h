// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "HAL/Platform.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/NameTypes.h"
#include "UObject/UObjectGlobals.h"
#include "UnderscoreCueBehavior.h"

#include "UnderscoreSubsystem.generated.h"

class FSubsystemCollectionBase;
class UAudioComponent;
class UObject;
class UQuartzClockHandle;
class USoundBase;
class UWorld;
struct FFrame;

namespace Underscore
{
	static const FName ClockName = FName(TEXT("UnderscoreClock"));
}

class UUnderscoreCue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnderscoreSyncEvent, FName, EventName);

UCLASS(BlueprintType)
class UNDERSCORE_API UUnderscoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FUnderscoreSyncEvent OnMarker;

	UPROPERTY(BlueprintAssignable)
	FUnderscoreCueEvent OnCueFinishedEvent;

	UPROPERTY(BlueprintAssignable)
	FUnderscoreSectionStartedEvent OnNewSectionStartedEvent;

	// UGameInstanceSubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	// end UGameInstanceSubsystem 

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void StartCue(UUnderscoreCue* InCue);

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void Stop(float FadeTime = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void TriggerEvent(FName EventName);

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void Resume();

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	bool IsPlaying() const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	float GetBPM() const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void SetState(const FGameplayTag InState);

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void ClearState(const FGameplayTag InState);

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void ResetStates();

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	bool IsStateActive(const FGameplayTag& InState) const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	bool IsStateConditionValid(const FGameplayTagQuery& InCondition) const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	FQuartzTimeSignature GetTimeSignature() const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	UQuartzClockHandle* GetClock() const;

	UFUNCTION(BlueprintCallable, Category = "Underscore")
	UAudioComponent* PrepareComponent(USoundBase* Sound);

	// Helper to find the correct quartz clock for your underscore cue
	UFUNCTION(BlueprintCallable, Category = "Underscore")
	void SubscribeToQuantizationEvent(EQuartzCommandQuantization InQuantizationBoundary, const FOnQuartzMetronomeEventBP& OnQuantizationEvent);

	void OnWorldBeginTeardown(UWorld* World);

protected:

	UPROPERTY(Transient)
	TArray<UAudioComponent*> ComponentPool;

	UPROPERTY(Transient)
	UUnderscoreCueBehavior* CueManager = nullptr;

	UPROPERTY(Transient)
	UQuartzClockHandle* ClockHandle = nullptr;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveStates;

	UFUNCTION()
	UAudioComponent* CreateNewAudioComponent(USoundBase* Sound);
};