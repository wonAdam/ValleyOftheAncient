// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Quartz/QuartzSubsystem.h"
#include "GameplayTagContainer.h"
#include "UnderscoreSection.h"
#include "UnderscoreCue.generated.h"

// A Complete Composition, composed of interactive Sections that can transition based on Game State changes
UCLASS(Blueprintable)
class UNDERSCORE_API UUnderscoreCue : public UObject
{
	GENERATED_BODY()

public:
	// Beats per minute in the below Time Signature
	UPROPERTY(EditAnywhere)
	float BPM = 120.f;

	UPROPERTY(EditAnywhere)
	FQuartzTimeSignature TimeSignature;
	
	// Optional states set when starting this cue
	UPROPERTY(EditAnywhere)
	TArray<FGameplayTag> InitialStates;

	// simple horizontal transition map
	UPROPERTY(EditAnywhere)
	TMap<UUnderscoreSection*, FGameplayTagQuery> Sections;

	// if specified, use the logic of this class to control this Cue, instead of the default
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUnderscoreCueBehavior> ManagerClassOverride;

	// Sounds to play between sections and the rules for when to play them
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreTransition> Transitions;
};