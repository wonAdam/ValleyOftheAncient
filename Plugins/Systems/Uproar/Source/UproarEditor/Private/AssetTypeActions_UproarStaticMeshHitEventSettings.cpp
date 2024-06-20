// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_UproarStaticMeshHitEventSettings.h"

#include "UproarStaticMeshHitEventSettings.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_UproarStaticMeshHitEventSettings::GetSupportedClass() const
{
	return UUproarStaticMeshHitEventSettings::StaticClass();
}

const TArray<FText>& FAssetTypeActions_UproarStaticMeshHitEventSettings::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetUproarSubmenu", "Uproar")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE