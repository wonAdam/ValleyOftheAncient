// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Engine/EngineTypes.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Stats/Stats2.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "UObject/UObjectGlobals.h"

#include "UproarSubsystem.generated.h"

class FAudioDevice;
class FSubsystemCollectionBase;
class UObject;
class USoundBase;
class UWorld;
struct FFrame;
struct FUproarPhysicsListenerEventData;

/** This Struct allows designers to associate MixStates with SoundControlBusMixes. */
USTRUCT()
struct UPROAR_API FUproarActivePhysicsEvent
{
	GENERATED_BODY()

	/**  */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	FVector EventLocation = FVector(0.f);

	/**  */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	int32 PhysicsEventType = 0;

	/**  */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	float MaxLifetime = 0.0f;

	/**  */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	float Lifetime = 0.0f;

	/**  */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	float VolumeMod = 1.0f;
};

/**
 * 
 */
UCLASS()
class UPROAR_API UUproarSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	// End USubsystem

public:
	// Begin FTickableGameObject
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	bool IsTickableInEditor() const override;
	bool IsTickableWhenPaused() const override;
	TStatId GetStatId() const override;
	// End FTickableGameObject

	UFUNCTION()
	void PhysicsEvent(const FUproarPhysicsListenerEventData& PhysicsListenerEventData);

private:

	// Sound Definition Library is a look up table for Physics Sound Events
	UPROPERTY()
	TMap<int32, USoundBase*> SoundDefinitionLibrary;

	int32 GetSpatialHashID(FVector InLocation);
	FVector GetSpatialHashCellCenter(FVector InLocation);

	FVector GetClosestListenerRelativeToLocation(FVector InLocation);

	// Spatial hash parameters
	float GridSize = 20000.0f;
	float GridCellSize = 75.0f;
	float GridDimensionMax = 0.0f;
	float GridDimensionMaxInverted = 0.0f;
	float GridConversion = 0.0f;

	// Sound event cell lifetime
	float MaxLifetime = 1.25f;

	bool bDrawDebug = false;
	
	bool bShouldTick = true;

	// Cached World Pointer
	UWorld* SubsystemWorld;

	// Cached Audio Device Pointer
	FAudioDevice* AudioDevice;

	TArray<FUproarActivePhysicsEvent> ActiveEvents;
	TArray<FUproarActivePhysicsEvent> PendingEvents;

	TMap<int32, FUproarActivePhysicsEvent> ActiveEventHash;

	void UpdateActiveEvents(float InDeltaTime);
	void UpdateActiveEventHash();
	void GenerateActiveEventsFromPendingEvents();
	void ClearPendingEvents();
};
