// GasAbilityGenerator - Dialogue Token Registry Implementation
// v4.4: Validated token system for Excel dialogue authoring

#include "XLSXSupport/DialogueTokenRegistry.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/DefaultValueHelper.h"

// Static empty set for invalid lookups
TSet<FString> FDialogueTokenRegistry::EmptySet;

FDialogueTokenRegistry& FDialogueTokenRegistry::Get()
{
	static FDialogueTokenRegistry Instance;
	return Instance;
}

FDialogueTokenRegistry::FDialogueTokenRegistry()
{
	RegisterBuiltInTokens();
}

void FDialogueTokenRegistry::RegisterTokenSpec(const FDialogueTokenSpec& Spec)
{
	// Check for duplicate
	for (const auto& Existing : Specs)
	{
		if (Existing.TokenName.Equals(Spec.TokenName, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Warning, TEXT("DialogueTokenRegistry: Token '%s' already registered, skipping"), *Spec.TokenName);
			return;
		}
	}
	Specs.Add(Spec);
}

void FDialogueTokenRegistry::RegisterBuiltInTokens()
{
	// ============================================
	// EVENTS (20 tokens)
	// ============================================

	// NE_BeginQuest(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_BeginQuest");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_BeginQuest");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_BeginQuest.NE_BeginQuest_C");
		Spec.DisplayName = TEXT("Begin Quest");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NE_GiveItem(ItemId, Count)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_GiveItem");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("BPE_AddItemToInventory");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_AddItemToInventory.BPE_AddItemToInventory_C");
		Spec.DisplayName = TEXT("Give Item");

		FDialogueTokenParam ItemParam;
		ItemParam.ParamName = TEXT("ItemId");
		ItemParam.Type = ETokenParamType::IdRef;
		ItemParam.bRequired = true;
		ItemParam.UEPropertyName = TEXT("Item");
		Spec.Params.Add(ItemParam);

		FDialogueTokenParam CountParam;
		CountParam.ParamName = TEXT("Count");
		CountParam.Type = ETokenParamType::Int;
		CountParam.bRequired = false;
		CountParam.UEPropertyName = TEXT("Quantity");
		CountParam.DefaultValue = TEXT("1");
		Spec.Params.Add(CountParam);

		RegisterTokenSpec(Spec);
	}

	// NE_RemoveItem(ItemId, Count)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_RemoveItem");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("BPE_ConsumeItem");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_ConsumeItem.BPE_ConsumeItem_C");
		Spec.DisplayName = TEXT("Remove Item");

		FDialogueTokenParam ItemParam;
		ItemParam.ParamName = TEXT("ItemId");
		ItemParam.Type = ETokenParamType::IdRef;
		ItemParam.bRequired = true;
		ItemParam.UEPropertyName = TEXT("Item");
		Spec.Params.Add(ItemParam);

		FDialogueTokenParam CountParam;
		CountParam.ParamName = TEXT("Count");
		CountParam.Type = ETokenParamType::Int;
		CountParam.bRequired = false;
		CountParam.UEPropertyName = TEXT("Quantity");
		CountParam.DefaultValue = TEXT("1");
		Spec.Params.Add(CountParam);

		RegisterTokenSpec(Spec);
	}

	// NE_AddTag(Tag)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_AddTag");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_AddGameplayTags");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_AddGameplayTags.NE_AddGameplayTags_C");
		Spec.DisplayName = TEXT("Add Gameplay Tag");

		FDialogueTokenParam TagParam;
		TagParam.ParamName = TEXT("Tag");
		TagParam.Type = ETokenParamType::String;  // Tags are strings like "State.Flag.TalkedToSeth"
		TagParam.bRequired = true;
		TagParam.UEPropertyName = TEXT("TagsToAdd");
		Spec.Params.Add(TagParam);

		RegisterTokenSpec(Spec);
	}

	// NE_RemoveTag(Tag)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_RemoveTag");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_RemoveGameplayTags");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_RemoveGameplayTags.NE_RemoveGameplayTags_C");
		Spec.DisplayName = TEXT("Remove Gameplay Tag");

		FDialogueTokenParam TagParam;
		TagParam.ParamName = TEXT("Tag");
		TagParam.Type = ETokenParamType::String;
		TagParam.bRequired = true;
		TagParam.UEPropertyName = TEXT("TagsToRemove");
		Spec.Params.Add(TagParam);

		RegisterTokenSpec(Spec);
	}

	// NE_GiveXP(Amount)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_GiveXP");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_GiveXP");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_GiveXP.NE_GiveXP_C");
		Spec.DisplayName = TEXT("Give XP");

		FDialogueTokenParam AmountParam;
		AmountParam.ParamName = TEXT("Amount");
		AmountParam.Type = ETokenParamType::Int;
		AmountParam.bRequired = true;
		AmountParam.UEPropertyName = TEXT("XPToGrant");
		Spec.Params.Add(AmountParam);

		RegisterTokenSpec(Spec);
	}

	// NE_AddCurrency(Amount)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_AddCurrency");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("BPE_AddCurrency");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_AddCurrency.BPE_AddCurrency_C");
		Spec.DisplayName = TEXT("Add Currency");

		FDialogueTokenParam AmountParam;
		AmountParam.ParamName = TEXT("Amount");
		AmountParam.Type = ETokenParamType::Int;
		AmountParam.bRequired = true;
		AmountParam.UEPropertyName = TEXT("CurrencyToAdd");
		Spec.Params.Add(AmountParam);

		RegisterTokenSpec(Spec);
	}

	// NE_BeginDialogue(DialogueId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_BeginDialogue");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_BeginDialogue");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_BeginDialogue.NE_BeginDialogue_C");
		Spec.DisplayName = TEXT("Begin Dialogue");

		FDialogueTokenParam DialogueParam;
		DialogueParam.ParamName = TEXT("DialogueId");
		DialogueParam.Type = ETokenParamType::IdRef;
		DialogueParam.bRequired = true;
		DialogueParam.UEPropertyName = TEXT("Dialogue");
		Spec.Params.Add(DialogueParam);

		RegisterTokenSpec(Spec);
	}

	// NE_RestartQuest(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_RestartQuest");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_RestartQuest");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_RestartQuest.NE_RestartQuest_C");
		Spec.DisplayName = TEXT("Restart Quest");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NE_StartTrading()
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_StartTrading");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("BPE_StartTrading");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_StartTrading.BPE_StartTrading_C");
		Spec.DisplayName = TEXT("Start Trading");
		// No parameters - trading with dialogue speaker

		RegisterTokenSpec(Spec);
	}

	// NE_AddSaveCheckpoint()
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_AddSaveCheckpoint");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_AddSaveCheckpoint");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_AddSaveCheckpoint.NE_AddSaveCheckpoint_C");
		Spec.DisplayName = TEXT("Save Checkpoint");
		// No parameters

		RegisterTokenSpec(Spec);
	}

	// NE_ShowNotification(Message)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_ShowNotification");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_ShowNotification");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_ShowNotification.NE_ShowNotification_C");
		Spec.DisplayName = TEXT("Show Notification");

		FDialogueTokenParam MessageParam;
		MessageParam.ParamName = TEXT("Message");
		MessageParam.Type = ETokenParamType::String;
		MessageParam.bRequired = true;
		MessageParam.UEPropertyName = TEXT("NotificationText");
		Spec.Params.Add(MessageParam);

		RegisterTokenSpec(Spec);
	}

	// NE_PrintString(Message) - for debugging
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_PrintString");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_PrintString");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_PrintString.NE_PrintString_C");
		Spec.DisplayName = TEXT("Print String");

		FDialogueTokenParam MessageParam;
		MessageParam.ParamName = TEXT("Message");
		MessageParam.Type = ETokenParamType::String;
		MessageParam.bRequired = true;
		MessageParam.UEPropertyName = TEXT("String");
		Spec.Params.Add(MessageParam);

		RegisterTokenSpec(Spec);
	}

	// NE_RemoveFollowGoal()
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_RemoveFollowGoal");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("BPE_RemoveFollowGoal");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_RemoveFollowGoal.BPE_RemoveFollowGoal_C");
		Spec.DisplayName = TEXT("Remove Follow Goal");
		// No parameters - removes follow goal from dialogue speaker

		RegisterTokenSpec(Spec);
	}

	// NE_AddFaction(Faction)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_AddFaction");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_AddFactions");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_AddFactions.NE_AddFactions_C");
		Spec.DisplayName = TEXT("Add Faction");

		FDialogueTokenParam FactionParam;
		FactionParam.ParamName = TEXT("Faction");
		FactionParam.Type = ETokenParamType::String;  // Faction tag like "Narrative.Factions.Friendly"
		FactionParam.bRequired = true;
		FactionParam.UEPropertyName = TEXT("FactionsToAdd");
		Spec.Params.Add(FactionParam);

		RegisterTokenSpec(Spec);
	}

	// NE_RemoveFaction(Faction)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_RemoveFaction");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_RemoveFactions");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_RemoveFactions.NE_RemoveFactions_C");
		Spec.DisplayName = TEXT("Remove Faction");

		FDialogueTokenParam FactionParam;
		FactionParam.ParamName = TEXT("Faction");
		FactionParam.Type = ETokenParamType::String;  // Faction tag
		FactionParam.bRequired = true;
		FactionParam.UEPropertyName = TEXT("FactionsToRemove");
		Spec.Params.Add(FactionParam);

		RegisterTokenSpec(Spec);
	}

	// NE_SetFactionAttitude(Faction, Attitude)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_SetFactionAttitude");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_SetFactionAttitude");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_SetFactionAttitude.NE_SetFactionAttitude_C");
		Spec.DisplayName = TEXT("Set Faction Attitude");

		FDialogueTokenParam FactionParam;
		FactionParam.ParamName = TEXT("Faction");
		FactionParam.Type = ETokenParamType::String;  // Faction tag
		FactionParam.bRequired = true;
		FactionParam.UEPropertyName = TEXT("Faction");
		Spec.Params.Add(FactionParam);

		FDialogueTokenParam AttitudeParam;
		AttitudeParam.ParamName = TEXT("Attitude");
		AttitudeParam.Type = ETokenParamType::Enum;
		AttitudeParam.bRequired = true;
		AttitudeParam.UEPropertyName = TEXT("Attitude");
		AttitudeParam.EnumValues.Add(TEXT("Friendly"));
		AttitudeParam.EnumValues.Add(TEXT("Neutral"));
		AttitudeParam.EnumValues.Add(TEXT("Hostile"));
		Spec.Params.Add(AttitudeParam);

		RegisterTokenSpec(Spec);
	}

	// NE_AddGoal(GoalClass) - C++ native event
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_AddGoal");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NarrativeEvent_AddGoalToNPC");
		Spec.UEClassPath = TEXT("/Script/NarrativeArsenal.NarrativeEvent_AddGoalToNPC");
		Spec.DisplayName = TEXT("Add Goal to NPC");

		FDialogueTokenParam GoalParam;
		GoalParam.ParamName = TEXT("GoalClass");
		GoalParam.Type = ETokenParamType::IdRef;  // Goal class reference
		GoalParam.bRequired = true;
		GoalParam.UEPropertyName = TEXT("GoalToAdd");
		Spec.Params.Add(GoalParam);

		RegisterTokenSpec(Spec);
	}

	// NE_FailQuest(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NE_FailQuest");
		Spec.Category = ETokenCategory::Event;
		Spec.UEClassName = TEXT("NE_FailQuest");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_FailQuest.NE_FailQuest_C");
		Spec.DisplayName = TEXT("Fail Quest");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// ============================================
	// CONDITIONS (21 tokens)
	// ============================================

	// NC_QuestState(QuestId, State)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_QuestState");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsQuestAtState");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsQuestAtState.NC_IsQuestAtState_C");
		Spec.DisplayName = TEXT("Quest State");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		FDialogueTokenParam StateParam;
		StateParam.ParamName = TEXT("State");
		StateParam.Type = ETokenParamType::String;  // State name like "Active", "Completed"
		StateParam.bRequired = true;
		StateParam.UEPropertyName = TEXT("QuestState");
		Spec.Params.Add(StateParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasItem(ItemId, MinCount)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasItem");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasItem");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasItem.BPC_HasItem_C");
		Spec.DisplayName = TEXT("Has Item");

		FDialogueTokenParam ItemParam;
		ItemParam.ParamName = TEXT("ItemId");
		ItemParam.Type = ETokenParamType::IdRef;
		ItemParam.bRequired = true;
		ItemParam.UEPropertyName = TEXT("Item");
		Spec.Params.Add(ItemParam);

		FDialogueTokenParam CountParam;
		CountParam.ParamName = TEXT("MinCount");
		CountParam.Type = ETokenParamType::Int;
		CountParam.bRequired = false;
		CountParam.UEPropertyName = TEXT("Quantity");
		CountParam.DefaultValue = TEXT("1");
		Spec.Params.Add(CountParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsQuestInProgress(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsQuestInProgress");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsQuestInProgress");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsQuestInProgress.NC_IsQuestInProgress_C");
		Spec.DisplayName = TEXT("Quest In Progress");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsQuestSucceeded(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsQuestSucceeded");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsQuestSucceeded");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsQuestSucceeded.NC_IsQuestSucceeded_C");
		Spec.DisplayName = TEXT("Quest Succeeded");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsQuestFailed(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsQuestFailed");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsQuestFailed");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsQuestFailed.NC_IsQuestFailed_C");
		Spec.DisplayName = TEXT("Quest Failed");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsQuestStartedOrFinished(QuestId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsQuestStartedOrFinished");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsQuestStartedOrFinished");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsQuestStartedOrFinished.NC_IsQuestStartedOrFinished_C");
		Spec.DisplayName = TEXT("Quest Started Or Finished");

		FDialogueTokenParam QuestParam;
		QuestParam.ParamName = TEXT("QuestId");
		QuestParam.Type = ETokenParamType::IdRef;
		QuestParam.bRequired = true;
		QuestParam.UEPropertyName = TEXT("Quest");
		Spec.Params.Add(QuestParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasDialogueNodePlayed(DialogueId, NodeId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasDialogueNodePlayed");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_HasDialogueNodePlayed");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_HasDialogueNodePlayed.NC_HasDialogueNodePlayed_C");
		Spec.DisplayName = TEXT("Dialogue Node Played");

		FDialogueTokenParam DialogueParam;
		DialogueParam.ParamName = TEXT("DialogueId");
		DialogueParam.Type = ETokenParamType::IdRef;
		DialogueParam.bRequired = true;
		DialogueParam.UEPropertyName = TEXT("Dialogue");
		Spec.Params.Add(DialogueParam);

		FDialogueTokenParam NodeParam;
		NodeParam.ParamName = TEXT("NodeId");
		NodeParam.Type = ETokenParamType::String;
		NodeParam.bRequired = true;
		NodeParam.UEPropertyName = TEXT("NodeID");
		Spec.Params.Add(NodeParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsDayTime()
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsDayTime");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsDayTime");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsDayTime.NC_IsDayTime_C");
		Spec.DisplayName = TEXT("Is Day Time");
		// No parameters

		RegisterTokenSpec(Spec);
	}

	// NC_LevelCheck(MinLevel)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_LevelCheck");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_LevelCheck");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_LevelCheck.BPC_LevelCheck_C");
		Spec.DisplayName = TEXT("Level Check");

		FDialogueTokenParam LevelParam;
		LevelParam.ParamName = TEXT("MinLevel");
		LevelParam.Type = ETokenParamType::Int;
		LevelParam.bRequired = true;
		LevelParam.UEPropertyName = TEXT("RequiredLevel");
		Spec.Params.Add(LevelParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasFollowGoalFor(NPCId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasFollowGoalFor");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasFollowGoalFor");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasFollowGoalFor.BPC_HasFollowGoalFor_C");
		Spec.DisplayName = TEXT("Has Follow Goal");

		FDialogueTokenParam NPCParam;
		NPCParam.ParamName = TEXT("NPCId");
		NPCParam.Type = ETokenParamType::IdRef;
		NPCParam.bRequired = true;
		NPCParam.UEPropertyName = TEXT("NPC");
		Spec.Params.Add(NPCParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasInteractGoalFor(InteractableId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasInteractGoalFor");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasInteractGoalFor");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasInteractGoalFor.BPC_HasInteractGoalFor_C");
		Spec.DisplayName = TEXT("Has Interact Goal");

		FDialogueTokenParam InteractableParam;
		InteractableParam.ParamName = TEXT("InteractableId");
		InteractableParam.Type = ETokenParamType::IdRef;
		InteractableParam.bRequired = true;
		InteractableParam.UEPropertyName = TEXT("Interactable");
		Spec.Params.Add(InteractableParam);

		RegisterTokenSpec(Spec);
	}

	// NC_DifficultyCheck(Difficulty)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_DifficultyCheck");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_DifficultyCheck");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_DifficultyCheck.BPC_DifficultyCheck_C");
		Spec.DisplayName = TEXT("Difficulty Check");

		FDialogueTokenParam DifficultyParam;
		DifficultyParam.ParamName = TEXT("Difficulty");
		DifficultyParam.Type = ETokenParamType::Enum;
		DifficultyParam.bRequired = true;
		DifficultyParam.UEPropertyName = TEXT("RequiredDifficulty");
		DifficultyParam.EnumValues.Add(TEXT("Easy"));
		DifficultyParam.EnumValues.Add(TEXT("Normal"));
		DifficultyParam.EnumValues.Add(TEXT("Hard"));
		DifficultyParam.EnumValues.Add(TEXT("Nightmare"));
		Spec.Params.Add(DifficultyParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasPerk(PerkId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasPerk");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasPerk");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasPerk.BPC_HasPerk_C");
		Spec.DisplayName = TEXT("Has Perk");

		FDialogueTokenParam PerkParam;
		PerkParam.ParamName = TEXT("PerkId");
		PerkParam.Type = ETokenParamType::IdRef;
		PerkParam.bRequired = true;
		PerkParam.UEPropertyName = TEXT("Perk");
		Spec.Params.Add(PerkParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasWeaponOut()
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasWeaponOut");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasWeaponOut");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasWeaponOut.BPC_HasWeaponOut_C");
		Spec.DisplayName = TEXT("Has Weapon Out");
		// No parameters

		RegisterTokenSpec(Spec);
	}

	// NC_IsItemEquipped(ItemId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsItemEquipped");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_IsItemEquipped");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_IsItemEquipped.BPC_IsItemEquipped_C");
		Spec.DisplayName = TEXT("Is Item Equipped");

		FDialogueTokenParam ItemParam;
		ItemParam.ParamName = TEXT("ItemId");
		ItemParam.Type = ETokenParamType::IdRef;
		ItemParam.bRequired = true;
		ItemParam.UEPropertyName = TEXT("Item");
		Spec.Params.Add(ItemParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsOccupying(InteractableId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsOccupying");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_IsOccupyingInteractable");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_IsOccupyingInteractable.BPC_IsOccupyingInteractable_C");
		Spec.DisplayName = TEXT("Is Occupying Interactable");

		FDialogueTokenParam InteractableParam;
		InteractableParam.ParamName = TEXT("InteractableId");
		InteractableParam.Type = ETokenParamType::IdRef;
		InteractableParam.bRequired = false;  // May check any interactable
		InteractableParam.UEPropertyName = TEXT("Interactable");
		Spec.Params.Add(InteractableParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasCompletedDataTask(TaskId)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasCompletedDataTask");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_HasCompletedDataTask");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_HasCompletedDataTask.NC_HasCompletedDataTask_C");
		Spec.DisplayName = TEXT("Has Completed Data Task");

		FDialogueTokenParam TaskParam;
		TaskParam.ParamName = TEXT("TaskId");
		TaskParam.Type = ETokenParamType::IdRef;
		TaskParam.bRequired = true;
		TaskParam.UEPropertyName = TEXT("DataTask");
		Spec.Params.Add(TaskParam);

		RegisterTokenSpec(Spec);
	}

	// NC_IsTimeInRange(StartTime, EndTime)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_IsTimeInRange");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("NC_IsTimeInRange");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/NC_IsTimeInRange.NC_IsTimeInRange_C");
		Spec.DisplayName = TEXT("Is Time In Range");

		FDialogueTokenParam StartParam;
		StartParam.ParamName = TEXT("StartTime");
		StartParam.Type = ETokenParamType::Int;  // 0-2400 format
		StartParam.bRequired = true;
		StartParam.UEPropertyName = TEXT("StartTime");
		Spec.Params.Add(StartParam);

		FDialogueTokenParam EndParam;
		EndParam.ParamName = TEXT("EndTime");
		EndParam.Type = ETokenParamType::Int;
		EndParam.bRequired = true;
		EndParam.UEPropertyName = TEXT("EndTime");
		Spec.Params.Add(EndParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasForm(FormId) - Character Creator form check
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasForm");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasForm");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/CharCreator/Blueprints/Conditions/BPC_HasForm.BPC_HasForm_C");
		Spec.DisplayName = TEXT("Has Form");

		FDialogueTokenParam FormParam;
		FormParam.ParamName = TEXT("FormId");
		FormParam.Type = ETokenParamType::IdRef;
		FormParam.bRequired = true;
		FormParam.UEPropertyName = TEXT("Form");
		Spec.Params.Add(FormParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasCurrency(MinAmount)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasCurrency");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasCurrency");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasCurrency.BPC_HasCurrency_C");
		Spec.DisplayName = TEXT("Has Currency");

		FDialogueTokenParam AmountParam;
		AmountParam.ParamName = TEXT("MinAmount");
		AmountParam.Type = ETokenParamType::Int;
		AmountParam.bRequired = true;
		AmountParam.UEPropertyName = TEXT("RequiredCurrency");
		Spec.Params.Add(AmountParam);

		RegisterTokenSpec(Spec);
	}

	// NC_HasGameplayTag(Tag)
	{
		FDialogueTokenSpec Spec;
		Spec.TokenName = TEXT("NC_HasGameplayTag");
		Spec.Category = ETokenCategory::Condition;
		Spec.UEClassName = TEXT("BPC_HasGameplayTag");
		Spec.UEClassPath = TEXT("/NarrativePro/Pro/Core/Tales/Conditions/BPC_HasGameplayTag.BPC_HasGameplayTag_C");
		Spec.DisplayName = TEXT("Has Gameplay Tag");

		FDialogueTokenParam TagParam;
		TagParam.ParamName = TEXT("Tag");
		TagParam.Type = ETokenParamType::String;  // Tag like "State.Flag.TalkedToNPC"
		TagParam.bRequired = true;
		TagParam.UEPropertyName = TEXT("Tag");
		Spec.Params.Add(TagParam);

		RegisterTokenSpec(Spec);
	}

	UE_LOG(LogTemp, Log, TEXT("DialogueTokenRegistry: Registered %d built-in tokens"), Specs.Num());
}

bool FDialogueTokenSpec::MatchesClass(UClass* Class) const
{
	if (!Class) return false;

	// Check class name match
	FString ClassName = Class->GetName();
	if (ClassName.Equals(UEClassName, ESearchCase::IgnoreCase))
	{
		return true;
	}

	// Check with _C suffix (Blueprint generated class)
	if (ClassName.Equals(UEClassName + TEXT("_C"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	return false;
}

const FDialogueTokenSpec* FDialogueTokenRegistry::FindByTokenName(const FString& TokenName) const
{
	for (const auto& Spec : Specs)
	{
		if (Spec.TokenName.Equals(TokenName, ESearchCase::IgnoreCase))
		{
			return &Spec;
		}
	}
	return nullptr;
}

const FDialogueTokenSpec* FDialogueTokenRegistry::FindByClass(UClass* Class) const
{
	if (!Class) return nullptr;

	for (const auto& Spec : Specs)
	{
		if (Spec.MatchesClass(Class))
		{
			return &Spec;
		}
	}
	return nullptr;
}

const FDialogueTokenSpec* FDialogueTokenRegistry::FindByClassName(const FString& ClassName) const
{
	for (const auto& Spec : Specs)
	{
		if (Spec.UEClassName.Equals(ClassName, ESearchCase::IgnoreCase))
		{
			return &Spec;
		}
	}
	return nullptr;
}

// ============================================
// PARSING
// ============================================

FTokenParseResult FDialogueTokenRegistry::ParseTokenString(const FString& TokenStr) const
{
	FTokenParseResult Result;
	Result.bSuccess = true;

	FString Trimmed = TokenStr.TrimStartAndEnd();

	// Empty string = unchanged (not an error)
	if (Trimmed.IsEmpty())
	{
		return Result;
	}

	// Split by semicolon
	TArray<FString> TokenParts;
	Trimmed.ParseIntoArray(TokenParts, TEXT(";"), true);

	for (const FString& Part : TokenParts)
	{
		FString TrimmedPart = Part.TrimStartAndEnd();
		if (TrimmedPart.IsEmpty()) continue;

		FString Error;
		FParsedToken Token = ParseSingleToken(TrimmedPart, Error);

		if (!Error.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = Error;
			return Result;
		}

		// v4.10: P3 - Validate token spec exists (early cross-reference validation)
		if (!Token.bIsClear)
		{
			const FDialogueTokenSpec* Spec = FindByTokenName(Token.TokenName);
			if (!Spec)
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Unknown token type: '%s'"), *Token.TokenName);
				return Result;
			}
		}

		Result.Tokens.Add(Token);
	}

	return Result;
}

FParsedToken FDialogueTokenRegistry::ParseSingleToken(const FString& TokenStr, FString& OutError) const
{
	FParsedToken Result;
	Result.RawString = TokenStr;

	FString Trimmed = TokenStr.TrimStartAndEnd();

	// Check for CLEAR token
	if (Trimmed.Equals(TEXT("CLEAR"), ESearchCase::IgnoreCase))
	{
		Result.bIsClear = true;
		Result.TokenName = TEXT("CLEAR");
		return Result;
	}

	// Parse: TokenName(Key=Value, Key2=Value2)
	int32 ParenOpen = Trimmed.Find(TEXT("("));
	int32 ParenClose = Trimmed.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (ParenOpen == INDEX_NONE)
	{
		// No parentheses - just token name with no params
		Result.TokenName = Trimmed;
		return Result;
	}

	if (ParenClose == INDEX_NONE || ParenClose < ParenOpen)
	{
		OutError = FString::Printf(TEXT("Malformed token: missing closing parenthesis in '%s'"), *TokenStr);
		return Result;
	}

	// Extract token name
	Result.TokenName = Trimmed.Left(ParenOpen).TrimStartAndEnd();

	if (Result.TokenName.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Malformed token: empty token name in '%s'"), *TokenStr);
		return Result;
	}

	// Extract parameters
	FString ParamsStr = Trimmed.Mid(ParenOpen + 1, ParenClose - ParenOpen - 1).TrimStartAndEnd();

	if (!ParamsStr.IsEmpty())
	{
		// Split by comma (but respect nested parentheses if any)
		TArray<FString> ParamParts;
		int32 Depth = 0;
		int32 LastSplit = 0;

		for (int32 i = 0; i < ParamsStr.Len(); i++)
		{
			TCHAR Ch = ParamsStr[i];
			if (Ch == TEXT('(')) Depth++;
			else if (Ch == TEXT(')')) Depth--;
			else if (Ch == TEXT(',') && Depth == 0)
			{
				ParamParts.Add(ParamsStr.Mid(LastSplit, i - LastSplit).TrimStartAndEnd());
				LastSplit = i + 1;
			}
		}
		// Add last part
		if (LastSplit < ParamsStr.Len())
		{
			ParamParts.Add(ParamsStr.Mid(LastSplit).TrimStartAndEnd());
		}

		// Parse each Key=Value pair
		for (const FString& ParamPart : ParamParts)
		{
			int32 EqPos = ParamPart.Find(TEXT("="));
			if (EqPos == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("Malformed parameter: expected Key=Value, got '%s' in '%s'"), *ParamPart, *TokenStr);
				return Result;
			}

			FString Key = ParamPart.Left(EqPos).TrimStartAndEnd();
			FString Value = ParamPart.Mid(EqPos + 1).TrimStartAndEnd();

			if (Key.IsEmpty())
			{
				OutError = FString::Printf(TEXT("Malformed parameter: empty key in '%s'"), *TokenStr);
				return Result;
			}

			Result.Params.Add(Key, Value);
		}
	}

	return Result;
}

// ============================================
// VALIDATION
// ============================================

bool FDialogueTokenRegistry::ValidateToken(const FParsedToken& Token, const FDialogueTokenSpec& Spec, FString& OutError) const
{
	// Check required parameters
	for (const auto& ParamDef : Spec.Params)
	{
		if (ParamDef.bRequired)
		{
			const FString* Value = Token.Params.Find(ParamDef.ParamName.ToString());
			if (!Value || Value->IsEmpty())
			{
				// Check if default is available
				if (ParamDef.DefaultValue.IsEmpty())
				{
					OutError = FString::Printf(TEXT("Missing required parameter '%s' in token '%s'"),
						*ParamDef.ParamName.ToString(), *Token.TokenName);
					return false;
				}
			}
		}
	}

	// Validate parameter types
	for (const auto& Pair : Token.Params)
	{
		// Find matching param def
		const FDialogueTokenParam* ParamDef = nullptr;
		for (const auto& P : Spec.Params)
		{
			if (P.ParamName.ToString().Equals(Pair.Key, ESearchCase::IgnoreCase))
			{
				ParamDef = &P;
				break;
			}
		}

		if (!ParamDef)
		{
			OutError = FString::Printf(TEXT("Unknown parameter '%s' in token '%s'"), *Pair.Key, *Token.TokenName);
			return false;
		}

		// Type-specific validation
		switch (ParamDef->Type)
		{
		case ETokenParamType::Bool:
			if (!Pair.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) &&
				!Pair.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				OutError = FString::Printf(TEXT("Parameter '%s' must be true/false, got '%s'"), *Pair.Key, *Pair.Value);
				return false;
			}
			break;

		case ETokenParamType::Int:
			if (!Pair.Value.IsNumeric())
			{
				OutError = FString::Printf(TEXT("Parameter '%s' must be integer, got '%s'"), *Pair.Key, *Pair.Value);
				return false;
			}
			break;

		case ETokenParamType::Float:
			{
				float Dummy;
				if (!FDefaultValueHelper::ParseFloat(Pair.Value, Dummy))
				{
					OutError = FString::Printf(TEXT("Parameter '%s' must be float, got '%s'"), *Pair.Key, *Pair.Value);
					return false;
				}
			}
			break;

		case ETokenParamType::Enum:
			{
				bool bFound = false;
				for (const FString& EnumVal : ParamDef->EnumValues)
				{
					if (EnumVal.Equals(Pair.Value, ESearchCase::IgnoreCase))
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					OutError = FString::Printf(TEXT("Parameter '%s' must be one of [%s], got '%s'"),
						*Pair.Key, *FString::Join(ParamDef->EnumValues, TEXT(", ")), *Pair.Value);
					return false;
				}
			}
			break;

		case ETokenParamType::IdRef:
			// IdRef validation is done separately with ValidateIdRef
			break;

		case ETokenParamType::String:
			// Any string is valid
			break;
		}
	}

	return true;
}

bool FDialogueTokenRegistry::ValidateIdRef(const FString& IdType, const FString& IdValue, const TSet<FString>& ValidIds) const
{
	if (ValidIds.Num() == 0)
	{
		// No validation set - accept any value (graceful degradation)
		return true;
	}

	return ValidIds.Contains(IdValue);
}

// ============================================
// SERIALIZATION (UObject -> Token String)
// ============================================

FString FDialogueTokenRegistry::SerializeEvent(UObject* Event) const
{
	if (!Event) return TEXT("");

	const FDialogueTokenSpec* Spec = FindByClass(Event->GetClass());
	if (!Spec)
	{
		// v4.23 FAIL-FAST: T1 - Manifest references event class not in token registry
		UE_LOG(LogTemp, Error, TEXT("[E_TOKEN_EVENT_UNSUPPORTED] SerializeEvent: Event class '%s' not registered in token registry"), *Event->GetClass()->GetName());
		return FString(); // Return empty to signal failure to caller
	}

	// Build token string
	TArray<FString> ParamStrings;
	for (const auto& ParamDef : Spec->Params)
	{
		FString Value = GetPropertyAsString(Event, ParamDef);
		if (!Value.IsEmpty())
		{
			ParamStrings.Add(FString::Printf(TEXT("%s=%s"), *ParamDef.ParamName.ToString(), *Value));
		}
	}

	if (ParamStrings.Num() > 0)
	{
		return FString::Printf(TEXT("%s(%s)"), *Spec->TokenName, *FString::Join(ParamStrings, TEXT(", ")));
	}
	else
	{
		return Spec->TokenName;
	}
}

FString FDialogueTokenRegistry::SerializeCondition(UObject* Condition) const
{
	// Same logic as events
	return SerializeEvent(Condition);
}

FString FDialogueTokenRegistry::SerializeEvents(const TArray<UObject*>& Events) const
{
	TArray<FString> TokenStrings;
	for (UObject* Event : Events)
	{
		if (Event)
		{
			TokenStrings.Add(SerializeEvent(Event));
		}
	}
	return FString::Join(TokenStrings, TEXT("; "));
}

FString FDialogueTokenRegistry::SerializeConditions(const TArray<UObject*>& Conditions) const
{
	TArray<FString> TokenStrings;
	for (UObject* Condition : Conditions)
	{
		if (Condition)
		{
			TokenStrings.Add(SerializeCondition(Condition));
		}
	}
	return FString::Join(TokenStrings, TEXT("; "));
}

// ============================================
// DESERIALIZATION (Token String -> UObjects)
// ============================================

FTokenDeserializeResult FDialogueTokenRegistry::DeserializeEvents(const FString& TokenStr, UObject* Outer) const
{
	FTokenDeserializeResult Result;

	// Parse tokens
	FTokenParseResult ParseResult = ParseTokenString(TokenStr);
	if (!ParseResult.bSuccess)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = ParseResult.ErrorMessage;
		return Result;
	}

	// Empty string = unchanged
	if (ParseResult.Tokens.Num() == 0)
	{
		Result.bSuccess = true;
		return Result;
	}

	// Process tokens
	for (const FParsedToken& Token : ParseResult.Tokens)
	{
		// Handle CLEAR
		if (Token.bIsClear)
		{
			Result.bShouldClear = true;
			continue;
		}

		// Find spec
		const FDialogueTokenSpec* Spec = FindByTokenName(Token.TokenName);
		if (!Spec)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Unknown event token: '%s'"), *Token.TokenName);
			return Result;
		}

		if (Spec->Category != ETokenCategory::Event)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Token '%s' is not an event"), *Token.TokenName);
			return Result;
		}

		// Validate
		FString ValidationError;
		if (!ValidateToken(Token, *Spec, ValidationError))
		{
			Result.bSuccess = false;
			Result.ErrorMessage = ValidationError;
			return Result;
		}

		// Instantiate
		FString InstantiateError;
		UObject* EventObj = InstantiateFromSpec(*Spec, Token, Outer, InstantiateError);
		if (!EventObj)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = InstantiateError;
			return Result;
		}

		Result.Objects.Add(EventObj);
	}

	Result.bSuccess = true;
	return Result;
}

FTokenDeserializeResult FDialogueTokenRegistry::DeserializeConditions(const FString& TokenStr, UObject* Outer) const
{
	FTokenDeserializeResult Result;

	// Parse tokens
	FTokenParseResult ParseResult = ParseTokenString(TokenStr);
	if (!ParseResult.bSuccess)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = ParseResult.ErrorMessage;
		return Result;
	}

	// Empty string = unchanged
	if (ParseResult.Tokens.Num() == 0)
	{
		Result.bSuccess = true;
		return Result;
	}

	// Process tokens
	for (const FParsedToken& Token : ParseResult.Tokens)
	{
		// Handle CLEAR
		if (Token.bIsClear)
		{
			Result.bShouldClear = true;
			continue;
		}

		// Find spec
		const FDialogueTokenSpec* Spec = FindByTokenName(Token.TokenName);
		if (!Spec)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Unknown condition token: '%s'"), *Token.TokenName);
			return Result;
		}

		if (Spec->Category != ETokenCategory::Condition)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Token '%s' is not a condition"), *Token.TokenName);
			return Result;
		}

		// Validate
		FString ValidationError;
		if (!ValidateToken(Token, *Spec, ValidationError))
		{
			Result.bSuccess = false;
			Result.ErrorMessage = ValidationError;
			return Result;
		}

		// Instantiate
		FString InstantiateError;
		UObject* ConditionObj = InstantiateFromSpec(*Spec, Token, Outer, InstantiateError);
		if (!ConditionObj)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = InstantiateError;
			return Result;
		}

		Result.Objects.Add(ConditionObj);
	}

	Result.bSuccess = true;
	return Result;
}

// ============================================
// ID LISTS
// ============================================

void FDialogueTokenRegistry::SetValidIds(const FString& IdType, const TSet<FString>& Ids)
{
	ValidIdSets.Add(IdType, Ids);
}

const TSet<FString>& FDialogueTokenRegistry::GetValidIds(const FString& IdType) const
{
	const TSet<FString>* Found = ValidIdSets.Find(IdType);
	return Found ? *Found : EmptySet;
}

void FDialogueTokenRegistry::ClearValidIds()
{
	ValidIdSets.Empty();
}

// ============================================
// INTERNAL HELPERS
// ============================================

UObject* FDialogueTokenRegistry::InstantiateFromSpec(const FDialogueTokenSpec& Spec, const FParsedToken& Token, UObject* Outer, FString& OutError) const
{
	// Try to load the class
	UClass* Class = LoadClass<UObject>(nullptr, *Spec.UEClassPath);
	if (!Class)
	{
		// Try without full path - might be a native class
		// In UE5, use nullptr instead of deprecated ANY_PACKAGE
		Class = FindFirstObject<UClass>(*Spec.UEClassName, EFindFirstObjectOptions::None);
	}

	if (!Class)
	{
		OutError = FString::Printf(TEXT("Could not find class '%s' for token '%s'"), *Spec.UEClassName, *Token.TokenName);
		return nullptr;
	}

	// Create instance
	UObject* Instance = NewObject<UObject>(Outer ? Outer : GetTransientPackage(), Class);
	if (!Instance)
	{
		OutError = FString::Printf(TEXT("Failed to create instance of '%s'"), *Spec.UEClassName);
		return nullptr;
	}

	// Set properties from token params
	for (const auto& ParamDef : Spec.Params)
	{
		const FString* Value = Token.Params.Find(ParamDef.ParamName.ToString());
		FString ActualValue = Value ? *Value : ParamDef.DefaultValue;

		if (!ActualValue.IsEmpty())
		{
			FString PropError;
			if (!SetPropertyFromParam(Instance, ParamDef, ActualValue, PropError))
			{
				// Log warning but don't fail - property might not exist or have different name
				UE_LOG(LogTemp, Warning, TEXT("DialogueTokenRegistry: %s"), *PropError);
			}
		}
	}

	return Instance;
}

bool FDialogueTokenRegistry::SetPropertyFromParam(UObject* Object, const FDialogueTokenParam& ParamDef, const FString& Value, FString& OutError) const
{
	if (!Object) return false;

	FProperty* Prop = Object->GetClass()->FindPropertyByName(ParamDef.UEPropertyName);
	if (!Prop)
	{
		OutError = FString::Printf(TEXT("Property '%s' not found on class '%s'"),
			*ParamDef.UEPropertyName.ToString(), *Object->GetClass()->GetName());
		return false;
	}

	void* PropAddr = Prop->ContainerPtrToValuePtr<void>(Object);

	// Handle different property types
	if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		IntProp->SetPropertyValue(PropAddr, FCString::Atoi(*Value));
		return true;
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		FloatProp->SetPropertyValue(PropAddr, FCString::Atof(*Value));
		return true;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
	{
		BoolProp->SetPropertyValue(PropAddr, Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
		return true;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
	{
		StrProp->SetPropertyValue(PropAddr, Value);
		return true;
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
	{
		NameProp->SetPropertyValue(PropAddr, FName(*Value));
		return true;
	}
	else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Prop))
	{
		// For IdRef types, the Value is an asset ID that needs to be resolved
		// For now, store as soft object path - will be resolved later
		FSoftObjectPtr& SoftPtr = *static_cast<FSoftObjectPtr*>(PropAddr);
		// Try to find the asset by name in common paths
		// This is a simplified approach - full implementation would search asset registry
		SoftPtr = FSoftObjectPath(Value);
		return true;
	}
	else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Prop))
	{
		FSoftObjectPtr& SoftPtr = *static_cast<FSoftObjectPtr*>(PropAddr);
		SoftPtr = FSoftObjectPath(Value);
		return true;
	}
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
	{
		// Try to load the object by path
		UObject* LoadedObj = LoadObject<UObject>(nullptr, *Value);
		if (LoadedObj)
		{
			ObjProp->SetObjectPropertyValue(PropAddr, LoadedObj);
			return true;
		}
		else
		{
			OutError = FString::Printf(TEXT("Could not load object '%s' for property '%s'"), *Value, *ParamDef.UEPropertyName.ToString());
			return false;
		}
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		// Handle FGameplayTagContainer for tags
		if (StructProp->Struct->GetName() == TEXT("GameplayTagContainer"))
		{
			FGameplayTagContainer* TagContainer = static_cast<FGameplayTagContainer*>(PropAddr);
			TagContainer->AddTag(FGameplayTag::RequestGameplayTag(FName(*Value)));
			return true;
		}
	}

	// v4.23 FAIL-FAST: T2 - Property type not supported by token registry
	OutError = FString::Printf(TEXT("[E_TOKEN_PROPERTY_UNSUPPORTED] Unsupported property type for '%s'"), *ParamDef.UEPropertyName.ToString());
	return false;
}

FString FDialogueTokenRegistry::GetPropertyAsString(UObject* Object, const FDialogueTokenParam& ParamDef) const
{
	if (!Object) return TEXT("");

	FProperty* Prop = Object->GetClass()->FindPropertyByName(ParamDef.UEPropertyName);
	if (!Prop) return TEXT("");

	void* PropAddr = Prop->ContainerPtrToValuePtr<void>(Object);

	// Handle different property types
	if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		return FString::FromInt(IntProp->GetPropertyValue(PropAddr));
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
	{
		return FString::SanitizeFloat(FloatProp->GetPropertyValue(PropAddr));
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
	{
		return BoolProp->GetPropertyValue(PropAddr) ? TEXT("true") : TEXT("false");
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
	{
		return StrProp->GetPropertyValue(PropAddr);
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
	{
		return NameProp->GetPropertyValue(PropAddr).ToString();
	}
	else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Prop))
	{
		const FSoftObjectPtr& SoftPtr = *static_cast<const FSoftObjectPtr*>(PropAddr);
		// Return just the asset name, not full path
		FString AssetPath = SoftPtr.ToString();
		int32 LastDot = AssetPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (LastDot != INDEX_NONE)
		{
			return AssetPath.Mid(LastDot + 1);
		}
		return AssetPath;
	}
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
	{
		UObject* ObjValue = ObjProp->GetObjectPropertyValue(PropAddr);
		return ObjValue ? ObjValue->GetName() : TEXT("");
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		// Handle FGameplayTagContainer
		if (StructProp->Struct->GetName() == TEXT("GameplayTagContainer"))
		{
			const FGameplayTagContainer* TagContainer = static_cast<const FGameplayTagContainer*>(PropAddr);
			TArray<FString> TagStrings;
			for (const FGameplayTag& Tag : *TagContainer)
			{
				TagStrings.Add(Tag.ToString());
			}
			return FString::Join(TagStrings, TEXT(","));
		}
	}

	return TEXT("");
}
