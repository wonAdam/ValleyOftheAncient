// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarSubsystem.h"

#include "AudioDevice.h"
#include "Containers/EnumAsByte.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "HAL/PlatformCrt.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Math/Color.h"
#include "Math/Transform.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "Templates/Tuple.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPath.h"
#include "Uproar.h"
#include "UproarDataTypes.h"
#include "UproarSettings.h"

class FSubsystemCollectionBase;



void UUproarSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Get Subsystem World
	SubsystemWorld = GetWorld();

	// Get world
	if (SubsystemWorld)
	{
		// Get Audio Device Handle
		AudioDevice = SubsystemWorld->GetAudioDeviceRaw();
	}

	if (AudioDevice == nullptr)
	{
		UE_LOG(LogUproar, Error, TEXT("Audio Device Invalid."));
	}

	// Get plugin settings on Subsystem initialization
	if (const UUproarProjectSettings* ProjectSettings = GetDefault<UUproarProjectSettings>())
	{
		// Pass in Project Settings Data to Subsystem
		GridSize = FMath::Clamp(ProjectSettings->UproarSpatialGridSize, 1000.0f, 20000.0f);

		GridCellSize = FMath::Clamp(ProjectSettings->UproarSpatialGridCellSize, 20.0f, 10000.0f);

		GridDimensionMax = GridSize / GridCellSize;
		GridDimensionMaxInverted = GridCellSize / GridSize;
		GridConversion = 1 / GridCellSize;

		bDrawDebug = ProjectSettings->bDrawDebugCells;

		MaxLifetime = ProjectSettings->UproarSoundEventLifespanSeconds;

		FSoftObjectPath SoundDefinitionPath = ProjectSettings->UproarSoundDefinition;


		// Attempt to load Sound Definition Library
		if (UObject* SDObject = SoundDefinitionPath.TryLoad())
		{
			if (UDataTable* SoundDefinition = Cast<UDataTable>(SDObject))
			{
				for (auto& It : SoundDefinition->GetRowMap())
				{
					FUproarSoundDefinition* Definition = reinterpret_cast<FUproarSoundDefinition*>(It.Value);

					if (Definition)
					{
						int32 DefinitionKey = UproarFunctionLibrary::GenerateUproarSoundDefinitionKey(Definition->SurfaceType, Definition->EventType, Definition->Magnitude, Definition->Speed);
						USoundBase* DefinitionValue = Definition->Sound;

						if (DefinitionValue)
						{
							SoundDefinitionLibrary.Add(DefinitionKey, DefinitionValue);
						}
					}
				}
			}

		}
	}

	// Subsystem should not tick before init or after deinit
	bShouldTick = true;
}

void UUproarSubsystem::Deinitialize()
{
	bShouldTick = false;
}

bool UUproarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Check with parent first
	if (Super::ShouldCreateSubsystem(Outer) == false)
	{
		return false;
	}

	// Get World
	if (UWorld* OuterWorld = Outer->GetWorld())
	{
		if (FAudioDevice* OuterAudioDevice = OuterWorld->GetAudioDeviceRaw())
		{
			// If we do have an audio device, we can create this subsystem.
			return DoesSupportWorldType(OuterWorld->WorldType);
		}
	}

	// If we do not have an audio device, we do not need to create this subsystem.
	return false;
}

bool UUproarSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return (WorldType == EWorldType::Game || WorldType == EWorldType::PIE);
}


void UUproarSubsystem::Tick(float DeltaTime)
{
	// Just in case
	if (bShouldTick)
	{
		UpdateActiveEvents(DeltaTime);
		UpdateActiveEventHash();

		GenerateActiveEventsFromPendingEvents();
		ClearPendingEvents();
	}
}

bool UUproarSubsystem::IsTickable() const
{
	return bShouldTick;
}

bool UUproarSubsystem::IsTickableInEditor() const
{
	return false;
}

bool UUproarSubsystem::IsTickableWhenPaused() const
{
	return false;
}

TStatId UUproarSubsystem::GetStatId() const
{
	return TStatId();
}

void UUproarSubsystem::PhysicsEvent(const FUproarPhysicsListenerEventData& PhysicsListenerEventData)
{
	int32 PhysicsListenerEventHashKey = UproarFunctionLibrary::GenerateUproarSoundDefinitionKey(PhysicsListenerEventData.SurfaceType, PhysicsListenerEventData.PhysicsEventType, PhysicsListenerEventData.Magnitude, PhysicsListenerEventData.Speed);

	FUproarActivePhysicsEvent CandidateEvent;
	
	CandidateEvent.VolumeMod = PhysicsListenerEventData.VolumeMod;
	CandidateEvent.EventLocation = PhysicsListenerEventData.Location;
	CandidateEvent.MaxLifetime = MaxLifetime;
	CandidateEvent.PhysicsEventType = PhysicsListenerEventHashKey;
	CandidateEvent.Lifetime = 0.0f;

	PendingEvents.Add(CandidateEvent);

}

int32 UUproarSubsystem::GetSpatialHashID(FVector InLocation)
{
	return (FMath::FloorToInt(InLocation.X * GridConversion)) 
		+ (FMath::FloorToInt(InLocation.Y * GridConversion) * GridDimensionMax) 
		+ (FMath::FloorToInt(InLocation.Z * GridConversion) * GridDimensionMax * GridDimensionMax);
}

FVector UUproarSubsystem::GetSpatialHashCellCenter(FVector InLocation)
{
	FVector CellCenterPoint;

	CellCenterPoint.X = (FMath::FloorToFloat(InLocation.X * GridConversion) + 0.5f) * GridCellSize;
	CellCenterPoint.Y = (FMath::FloorToFloat(InLocation.Y * GridConversion) + 0.5f) * GridCellSize;
	CellCenterPoint.Z = (FMath::FloorToFloat(InLocation.Z * GridConversion)  + 0.5f) * GridCellSize;

	return CellCenterPoint;
}


FVector UUproarSubsystem::GetClosestListenerRelativeToLocation(FVector InLocation)
{
	FVector ListenerRelativeLocation;

	if (ensureMsgf(AudioDevice, TEXT("AudioDevice is invalid.")))
	{
		// Retrieve listener proxies
		TArray<FListenerProxy>& ListenerTransforms = AudioDevice->ListenerProxies;

		if (ListenerTransforms.Num())
		{
			// Get the first relative location
			ListenerRelativeLocation = InLocation + ListenerTransforms[0].Transform.GetLocation();

			for (auto It = ListenerTransforms.CreateConstIterator(); It; ++It)
			{
				// Get Location and Direction from Listener
				const FVector ListenerLocation = It->Transform.GetLocation();
				FVector CurrentRelativeListener = InLocation + ListenerLocation;

				// Try to find the shortest Listener distance from the source, that will likely be the relative listener position for the playing sound
				if (CurrentRelativeListener.Size() < ListenerRelativeLocation.Size())
				{
					ListenerRelativeLocation = CurrentRelativeListener;
				}
			}

		}
	}

	return ListenerRelativeLocation;
}

void UUproarSubsystem::UpdateActiveEvents(float InDeltaTime)
{
	TArray<FUproarActivePhysicsEvent> EventsToKeep;

	for (auto It = ActiveEvents.CreateIterator(); It; ++It)
	{
		It->Lifetime = It->Lifetime + InDeltaTime;

		if (It->Lifetime < It->MaxLifetime)
		{
			EventsToKeep.Add(*It);
		}
	}

	ActiveEvents.Empty();
	ActiveEvents.Append(EventsToKeep);
}

void UUproarSubsystem::UpdateActiveEventHash()
{
	ActiveEventHash.Empty();

	for (auto It = ActiveEvents.CreateIterator(); It; ++It)
	{
		FVector ListenerRelativeLocation = GetClosestListenerRelativeToLocation(It->EventLocation);
		int32 ActiveEventHashKey = GetSpatialHashID(ListenerRelativeLocation);

		ActiveEventHash.FindOrAdd(ActiveEventHashKey, *It);
	}
}

void UUproarSubsystem::GenerateActiveEventsFromPendingEvents()
{
	// Cycle through pending events and generate sounds for all the valid ones
	for (auto It = PendingEvents.CreateIterator(); It; ++It)
	{
		// Get Listener Relative Location, the Spatial Hash is oriented around the Listener
		FVector ListenerRelativeLocation = GetClosestListenerRelativeToLocation(It->EventLocation);
		int32 ActiveEventHashKey = GetSpatialHashID(ListenerRelativeLocation);

		// Search to see if this Spatial Hash is already used, if not, then we can add this pending event
		if (!ActiveEventHash.Find(ActiveEventHashKey))
		{
			// Look up Sound in our SoundDefinitionLibrary, Return Double Pointer
			if (USoundBase** SoundPointer = SoundDefinitionLibrary.Find(It->PhysicsEventType))
			{
				USoundBase* Sound = *SoundPointer;

				// Validate and make sure Subsystem World is still valid
				if (Sound && SubsystemWorld)
				{
					// Play sound at actual event location
					UGameplayStatics::PlaySoundAtLocation(SubsystemWorld, Sound, It->EventLocation, It->VolumeMod);

					// Add Event to Active Event Hash
					ActiveEventHash.Add(ActiveEventHashKey, *It);

					// Add Event to Active Event List
					ActiveEvents.Add(*It);

					// If Draw Debug is true, calculate hash centerpoint and place box visualization at it
					if (bDrawDebug)
					{
						FVector CellCenter = GetSpatialHashCellCenter(It->EventLocation);

						FVector CellScale = FVector(1.0f, 1.0f, 1.0f);
						FTransform CellTransform;

						CellTransform.SetScale3D(CellScale * (GridCellSize * 0.5f));

						// Draw debug box at cell space
						DrawDebugBox(SubsystemWorld, CellCenter, FVector(GridCellSize * 0.5f), FColor::Orange, false, MaxLifetime, '\000', 1.0f);
					}
				}

			}
		}
	}
}

void UUproarSubsystem::ClearPendingEvents()
{
	PendingEvents.Empty();
}
