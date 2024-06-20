// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
#include "Containers/EnumAsByte.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "UObject/UObjectGlobals.h"

#include "UproarDataTypes.generated.h"

class UObject;
class USoundBase;
struct FFrame;

/** This enum is for classifying the mass or size of physics events. */
UENUM(BlueprintType)
enum class EUproarPhysicsEventType : uint8
{
	BREAK		UMETA(DisplayName = "Break Event"),
	COLLISION	UMETA(DisplayName = "Collision Event"),
	EType_MAX	UMETA(Hidden)
};

/** This enum is for classifying the mass or size of physics events. */
UENUM(BlueprintType)
enum class EUproarMagnitude : uint8
{
	TINY		UMETA(DisplayName = "Tiny"),
	SMALL		UMETA(DisplayName = "Small"),
	MEDIUM		UMETA(DisplayName = "Medium"),
	LARGE		UMETA(DisplayName = "Large"),
	EPIC		UMETA(DisplayName = "Epic"),
	EType_MAX	UMETA(Hidden)
};

/** This enum is for classifying the velocity of physics events. */
UENUM(BlueprintType)
enum class EUproarSpeed : uint8
{
	SLOW		UMETA(DisplayName = "Slow"),
	MID_SPEED	UMETA(DisplayName = "Mid-Speed"),
	QUICK		UMETA(DisplayName = "Quick"),
	EPIC		UMETA(DisplayName = "Epic"),
	EType_MAX	UMETA(Hidden)
};

/** This enum is for classifying the velocity of physics events. */
UENUM(BlueprintType)
enum class EUproarSpatialGridSize : uint8
{
	SMALL		UMETA(DisplayName = "Small Grid Size"),
	MEDIUM		UMETA(DisplayName = "Medium Grid Size"),
	LARGE		UMETA(DisplayName = "Large Gride Size"),
	EType_MAX	UMETA(Hidden)
};

/** This Struct allows designers to associate MixStates with SoundControlBusMixes. */
USTRUCT(BlueprintType)
struct UPROAR_API FUproarPhysicsListenerEventData
{
	GENERATED_BODY()

	/** The location of the physics listener event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	FVector Location = FVector(0.f);

	/** The surface type of the physics listener event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	TEnumAsByte<EPhysicalSurface> SurfaceType = EPhysicalSurface::SurfaceType1;

	/** The type of physics listener event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	EUproarPhysicsEventType PhysicsEventType = EUproarPhysicsEventType::BREAK;

	/** The magnitude or size of the physics listener event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	EUproarMagnitude Magnitude = EUproarMagnitude::TINY;

	/** The speed of the physics listener event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	EUproarSpeed Speed = EUproarSpeed::SLOW;

	/** Listener determined volume modulation for the event. */
	UPROPERTY(EditAnywhere, meta = (Categories = "Uproar"))
	float VolumeMod = 1.0f;
};

/**  */
UCLASS(BlueprintType)
class UPROAR_API UproarFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Generates a hash key for mapping sounds in Uproars sound definition library. */
	UFUNCTION(BlueprintCallable)
	static int32 GenerateUproarSoundDefinitionKey(
		const TEnumAsByte<EPhysicalSurface> SurfaceType
		, const EUproarPhysicsEventType EventType
		, const EUproarMagnitude Magnitude
		, const EUproarSpeed Speed
	);
};

/** This Struct allows designers to associate types of Physics Events with Sounds. */
USTRUCT(BlueprintType)
struct UPROAR_API FUproarSoundDefinition : public FTableRowBase
{
	GENERATED_BODY()

public:

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsEvent)
	TEnumAsByte<EPhysicalSurface> SurfaceType = EPhysicalSurface::SurfaceType1;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsEvent)
	EUproarPhysicsEventType EventType = EUproarPhysicsEventType::BREAK;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsEvent)
	EUproarMagnitude Magnitude = EUproarMagnitude::TINY;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsEvent)
	EUproarSpeed Speed = EUproarSpeed::SLOW;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoundEvent)
	USoundBase* Sound = nullptr;
};
