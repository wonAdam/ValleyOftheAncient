// Copyright Epic Games, Inc. All Rights Reserved.


#include "MixStateBankFactory.h"

#include "MixStateBank.h"
#include "Templates/SubclassOf.h"

class FFeedbackContext;
class UClass;
class UObject;

UMixStateBankFactory::UMixStateBankFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMixStateBank::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UMixStateBankFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMixStateBank>(InParent, Name, Flags);

}