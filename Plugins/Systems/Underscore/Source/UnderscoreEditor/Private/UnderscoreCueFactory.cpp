// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreCueFactory.h"

#include "Templates/SubclassOf.h"
#include "UnderscoreCue.h"

class FFeedbackContext;
class UClass;
class UObject;

UUnderscoreCueFactory::UUnderscoreCueFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUnderscoreCue::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UUnderscoreCueFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UUnderscoreCue>(InParent, Name, Flags);

}