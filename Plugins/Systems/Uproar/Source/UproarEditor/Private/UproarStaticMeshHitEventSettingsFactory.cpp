// Copyright Epic Games, Inc. All Rights Reserved.


#include "UproarStaticMeshHitEventSettingsFactory.h"

#include "Templates/SubclassOf.h"
#include "UproarStaticMeshHitEventSettings.h"

class FFeedbackContext;
class UClass;
class UObject;

UUproarStaticMeshHitEventSettingsFactory::UUproarStaticMeshHitEventSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUproarStaticMeshHitEventSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UUproarStaticMeshHitEventSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UUproarStaticMeshHitEventSettings>(InParent, Name, Flags);

}

