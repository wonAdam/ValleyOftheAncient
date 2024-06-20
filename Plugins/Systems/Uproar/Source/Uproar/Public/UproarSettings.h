// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "HAL/Platform.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "UObject/NameTypes.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

#include "UproarSettings.generated.h"

class UObject;
struct FPropertyChangedEvent;

// class UDataTable;

/**
* Game Settings which allow the user to set Default Mixes which activate/deactivate with the Crossfader Subsystem
* as well as define default MixStateBanks which are added to the Subsystem main bank on initialization.
*/
UCLASS(config = Uproar, defaultconfig, meta = (DisplayName = "Uproar"))
class UPROAR_API UUproarProjectSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	// The total grid size in one dimension (total grid space will be this value cubed around the Listener)
	// The Grid Size divided by the Cell Size cubed should be under the maximum value of 32-bit integers
	UPROPERTY(config, EditAnywhere, meta = (ClampMin = "1000.0", ClampMax = "20000.0", UIMin = "1000.0", UIMax = "20000.0"))
	float UproarSpatialGridSize = 20000.0f;

	// The UUnit size of an individual grid hash cell, the smaller this value, the more grid cells, the more sounds can play
	// in the same space. (Grid Size / Grid Cell Size) cubed should be smaller than the maximum value of 32-bit integers
	UPROPERTY(config, EditAnywhere, meta = (ClampMin = "20.0", ClampMax = "10000.0", UIMin = "20.0", UIMax = "10000.0"))
	float UproarSpatialGridCellSize = 75.0f;

	// The lifetime of a single grid cell
	UPROPERTY(config, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float UproarSoundEventLifespanSeconds = 1.25f;

	// Sound Definition Library
	UPROPERTY(config, EditAnywhere, meta = (AllowedClasses = "/Script/Engine.DataTable"))
	FSoftObjectPath UproarSoundDefinition;

	// Whether or not a debug visualization of grid hash cells should draw when sounds play in them
	UPROPERTY(config, EditAnywhere)
	bool bDrawDebugCells = false;

public:

	// Beginning of UDeveloperSettings Interface
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("UproarPlugin", "UproarSettingsSection", "Uproar"); };
#endif
	// End of UDeveloperSettings Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


};
