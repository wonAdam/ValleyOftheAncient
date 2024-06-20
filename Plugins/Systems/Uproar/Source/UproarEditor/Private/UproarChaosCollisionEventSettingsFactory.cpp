// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarChaosCollisionEventSettingsFactory.h"

#include "Templates/SubclassOf.h"
#include "UproarChaosCollisionEventSettings.h"

class FFeedbackContext;
class UClass;
class UObject;

UUproarChaosCollisionEventSettingsFactory::UUproarChaosCollisionEventSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUproarChaosCollisionEventSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UUproarChaosCollisionEventSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UUproarChaosCollisionEventSettings>(InParent, Name, Flags);

}
