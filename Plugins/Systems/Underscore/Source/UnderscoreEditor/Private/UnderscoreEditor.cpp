// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderscoreEditor.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_UnderscoreCue.h"
#include "AssetTypeActions_UnderscoreSection.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

void FUnderscoreEditorModule::StartupModule()
{
	// Register the audio editor asset type actions.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_UnderscoreCue));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_UnderscoreSection));
}

void FUnderscoreEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IMPLEMENT_MODULE(FUnderscoreEditorModule, UnderscoreEditor)