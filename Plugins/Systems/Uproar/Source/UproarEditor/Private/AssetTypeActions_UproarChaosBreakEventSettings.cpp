// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_UproarChaosBreakEventSettings.h"

#include "UproarChaosBreakEventSettings.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_UproarChaosBreakEventSettings::GetSupportedClass() const
{
	return UUproarChaosBreakEventSettings::StaticClass();
}

const TArray<FText>& FAssetTypeActions_UproarChaosBreakEventSettings::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetUproarSubmenu", "Uproar")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE