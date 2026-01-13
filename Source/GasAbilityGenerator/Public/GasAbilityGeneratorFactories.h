// GasAbilityGeneratorFactories.h
// v4.0: Asset factories for Spec DataAssets
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "GasAbilityGeneratorFactories.generated.h"

/**
 * Factory for creating NPC Package Spec assets
 * Creates in Content Browser: Right-click -> Miscellaneous -> NPC Package Spec
 */
UCLASS()
class GASABILITYGENERATOR_API UNPCPackageSpecFactory : public UFactory
{
	GENERATED_BODY()

public:
	UNPCPackageSpecFactory();

	virtual UObject* FactoryCreateNew(
		UClass* Class,
		UObject* InParent,
		FName Name,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

/**
 * Factory for creating Quest Spec assets
 */
UCLASS()
class GASABILITYGENERATOR_API UQuestSpecFactory : public UFactory
{
	GENERATED_BODY()

public:
	UQuestSpecFactory();

	virtual UObject* FactoryCreateNew(
		UClass* Class,
		UObject* InParent,
		FName Name,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

/**
 * Factory for creating Item Spec assets
 */
UCLASS()
class GASABILITYGENERATOR_API UItemSpecFactory : public UFactory
{
	GENERATED_BODY()

public:
	UItemSpecFactory();

	virtual UObject* FactoryCreateNew(
		UClass* Class,
		UObject* InParent,
		FName Name,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};
