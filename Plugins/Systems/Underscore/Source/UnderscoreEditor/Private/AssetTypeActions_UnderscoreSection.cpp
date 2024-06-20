// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_UnderscoreSection.h"

#include "UnderscoreSection.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_UnderscoreSection::GetSupportedClass() const
{
	return UUnderscoreSection::StaticClass();
}

const TArray<FText>& FAssetTypeActions_UnderscoreSection::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetUnderscoreSubMenu", "Underscore")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE