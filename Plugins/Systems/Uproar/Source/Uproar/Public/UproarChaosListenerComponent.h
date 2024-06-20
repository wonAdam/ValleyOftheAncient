// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chaos/ChaosEventListenerComponent.h"
#include "Chaos/ChaosNotifyHandlerInterface.h"
#include "Containers/Array.h"
#include "Engine/EngineBaseTypes.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"

#include "UproarChaosListenerComponent.generated.h"

class AActor;
class UObject;
class UUproarChaosBreakEventSettings;
class UUproarChaosCollisionEventSettings;
struct FChaosBreakEvent;
struct FFrame;

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup = Uproar, meta = (BlueprintSpawnableComponent))
class UPROAR_API UUproarChaosListenerComponent : public UChaosEventListenerComponent
{
	GENERATED_BODY()
	
public:

	UUproarChaosListenerComponent();

	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

	// When true, the component will draw debug points scaled to incoming events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugDraw = false;

	// Settings for interpreting Chaos Break Events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUproarChaosBreakEventSettings* UproarChaosBreakEventSettings;

	// Settings for interpreting Chaos Collision Events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUproarChaosCollisionEventSettings* UproarChaosCollisionEventSettings;

	// When Chaos Break Events are received
	UFUNCTION()
	void OnBreakEvent(const FChaosBreakEvent& ChaosBreakEvent);

	// When Chaos Collision Events are received
	UFUNCTION()
	void OnCollisionEvent(const FChaosPhysicsCollisionInfo& ChaosPhysicsCollisionInfo);

	// Chaos Physics delegates
	FScriptDelegate BreakEventDelegate;
	FScriptDelegate PhysicsCollisionDelegate;

	FOnChaosBreakEvent OnChaosBreakEvent;
	FOnChaosPhysicsCollision OnChaosPhysicsCollision;


protected:

	// Cached parent actor
	UPROPERTY()
	AActor* ParentActor;

	// Cached sibling Geometry Collection Component
	UPROPERTY()
	TArray<UGeometryCollectionComponent*> ParentsGeometryCollectionComponents;

private:

	// Set up initial state for component
	void InitializeComponent();

	// Register with Subsystem
	void RegisterChaosEvents(UGeometryCollectionComponent* GeometryCollectionComponent);

	// Flag to determine if this component has been initialized and registered
	bool bInitialized;


};
