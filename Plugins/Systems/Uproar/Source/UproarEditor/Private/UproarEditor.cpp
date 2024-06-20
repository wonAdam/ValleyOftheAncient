// Copyright Epic Games, Inc. All Rights Reserved.

#include "UproarEditor.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_UproarChaosBreakEventSettings.h"
#include "AssetTypeActions_UproarChaosCollisionEventSettings.h"
#include "AssetTypeActions_UproarStaticMeshHitEventSettings.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"


#define LOCTEXT_NAMESPACE "FUproarEditorModule"

void FUproarEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Register the Uproar Editor asset type actions.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_UproarChaosCollisionEventSettings));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_UproarChaosBreakEventSettings));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_UproarStaticMeshHitEventSettings));

}

void FUproarEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUproarEditorModule, UproarEditor)