// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_UnderscoreCue.h"

#include "UnderscoreCue.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_UnderscoreCue::GetSupportedClass() const
{
	return UUnderscoreCue::StaticClass();
}

const TArray<FText>& FAssetTypeActions_UnderscoreCue::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetUnderscoreSubMenu", "Underscore")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE