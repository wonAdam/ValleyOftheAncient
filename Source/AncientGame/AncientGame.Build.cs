// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AncientGame : ModuleRules
{
	public AncientGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"AncientGame"
			}
		);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"ModularGameplayActors",
			"EnhancedInput",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"GameFeatures",
			"RHI",
			"RenderCore",
			"SlateCore",
			"NetCore",
			"ModularGameplay"
		});
	}
}
