// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarStaticMeshListenerComponent.h"

#include "Chaos/ChaosEngineInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/EnumAsByte.h"
#include "Containers/UnrealString.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInterface.h"
#include "Math/Color.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtr.h"
#include "Uproar.h"
#include "UproarDataTypes.h"
#include "UproarStaticMeshHitEventSettings.h"
#include "UproarSubsystem.h"

UUproarStaticMeshListenerComponent::UUproarStaticMeshListenerComponent()
	: ParentActor(nullptr)
	, ParentsStaticMeshComponent(nullptr)
	, bInitialized(false)
{
	// Initialize component tick so that it occurs after Physics for the most immediate physics state
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUproarStaticMeshListenerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Initialization requires registering with a child component that may not be created yet, so we try until we succeed
	if (!bInitialized)
	{
		InitializeComponent();
	}
}

void UUproarStaticMeshListenerComponent::OnHitEvent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	UWorld* World = GetWorld();

	// Validate World
	if (World && HitComponent)
	{
		// Hit result gives us an impulse vector, however on Static Meshes, mass is constant, 
		// so we can interpret (mv2 - mv1) as m(v2 - v1) or mass multiplied by delta v.
		// Here I'm dividing the scale of delta v by the Static Mesh's constant mass.
		// The Max makes sure my GetMass does not return 0.
		float DeltaVelocityLength = NormalImpulse.Size() / FMath::Max(HitComponent->GetMass(), SMALL_NUMBER);

		// Initialize our Physical Surface Type
		EPhysicalSurface PhysicalSurfaceType = EPhysicalSurface::SurfaceType_Default;

		// Cast to Static Mesh
		if (UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(HitComponent))
		{
			// Retrieve Physical Surface type off Geometry
			UPhysicalMaterial* GCCPhysMat = HitComponent->GetMaterial(0)->GetPhysicalMaterial();

			// If Physical Material is valid, get listed SurfaceType
			if (GCCPhysMat)
			{
				PhysicalSurfaceType = GCCPhysMat->SurfaceType;
			}
		}

		// Make sure Settings are still valid
		if (UproarStaticMeshHitEventSettings)
		{
			// Set up Listener Event Data struct, which we will populate with data before passing up to the Subsystem
			FUproarPhysicsListenerEventData PhysicsListenerEventData;

			// Retrieve Event Magnitude data classification
			PhysicsListenerEventData.Magnitude = UproarStaticMeshHitEventSettings->GetCollisionEventMagnitude();
			
			// Populate Listener Event Data struct with location and Event Type
			PhysicsListenerEventData.Location = Hit.Location;
			PhysicsListenerEventData.PhysicsEventType = EUproarPhysicsEventType::COLLISION;

			// If we succeed in retrieving a Collision Speed data classification, proceed
			if (UproarStaticMeshHitEventSettings->GetCollisionEventSpeed(DeltaVelocityLength, PhysicsListenerEventData.Speed, PhysicsListenerEventData.VolumeMod))
			{
				// Retrieve Physical Surface type from Event Settings (checking for overrides)
				PhysicsListenerEventData.SurfaceType = UproarStaticMeshHitEventSettings->GetCollisionEventSurfaceType(PhysicalSurfaceType);

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
					DrawDebugPoint(World, Hit.Location, PointScale, FColor::Red, false, 1.0f, (uint8)('\000'));
				}
			}
		}

		// Regardless of whether not there was a Uproar physics event, if debug draw is true, log the event
		if (bDebugDraw)
		{
			FString SurfaceType = UEnum::GetValueAsString(PhysicalSurfaceType);

			FString GCCName = HitComponent->GetFName().ToString();
			UE_LOG(LogUproar, Display, TEXT("Static Mesh Hit Event: %s, Surface Type: %s, Mass: %f, DeltaVelocity: %f"), *GCCName, *SurfaceType, HitComponent->GetMass(), DeltaVelocityLength);
		}
	}
}

void UUproarStaticMeshListenerComponent::InitializeComponent()
{
	ParentActor = static_cast<AActor*>(this->GetOwner());

	if (ParentActor)
	{
		ParentsStaticMeshComponent = ParentActor->FindComponentByClass<UStaticMeshComponent>();

		if (ParentsStaticMeshComponent)
		{
			// Static Mesh Component is valid, register events
			RegisterPhysicsEvents();
		}
	}
}

void UUproarStaticMeshListenerComponent::RegisterPhysicsEvents()
{
	// Validate cached Static Mesh Component
	if (ParentsStaticMeshComponent)
	{
		// Attempt to bind object if not yet bound
		if (!HitEventDelegate.IsBoundToObject(this))
		{
			HitEventDelegate.BindUFunction(this, TEXT("OnHitEvent"));
			ParentsStaticMeshComponent->OnComponentHit.Add(HitEventDelegate);
		}

		// Listener is initialized when its successfully bound to the object
		bInitialized = HitEventDelegate.IsBoundToObject(this);
	}
}

