// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineBaseTypes.h"
#include "Math/MathFwd.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"

#include "UproarStaticMeshListenerComponent.generated.h"

class AActor;
class UObject;
class UStaticMeshComponent;
class UUproarStaticMeshHitEventSettings;
struct FFrame;
struct FHitResult;

/**
 * 
 */
UCLASS(BlueprintType, ClassGroup = Uproar, meta = (BlueprintSpawnableComponent))
class UPROAR_API UUproarStaticMeshListenerComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:

	UUproarStaticMeshListenerComponent();

	//~ Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

	// When true, the component will draw debug points scaled to incoming events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugDraw = false;

	// Settings for interpreting Chaos Break Events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUproarStaticMeshHitEventSettings* UproarStaticMeshHitEventSettings;

	// When Physics Hit Events are received
	UFUNCTION()
	void OnHitEvent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	// Physics delegates
	FScriptDelegate HitEventDelegate;

	FComponentHitSignature OnComponentHit;

protected:

	// Cached parent actor
	UPROPERTY()
	AActor* ParentActor;

	// Cached sibling StaticMesh Component
	UPROPERTY()
	UStaticMeshComponent* ParentsStaticMeshComponent;

private:
	// Set up initial state for component
	void InitializeComponent();

	// Register Hit Event with sibling Static Mesh Component
	void RegisterPhysicsEvents();

	// Flag to determine if this component has been initialized and registered
	bool bInitialized;

};