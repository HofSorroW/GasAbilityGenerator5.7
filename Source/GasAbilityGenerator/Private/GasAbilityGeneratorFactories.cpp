// GasAbilityGeneratorFactories.cpp
// v4.0: Asset factories for Spec DataAssets
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorFactories.h"
#include "GasAbilityGeneratorSpecs.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

//=============================================================================
// NPC Package Spec Factory
//=============================================================================

UNPCPackageSpecFactory::UNPCPackageSpecFactory()
{
	SupportedClass = UNPCPackageSpec::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UNPCPackageSpecFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UNPCPackageSpec>(InParent, Class, Name, Flags);
}

FText UNPCPackageSpecFactory::GetDisplayName() const
{
	return LOCTEXT("NPCPackageSpec", "NPC Package Spec");
}

uint32 UNPCPackageSpecFactory::GetMenuCategories() const
{
	// Use GasAbilityGenerator category if registered, otherwise Miscellaneous
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type Category = AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
	if (Category != EAssetTypeCategories::None)
	{
		return Category;
	}
	return EAssetTypeCategories::Misc;
}

//=============================================================================
// Quest Spec Factory
//=============================================================================

UQuestSpecFactory::UQuestSpecFactory()
{
	SupportedClass = UQuestSpec::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UQuestSpecFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UQuestSpec>(InParent, Class, Name, Flags);
}

FText UQuestSpecFactory::GetDisplayName() const
{
	return LOCTEXT("QuestSpec", "Quest Spec");
}

uint32 UQuestSpecFactory::GetMenuCategories() const
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type Category = AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
	if (Category != EAssetTypeCategories::None)
	{
		return Category;
	}
	return EAssetTypeCategories::Misc;
}

//=============================================================================
// Item Spec Factory
//=============================================================================

UItemSpecFactory::UItemSpecFactory()
{
	SupportedClass = UItemSpec::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UItemSpecFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UItemSpec>(InParent, Class, Name, Flags);
}

FText UItemSpecFactory::GetDisplayName() const
{
	return LOCTEXT("ItemSpec", "Item Spec");
}

uint32 UItemSpecFactory::GetMenuCategories() const
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type Category = AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
	if (Category != EAssetTypeCategories::None)
	{
		return Category;
	}
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE
