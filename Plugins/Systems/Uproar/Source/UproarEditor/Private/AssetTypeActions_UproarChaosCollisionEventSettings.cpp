// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_UproarChaosCollisionEventSettings.h"

#include "UproarChaosCollisionEventSettings.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_UproarChaosCollisionEventSettings::GetSupportedClass() const
{
	return UUproarChaosCollisionEventSettings::StaticClass();
}

const TArray<FText>& FAssetTypeActions_UproarChaosCollisionEventSettings::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetUproarSubmenu", "Uproar")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE