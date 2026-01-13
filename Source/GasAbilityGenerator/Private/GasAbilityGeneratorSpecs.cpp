// GasAbilityGeneratorSpecs.cpp
// v4.0: Spec DataAssets for native UE workflow
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorSpecs.h"

//=============================================================================
// UNPCPackageSpec
//=============================================================================

bool UNPCPackageSpec::Validate(TArray<FString>& OutErrors) const
{
	// Required fields
	if (BaseName.IsEmpty())
	{
		OutErrors.Add(TEXT("BaseName is required"));
	}
	if (NPCId.IsEmpty())
	{
		OutErrors.Add(TEXT("NPCId is required"));
	}

	// Vendor validation
	if (bIsVendor && ShopName.IsEmpty())
	{
		OutErrors.Add(TEXT("Vendor enabled but ShopName is empty"));
	}

	// Schedule validation
	for (int32 i = 0; i < Schedule.Num(); i++)
	{
		const FSpecScheduleEntry& Entry = Schedule[i];
		if (Entry.GoalClass.IsNull())
		{
			OutErrors.Add(FString::Printf(TEXT("Schedule[%d]: GoalClass is required"), i));
		}
		if (Entry.StartTime >= Entry.EndTime && Entry.EndTime != 0)
		{
			// Allow wrap-around (e.g., 2200 to 600 next day)
			// Only error if both are in same range
		}
	}

	// Goals validation
	for (int32 i = 0; i < Goals.Num(); i++)
	{
		const FSpecGoalDefinition& Goal = Goals[i];
		if (Goal.Id.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("Goals[%d]: Id is required"), i));
		}
	}

	// Dialogue validation
	for (int32 i = 0; i < DialogueNodes.Num(); i++)
	{
		const FSpecDialogueNode& Node = DialogueNodes[i];
		if (Node.Id.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("DialogueNodes[%d]: Id is required"), i));
		}
		if (Node.Type != TEXT("npc") && Node.Type != TEXT("player"))
		{
			OutErrors.Add(FString::Printf(TEXT("DialogueNodes[%d]: Type must be 'npc' or 'player'"), i));
		}
	}

	// Quests validation
	for (int32 i = 0; i < Quests.Num(); i++)
	{
		const FSpecQuestDefinition& Quest = Quests[i];
		if (Quest.Id.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("Quests[%d]: Id is required"), i));
		}
	}

	return OutErrors.Num() == 0;
}

UNPCPackageSpec* UNPCPackageSpec::GetMergedSpec() const
{
	// If no parent, return copy of self
	if (ParentTemplate.IsNull())
	{
		return DuplicateObject(const_cast<UNPCPackageSpec*>(this), GetTransientPackage());
	}

	// Load parent
	UNPCPackageSpec* Parent = ParentTemplate.LoadSynchronous();
	if (!Parent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UNPCPackageSpec] Failed to load parent template: %s"),
			*ParentTemplate.ToString());
		return DuplicateObject(const_cast<UNPCPackageSpec*>(this), GetTransientPackage());
	}

	// Create merged copy starting from this spec
	UNPCPackageSpec* Merged = DuplicateObject(const_cast<UNPCPackageSpec*>(this), GetTransientPackage());

	// Merge arrays (use parent if child is empty)
	if (Merged->Abilities.Num() == 0) Merged->Abilities = Parent->Abilities;
	if (Merged->StartupEffects.Num() == 0) Merged->StartupEffects = Parent->StartupEffects;
	if (Merged->Activities.Num() == 0) Merged->Activities = Parent->Activities;
	if (Merged->GoalGenerators.Num() == 0) Merged->GoalGenerators = Parent->GoalGenerators;
	if (Merged->Goals.Num() == 0) Merged->Goals = Parent->Goals;
	if (Merged->Schedule.Num() == 0) Merged->Schedule = Parent->Schedule;
	if (Merged->DialogueNodes.Num() == 0) Merged->DialogueNodes = Parent->DialogueNodes;
	if (Merged->Quests.Num() == 0) Merged->Quests = Parent->Quests;
	if (Merged->ShopInventory.Num() == 0) Merged->ShopInventory = Parent->ShopInventory;

	// Merge single values (use parent if child not set)
	if (Merged->DefaultAttributes.IsNull()) Merged->DefaultAttributes = Parent->DefaultAttributes;
	if (Merged->NPCBlueprint.IsNull()) Merged->NPCBlueprint = Parent->NPCBlueprint;

	// Merge vendor config (use parent if child not vendor)
	if (!Merged->bIsVendor && Parent->bIsVendor)
	{
		Merged->bIsVendor = Parent->bIsVendor;
		Merged->ShopName = Parent->ShopName;
		Merged->TradingCurrency = Parent->TradingCurrency;
		Merged->BuyItemPercentage = Parent->BuyItemPercentage;
		Merged->SellItemPercentage = Parent->SellItemPercentage;
	}

	// Merge tags (combine parent + child)
	Merged->DefaultOwnedTags.AppendTags(Parent->DefaultOwnedTags);
	Merged->DefaultFactions.AppendTags(Parent->DefaultFactions);

	// Use parent values for default numeric fields
	if (Merged->MinLevel == 1 && Parent->MinLevel != 1) Merged->MinLevel = Parent->MinLevel;
	if (Merged->MaxLevel == 10 && Parent->MaxLevel != 10) Merged->MaxLevel = Parent->MaxLevel;
	if (FMath::IsNearlyEqual(Merged->AttackPriority, 0.5f) && !FMath::IsNearlyEqual(Parent->AttackPriority, 0.5f))
	{
		Merged->AttackPriority = Parent->AttackPriority;
	}

	// Inherit dialogue settings
	if (!Merged->bAutoCreateDialogue && Parent->bAutoCreateDialogue)
	{
		Merged->bAutoCreateDialogue = Parent->bAutoCreateDialogue;
	}
	if (!Merged->bAutoCreateTaggedDialogue && Parent->bAutoCreateTaggedDialogue)
	{
		Merged->bAutoCreateTaggedDialogue = Parent->bAutoCreateTaggedDialogue;
	}

	return Merged;
}

//=============================================================================
// UQuestSpec
//=============================================================================

bool UQuestSpec::Validate(TArray<FString>& OutErrors) const
{
	if (QuestName.IsEmpty())
	{
		OutErrors.Add(TEXT("QuestName is required"));
	}

	if (DisplayName.IsEmpty())
	{
		OutErrors.Add(TEXT("DisplayName is required"));
	}

	// At least one objective recommended
	if (Objectives.Num() == 0)
	{
		// This is a warning, not an error - quest can be manually configured
		UE_LOG(LogTemp, Warning, TEXT("[UQuestSpec] %s: No objectives defined"), *QuestName);
	}

	// Validate objectives
	for (int32 i = 0; i < Objectives.Num(); i++)
	{
		const FSpecQuestObjective& Obj = Objectives[i];
		// At least one task type should be set
		if (Obj.FindItem.IsNull() && Obj.GoToLocation.IsEmpty() && Obj.TalkToNPC.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("Objectives[%d]: Must have FindItem, GoToLocation, or TalkToNPC"), i));
		}
	}

	return OutErrors.Num() == 0;
}

//=============================================================================
// UItemSpec
//=============================================================================

bool UItemSpec::Validate(TArray<FString>& OutErrors) const
{
	if (ItemName.IsEmpty())
	{
		OutErrors.Add(TEXT("ItemName is required"));
	}

	if (DisplayName.IsEmpty())
	{
		OutErrors.Add(TEXT("DisplayName is required"));
	}

	// Warn about missing parent class but don't error (will default to EquippableItem)
	if (ParentClass.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UItemSpec] %s: ParentClass not set, will default to EquippableItem"), *ItemName);
	}

	// Validate equipment slot
	if (!EquipmentSlot.IsValid())
	{
		OutErrors.Add(TEXT("EquipmentSlot is required"));
	}

	// Validate stack settings
	if (bStackable && MaxStackSize < 2)
	{
		OutErrors.Add(TEXT("Stackable items must have MaxStackSize >= 2"));
	}

	return OutErrors.Num() == 0;
}
