// Copyright Epic Games, Inc. All Rights Reserved.

#include "Uproar.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FUproarModule"

void FUproarModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUproarModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
DEFINE_LOG_CATEGORY(LogUproar);

IMPLEMENT_MODULE(FUproarModule, Uproar)

