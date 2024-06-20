// Copyright Epic Games, Inc. All Rights Reserved.

#include "InstanceLevelCollision.h"
#include "EditorUtilityWidget.h"
#include "WidgetBlueprint.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"
#include "InstanceLevelCollisionCommands.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FInstanceLevelCollisionModule"

void FInstanceLevelCollisionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	InstanceLevelCollisionCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		InstanceLevelCollisionCommands::Get().InstanceLevelCollisionWidget,
		FExecuteAction::CreateRaw(this, &FInstanceLevelCollisionModule::LevelInstanceCollisionMenuUI)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> NewMenuExtender = MakeShareable(new FExtender);
		NewMenuExtender->AddMenuExtension("LevelEditor",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FInstanceLevelCollisionModule::AddMenuEntry));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(NewMenuExtender);
	}
}

void FInstanceLevelCollisionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	InstanceLevelCollisionCommands::Unregister();
}

void FInstanceLevelCollisionModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("CustomMenu", TAttribute<FText>(FText::FromString("LevelInstance Collision Tool")));

	MenuBuilder.AddMenuEntry(InstanceLevelCollisionCommands::Get().InstanceLevelCollisionWidget);

	MenuBuilder.EndSection();
}

void FInstanceLevelCollisionModule::LevelInstanceCollisionMenuUI()
{

	const FString BlueprintRef = "EditorUtilityWidgetBlueprint'/InstanceLevelCollision/Widget_ColliderMaker.Widget_ColliderMaker'";
	FSoftObjectPath BlueprintPath = FSoftObjectPath(BlueprintRef);
	UObject* BlueprintObject = BlueprintPath.TryLoad();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(BlueprintObject);
	UEditorUtilityWidgetBlueprint* EditorWidget = (UEditorUtilityWidgetBlueprint*)Blueprint;
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	EditorUtilitySubsystem->SpawnAndRegisterTab(EditorWidget);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInstanceLevelCollisionModule, InstanceLevelCollision)