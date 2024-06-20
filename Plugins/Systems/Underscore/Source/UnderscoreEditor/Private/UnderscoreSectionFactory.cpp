// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreSectionFactory.h"

#include "Templates/SubclassOf.h"
#include "UnderscoreSection.h"

class FFeedbackContext;
class UClass;
class UObject;

UUnderscoreSectionFactory::UUnderscoreSectionFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUnderscoreSection::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UUnderscoreSectionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UUnderscoreSection>(InParent, Name, Flags);

}