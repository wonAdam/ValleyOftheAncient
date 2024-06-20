// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarChaosListenerComponent.h"

#include "Chaos/ChaosEngineInterface.h"
#include "Chaos/ChaosGameplayEventDispatcher.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/EnumAsByte.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "HAL/PlatformCrt.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInterface.h"
#include "Math/Color.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Templates/Casts.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Uproar.h"
#include "UproarChaosBreakEventSettings.h"
#include "UproarChaosCollisionEventSettings.h"
#include "UproarDataTypes.h"
#include "UproarSubsystem.h"

class UStaticMeshComponent;


UUproarChaosListenerComponent::UUproarChaosListenerComponent()
	: ParentActor(nullptr)
	, bInitialized(false)
{
}

void UUproarChaosListenerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Initialization requires registering with a child component that may not be created yet, so we try until we succeed
	if (!bInitialized)
	{
		InitializeComponent();
	}
}

void UUproarChaosListenerComponent::OnBreakEvent(const FChaosBreakEvent& ChaosBreakEvent)
{
	UWorld* World = GetWorld();

	if (World && ChaosBreakEvent.Component)
	{
		// Get length of Angular Velocity vector
		float AngularVelocityLength = ChaosBreakEvent.AngularVelocity.Size();

		// Initialize our Physical Surface Type
		EPhysicalSurface PhysicalSurfaceType = EPhysicalSurface::SurfaceType_Default;

		// Cast to Static Mesh
		if (UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(ChaosBreakEvent.Component))
		{
			// Retrieve Physical Surface type off Geometry
			UPhysicalMaterial* GCCPhysMat = ChaosBreakEvent.Component->GetMaterial(0)->GetPhysicalMaterial();

			// If Physical Material is valid, get listed SurfaceType
			if (GCCPhysMat)
			{
				PhysicalSurfaceType = GCCPhysMat->SurfaceType;
			}
		}

		// Make sure Settings are still valid
		if (UproarChaosBreakEventSettings)
		{
			// Set up Listener Event Data struct, which we will populate with data before passing up to the Subsystem
			FUproarPhysicsListenerEventData PhysicsListenerEventData;

			// Retrieve Event Magnitude data classification
			if (UproarChaosBreakEventSettings->GetBreakEventMagnitude(ChaosBreakEvent.Mass, PhysicsListenerEventData.Magnitude))
			{
				// Populate Listener Event Data struct with location and Event Type
				PhysicsListenerEventData.Location = ChaosBreakEvent.Location;
				PhysicsListenerEventData.PhysicsEventType = EUproarPhysicsEventType::BREAK;

				// If we succeed in retrieving a Collision Speed data classification, proceed
				if (UproarChaosBreakEventSettings->GetBreakEventSpeed(AngularVelocityLength, PhysicsListenerEventData.Speed))
				{

					// Retrieve Physical Surface type from Event Settings (checking for overrides)
					PhysicsListenerEventData.SurfaceType = UproarChaosBreakEventSettings->GetBreakEventSurfaceType(PhysicalSurfaceType);

					// If UproarSubsystem exists, send the Listener Event up
					if (UUproarSubsystem* UproarSubsystem = World->GetSubsystem<UUproarSubsystem>())
					{
						UproarSubsystem->PhysicsEvent(PhysicsListenerEventData);
					}

					// If debug draw is on, draw points for each successful Physics Event
					if (bDebugDraw)
					{
						// Give Point a scalar based on an arbitrary method for displaying significance
						float PointScale = 10.0f * ((float)PhysicsListenerEventData.Magnitude * (float)PhysicsListenerEventData.Speed + 1.0f);
						DrawDebugPoint(World, ChaosBreakEvent.Location, PointScale, FColor::Red, false, 1.0f, (uint8)('\000'));
					}
				}
			}
		}

		// Regardless of whether not there was an Uproar physics event, if debug draw is true, log the event
		if (bDebugDraw)
		{
			FString SurfaceType = UEnum::GetValueAsString(PhysicalSurfaceType);

			FString GCCName = ChaosBreakEvent.Component->GetFName().ToString();
			UE_LOG(LogUproar, Display, TEXT("Breaking Event: %s, Surface Type: %s, Mass: %f, AngularVelocity: %f"), *GCCName, *SurfaceType, ChaosBreakEvent.Mass, ChaosBreakEvent.Velocity.Size());
		}
	}
}


void UUproarChaosListenerComponent::OnCollisionEvent(const FChaosPhysicsCollisionInfo& ChaosPhysicsCollisionInfo)
{
	UWorld* World = GetWorld();

	if (World && ChaosPhysicsCollisionInfo.Component)
	{
		// Get length of Velocity vector
		float VelocityLength = ChaosPhysicsCollisionInfo.Velocity.Size();

		// Initialize our Physical Surface Type
		EPhysicalSurface PhysicalSurfaceType = EPhysicalSurface::SurfaceType_Default;

		// Cast to Static Mesh
		if (UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(ChaosPhysicsCollisionInfo.Component))
		{
			// Retrieve Physical Surface type off Geometry
			UPhysicalMaterial* GCCPhysMat = ChaosPhysicsCollisionInfo.Component->GetMaterial(0)->GetPhysicalMaterial();

			// If Physical Material is valid, get listed SurfaceType
			if (GCCPhysMat)
			{
				PhysicalSurfaceType = GCCPhysMat->SurfaceType;
			}
		}

		// Make sure Settings are still valid
		if (UproarChaosCollisionEventSettings)
		{

			// Set up Listener Event Data struct, which we will populate with data before passing up to the Subsystem
			FUproarPhysicsListenerEventData PhysicsListenerEventData;

			// Retrieve Event Magnitude data classification
			if (UproarChaosCollisionEventSettings->GetCollisionEventMagnitude(ChaosPhysicsCollisionInfo.Mass, PhysicsListenerEventData.Magnitude))
			{
				// Populate Listener Event Data struct with location and Event Type
				PhysicsListenerEventData.Location = ChaosPhysicsCollisionInfo.Location;
				PhysicsListenerEventData.PhysicsEventType = EUproarPhysicsEventType::COLLISION;

				// Retrieve Event Speed data classification
				if (UproarChaosCollisionEventSettings->GetCollisionEventSpeed(ChaosPhysicsCollisionInfo.Velocity.Size(), PhysicsListenerEventData.Speed, PhysicsListenerEventData.VolumeMod))
				{
					// Retrieve Physical Surface type from Event Settings (checking for overrides)
					PhysicsListenerEventData.SurfaceType = UproarChaosCollisionEventSettings->GetCollisionEventSurfaceType(PhysicalSurfaceType);

					// If UproarSubsystem exists, send the Listener Event up
					if (UUproarSubsystem* UproarSubsystem = World->GetSubsystem<UUproarSubsystem>())
					{
						UproarSubsystem->PhysicsEvent(PhysicsListenerEventData);
						
						if (bDebugDraw)
						{
							FString ComponentNameString = ChaosPhysicsCollisionInfo.Component->GetFName().ToString();
							FString PhysicsEventTypeString = UEnum::GetValueAsString(PhysicsListenerEventData.PhysicsEventType);
							FString PhysicsEventMagnitudeString = UEnum::GetValueAsString(PhysicsListenerEventData.Magnitude);
							FString PhysicsEventSpeedString = UEnum::GetValueAsString(PhysicsListenerEventData.Speed);

							UE_LOG(LogUproar, Display, TEXT("Collision Listener Event: %s, Event Type: %s, Speed: %s, Magnitude: %s"), *ComponentNameString, *PhysicsEventTypeString, *PhysicsEventSpeedString, *PhysicsEventMagnitudeString);
						}

					}

					// If debug draw is on, draw points for each successful Physics Event
					if (bDebugDraw)
					{
						float PointScale = 10.0f * ((float)PhysicsListenerEventData.Magnitude * (float)PhysicsListenerEventData.Speed + 1.0f);
						DrawDebugPoint(World, ChaosPhysicsCollisionInfo.Location, PointScale, FColor::Yellow, false, 1.0f, (uint8)('\000'));
					}

				}
			}
		}

		// Regardless of whether not there was an Uproar physics event, if debug draw is true, log the event
		if (bDebugDraw)
		{
			FString SurfaceType = UEnum::GetValueAsString(PhysicalSurfaceType);

			FString GCCName = ChaosPhysicsCollisionInfo.Component->GetFName().ToString();
			UE_LOG(LogUproar, Display, TEXT("Collision Event: %s, Surface Type: %s, Velocity: %f, Mass: %f"), *GCCName, *SurfaceType, ChaosPhysicsCollisionInfo.Velocity.Size(), ChaosPhysicsCollisionInfo.Mass);
		}
	}

}

void UUproarChaosListenerComponent::InitializeComponent()
{
	ParentActor = Cast<AActor>(GetOwner());

	if (ParentActor)
	{
		for (UActorComponent* ParentsActorComponent : ParentActor->GetComponents())
		{

			if (UGeometryCollectionComponent* GeometryCollectionComponent = Cast<UGeometryCollectionComponent>(ParentsActorComponent))
			{
				ParentsGeometryCollectionComponents.Add(GeometryCollectionComponent);

				// Geometry Collection Component is valid, register with Subsystem
				RegisterChaosEvents(GeometryCollectionComponent);
			}

		}
		
	}
}

void UUproarChaosListenerComponent::RegisterChaosEvents(UGeometryCollectionComponent* GeometryCollectionComponent)
{
	if (GeometryCollectionComponent)
	{
		// If we have valid Geometry Collection Component to listen to, try to bind events
		// This function may be called several times until binding is successful
		if (!BreakEventDelegate.IsBoundToObject(this))
		{
			BreakEventDelegate.BindUFunction(this, TEXT("OnBreakEvent"));
			GeometryCollectionComponent->OnChaosBreakEvent.Add(BreakEventDelegate);
		}

		if (!PhysicsCollisionDelegate.IsBoundToObject(this))
		{
			PhysicsCollisionDelegate.BindUFunction(this, TEXT("OnCollisionEvent"));
			GeometryCollectionComponent->OnChaosPhysicsCollision.Add(PhysicsCollisionDelegate);
		}

		// If we've successfully bound both Break and Collision events, flag as initialized
		bInitialized = (BreakEventDelegate.IsBoundToObject(this) && PhysicsCollisionDelegate.IsBoundToObject(this));
	}
}