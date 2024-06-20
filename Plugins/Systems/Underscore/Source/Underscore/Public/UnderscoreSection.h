// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"
#include "HAL/Platform.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"

#include "UnderscoreSection.generated.h"

class UAudioComponent;
class USoundBase;
class UUnderscoreSection;
struct FUnderscoreTransport;

UENUM(BlueprintType)
enum class EUnderscoreStingerQuantization : uint8
{
	Bar,
	Beat
};

USTRUCT(BlueprintType)
struct FUnderscoreQuantizationRules
{
	GENERATED_BODY()

	// where in the music this is allowed to play
	UPROPERTY(EditAnywhere)
	EUnderscoreStingerQuantization Quantization = EUnderscoreStingerQuantization::Beat;

	// If we are in Beat Quantization mode, leaving this false will allow this stinger to play on any beat
	UPROPERTY(EditAnywhere)
	bool bRestrictBeatSyncPoints = false;

	// What Beats we are allowed to play on
	// ex {1,3} will allow it to start on beats 1 and 3, but not 2 or 4, etc
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "bRestrictBeatSyncPoints"))
	TArray<int32> AllowedBeats;

	// If true, this stinger can only be played on certain measures specified explicitly in the array below
	UPROPERTY(EditAnywhere)
	bool bRestrictBarSyncPoints = false;

	// What Bars we are allowed to play in
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "bRestrictBarSyncPoints"))
	TArray<int32> AllowedBars;

	// Get the next allowed time that this stinger would be allowed to play on
	bool GetNextTriggerPoint(const FUnderscoreTransport& InTransport, FUnderscoreTransport& OutStartTime) const;
	bool IsValidTriggerPoint(const FUnderscoreTransport& InTransport) const;
};


USTRUCT(BlueprintType)
struct FUnderscoreMarker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Bar = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Beat = 0;

	// The Name broadcast by this Marker
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MarkerName;
};

// Helper data type to keep our code squeaky clean
USTRUCT(BlueprintType)
struct FUnderscoreTransport
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Bar = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Beat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WrapLength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FQuartzTimeSignature TimeSignature;

	void Wrap();
	void Increment();
	int32 ToBeats() const;
	bool operator==(const FUnderscoreTransport& Other) const { return Other.Bar == this->Bar && Other.Beat == this->Beat; }

	static FUnderscoreTransport FromBeats(const int32 InNumBeats, const int32 InWrapLength, const FQuartzTimeSignature& InTimeSignature);
};

// A Vertical layer that can exist as part of a larger Section
// Layers are currently locked to the parent Section length
USTRUCT(BlueprintType)
struct FUnderscoreSectionLayer
{
	GENERATED_BODY()

	// The Layer itself
	UPROPERTY(EditAnywhere)
	USoundBase* Sound = nullptr;

	// If true, this layer will play silently and fade in when enabled if it is enabled after the start of the section. Otherwise, will not play until a loop or transition
	// Note: crossfading requires the Soundwave to have Play when Silent enabled in its concurrency settings
	UPROPERTY(EditAnywhere)
	bool bCrossfade = false;

	// How long should this layer fade in or out when toggled, in Seconds
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bCrossfade"))
	float CrossfadeTime = 0.5f;

	// Layer will only play if this PlayCondition is met, or if it is left empty
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery PlayCondition;

	// Handle to the audio component playing this layer, once it is played
	UPROPERTY(Transient, BlueprintReadOnly)
	UAudioComponent* AudioComponent = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly)
	bool bPlaying = false;
};


// A Musical OneShot
USTRUCT(BlueprintType)
struct FUnderscoreSectionStinger
{
	GENERATED_BODY()

	// The Event that will cause this stinger to play, if its conditions are also met
	// Multiple Stingers can share the same event, but only the one that can play the soonest will play
	UPROPERTY(EditAnywhere)
	FName PlayEvent;

	// Is this Stinger allowed to play during certain states, but not others?
	// Each Layer can also have its own conditions, as well
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery PlayCondition;

	// When (in time) is this stinger allowed to play
	UPROPERTY(EditAnywhere)
	FUnderscoreQuantizationRules PlayRules;

	// Any points in this stinger that should broadcast back to the game
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreMarker> Markers;

	// Layers in this stinger, which can optionally have their own play conditions
	UPROPERTY(EditAnywhere)
	TMap<USoundBase*, FGameplayTagQuery> Sounds;

	// Handle to the component that is playing this stinger currently
	UPROPERTY(Transient, BlueprintReadOnly)
	TArray<UAudioComponent*> AudioComponents;
};

USTRUCT(BlueprintType)
struct FUnderscoreTransition
{
	GENERATED_BODY()

	// Section this transition is expecting to come from
	// If empty, it can play coming from any section
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUnderscoreSection* From = nullptr;

	// Section this transition is expecting to go to
	// If empty, it can play going to any section
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUnderscoreSection* To = nullptr;

	// Length of this transition in Bars
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Length = 0;

	// Length of the pickup to this transition. Important for keeping the transport accurate during transitions
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PickupLength = 0;

	// How quickly to fade out the previously playing section. Negative to let persist
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FadeTime = -1.f;

	// When is this Transition allowed to start
	// This ignores PickupLength, so set to the point where you actually want the clip to start
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FUnderscoreQuantizationRules PlayRules;

	// Layers in this transition, which can have their own play conditions
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreSectionLayer> Layers;
};

// A section of a track, which can have interactive or rules-based playback properties
UCLASS(Blueprintable)
class UNDERSCORE_API UUnderscoreSection : public UObject
{
	GENERATED_BODY()

public:
	// length in bars before it should be allowed to loop or cue a new Section
	UPROPERTY(EditAnywhere)
	int32 Length;

	// # of beats before a new bar this should start
	UPROPERTY(EditAnywhere)
	int32 PickupLength;

	// Should we loop, or just end?
	UPROPERTY(EditAnywhere)
	bool bLoop;

	// Vertical layers that can play back simultaneously. Assumed all layers are equal in length
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreSectionLayer> Layers;

	// Any stingers that could be triggered
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreSectionStinger> Stingers;

	// Any points in this Section that should broadcast when they happen
	UPROPERTY(EditAnywhere)
	TArray<FUnderscoreMarker> Markers;

	// State to set automatically when finishing this section
	UPROPERTY(EditAnywhere)
	FGameplayTag DestinationState;

	// Section to play after this section. Will play any corresponding transitions between these two sections first
	UPROPERTY(EditAnywhere)
	UUnderscoreSection* DestinationSection;
};