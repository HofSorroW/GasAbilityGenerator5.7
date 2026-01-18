// GasAbilityGenerator - Quest Table Converter
// v4.8: Implementation

#include "QuestTableConverter.h"

TArray<FManifestQuestDefinition> FQuestTableConverter::ConvertRowsToManifest(
	const TArray<FQuestTableRow>& Rows,
	const FString& OutputFolder)
{
	TArray<FManifestQuestDefinition> Definitions;

	// Group rows by QuestName
	TMap<FString, TArray<const FQuestTableRow*>> QuestGroups;
	for (const FQuestTableRow& Row : Rows)
	{
		if (!Row.bDeleted && !Row.QuestName.IsEmpty())
		{
			QuestGroups.FindOrAdd(Row.QuestName).Add(&Row);
		}
	}

	// Convert each quest group
	for (const auto& Pair : QuestGroups)
	{
		FManifestQuestDefinition Def = ConvertQuestToDefinition(Pair.Key, Pair.Value, OutputFolder);
		ApplyDefaults(Def);
		Definitions.Add(Def);
	}

	return Definitions;
}

FManifestQuestDefinition FQuestTableConverter::ConvertQuestToDefinition(
	const FString& QuestName,
	const TArray<const FQuestTableRow*>& Rows,
	const FString& OutputFolder)
{
	FManifestQuestDefinition Def;
	Def.Name = FString::Printf(TEXT("Quest_%s"), *QuestName);
	Def.Folder = OutputFolder;

	// Get quest display name from first row
	if (Rows.Num() > 0)
	{
		Def.QuestName = Rows[0]->DisplayName.IsEmpty() ? QuestName : Rows[0]->DisplayName;
	}

	// Convert each row to a state
	for (const FQuestTableRow* Row : Rows)
	{
		FManifestQuestStateDefinition State = ConvertRowToState(*Row);
		Def.States.Add(State);

		// Set start state to first Regular state or explicit "Start" state
		if (Def.StartState.IsEmpty())
		{
			if (Row->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase) ||
				Row->StateType == EQuestStateType::Regular)
			{
				Def.StartState = Row->StateID;
			}
		}

		// Check for quest description
		if (Def.QuestDescription.IsEmpty() && !Row->Description.IsEmpty())
		{
			Def.QuestDescription = Row->Description;
		}

		// Get rewards from success states (quest-level rewards)
		if (Row->StateType == EQuestStateType::Success && !Row->Rewards.IsEmpty())
		{
			Def.Rewards = ParseRewards(Row->Rewards);
		}
	}

	return Def;
}

FManifestQuestStateDefinition FQuestTableConverter::ConvertRowToState(const FQuestTableRow& Row)
{
	FManifestQuestStateDefinition State;
	State.Id = Row.StateID;
	State.Description = Row.Description;
	State.Type = GetStateTypeString(Row.StateType);

	// Create branch from this state if it has a parent
	if (!Row.ParentBranch.IsEmpty())
	{
		FManifestQuestBranchDefinition Branch;
		Branch.Id = FString::Printf(TEXT("Branch_%s_to_%s"), *Row.ParentBranch, *Row.StateID);
		Branch.DestinationState = Row.StateID;

		// Parse tasks
		Branch.Tasks = ParseTasks(Row.Tasks);

		State.Branches.Add(Branch);
	}

	// Parse events for this state
	State.Events = ParseEvents(Row.Events);

	// Note: Conditions and Rewards are parsed for validation but stored differently:
	// - Conditions go on dialogue nodes, not quest states directly
	// - Rewards go on the quest definition, populated in ConvertQuestToDefinition

	return State;
}

TArray<FManifestQuestTaskDefinition> FQuestTableConverter::ParseTasks(const FString& TasksToken)
{
	TArray<FManifestQuestTaskDefinition> Tasks;
	if (TasksToken.IsEmpty()) return Tasks;

	// Split by semicolon: BPT_FindItem(Item=EI_Ore,Count=10);BPT_Move(Location=POI_Forge)
	TArray<FString> TaskTokens;
	TasksToken.ParseIntoArray(TaskTokens, TEXT(";"), true);

	for (const FString& Token : TaskTokens)
	{
		FString TrimmedToken = Token.TrimStartAndEnd();
		if (TrimmedToken.IsEmpty()) continue;

		FManifestQuestTaskDefinition Task;

		// Extract task class and params: BPT_FindItem(Item=EI_Ore,Count=10)
		int32 ParenStart = TrimmedToken.Find(TEXT("("));
		if (ParenStart != INDEX_NONE)
		{
			Task.TaskClass = TrimmedToken.Left(ParenStart).TrimStartAndEnd();

			int32 ParenEnd = TrimmedToken.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (ParenEnd > ParenStart)
			{
				FString ParamsStr = TrimmedToken.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
				TMap<FString, FString> Params = ParseTokenParams(ParamsStr);

				// Common task parameters - map to Argument field
				// Priority: Item > Location > NPC > Dialogue
				if (Params.Contains(TEXT("Item")))
				{
					Task.Argument = Params[TEXT("Item")];
				}
				else if (Params.Contains(TEXT("Location")))
				{
					Task.Argument = Params[TEXT("Location")];
				}
				else if (Params.Contains(TEXT("NPC")))
				{
					Task.Argument = Params[TEXT("NPC")];
				}
				else if (Params.Contains(TEXT("Dialogue")))
				{
					Task.Argument = Params[TEXT("Dialogue")];
				}

				// Quantity/Count
				if (Params.Contains(TEXT("Count")))
				{
					Task.Quantity = FCString::Atoi(*Params[TEXT("Count")]);
				}
				else if (Params.Contains(TEXT("Quantity")))
				{
					Task.Quantity = FCString::Atoi(*Params[TEXT("Quantity")]);
				}
			}
		}
		else
		{
			Task.TaskClass = TrimmedToken;
		}

		if (!Task.TaskClass.IsEmpty())
		{
			Tasks.Add(Task);
		}
	}

	return Tasks;
}

TArray<FManifestDialogueEventDefinition> FQuestTableConverter::ParseEvents(const FString& EventsToken)
{
	TArray<FManifestDialogueEventDefinition> Events;
	if (EventsToken.IsEmpty()) return Events;

	// Split by semicolon: NE_GiveXP(Amount=50);NE_AddItem(Item=EI_Sword)
	TArray<FString> EventTokens;
	EventsToken.ParseIntoArray(EventTokens, TEXT(";"), true);

	for (const FString& Token : EventTokens)
	{
		FString TrimmedToken = Token.TrimStartAndEnd();
		if (TrimmedToken.IsEmpty()) continue;

		FManifestDialogueEventDefinition Event;

		// Extract event type and params
		int32 ParenStart = TrimmedToken.Find(TEXT("("));
		if (ParenStart != INDEX_NONE)
		{
			Event.Type = TrimmedToken.Left(ParenStart).TrimStartAndEnd();

			int32 ParenEnd = TrimmedToken.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (ParenEnd > ParenStart)
			{
				FString ParamsStr = TrimmedToken.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
				Event.Properties = ParseTokenParams(ParamsStr);
			}
		}
		else
		{
			Event.Type = TrimmedToken;
		}

		if (!Event.Type.IsEmpty())
		{
			Events.Add(Event);
		}
	}

	return Events;
}

TArray<FManifestDialogueConditionDefinition> FQuestTableConverter::ParseConditions(const FString& ConditionsToken)
{
	TArray<FManifestDialogueConditionDefinition> Conditions;
	if (ConditionsToken.IsEmpty()) return Conditions;

	// Split by semicolon: NC_HasItem(Item=EI_Key);NC_QuestComplete(Quest=Quest_Prologue)
	TArray<FString> ConditionTokens;
	ConditionsToken.ParseIntoArray(ConditionTokens, TEXT(";"), true);

	for (const FString& Token : ConditionTokens)
	{
		FString TrimmedToken = Token.TrimStartAndEnd();
		if (TrimmedToken.IsEmpty()) continue;

		FManifestDialogueConditionDefinition Condition;

		// Check for NOT prefix
		if (TrimmedToken.StartsWith(TEXT("!")))
		{
			Condition.bNot = true;
			TrimmedToken = TrimmedToken.Mid(1);
		}

		// Extract condition type and params
		int32 ParenStart = TrimmedToken.Find(TEXT("("));
		if (ParenStart != INDEX_NONE)
		{
			Condition.Type = TrimmedToken.Left(ParenStart).TrimStartAndEnd();

			int32 ParenEnd = TrimmedToken.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (ParenEnd > ParenStart)
			{
				FString ParamsStr = TrimmedToken.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
				Condition.Properties = ParseTokenParams(ParamsStr);
			}
		}
		else
		{
			Condition.Type = TrimmedToken;
		}

		if (!Condition.Type.IsEmpty())
		{
			Conditions.Add(Condition);
		}
	}

	return Conditions;
}

FManifestQuestRewardDefinition FQuestTableConverter::ParseRewards(const FString& RewardsToken)
{
	FManifestQuestRewardDefinition Rewards;
	if (RewardsToken.IsEmpty()) return Rewards;

	// Parse: Reward(Currency=100,XP=50,Items=EI_Sword:1|EI_Shield:1)
	int32 ParenStart = RewardsToken.Find(TEXT("("));
	if (ParenStart != INDEX_NONE)
	{
		int32 ParenEnd = RewardsToken.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (ParenEnd > ParenStart)
		{
			FString ParamsStr = RewardsToken.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
			TMap<FString, FString> Params = ParseTokenParams(ParamsStr);

			if (Params.Contains(TEXT("Currency")))
			{
				Rewards.Currency = FCString::Atoi(*Params[TEXT("Currency")]);
			}
			if (Params.Contains(TEXT("Gold")))
			{
				Rewards.Currency = FCString::Atoi(*Params[TEXT("Gold")]);
			}
			if (Params.Contains(TEXT("XP")))
			{
				Rewards.XP = FCString::Atoi(*Params[TEXT("XP")]);
			}
			if (Params.Contains(TEXT("Experience")))
			{
				Rewards.XP = FCString::Atoi(*Params[TEXT("Experience")]);
			}
			if (Params.Contains(TEXT("Items")))
			{
				// Parse items: EI_Sword:1|EI_Shield:1
				FString ItemsStr = Params[TEXT("Items")];
				TArray<FString> ItemEntries;
				ItemsStr.ParseIntoArray(ItemEntries, TEXT("|"), true);

				for (const FString& Entry : ItemEntries)
				{
					FString ItemName, QuantityStr;
					if (Entry.Split(TEXT(":"), &ItemName, &QuantityStr))
					{
						Rewards.Items.Add(ItemName.TrimStartAndEnd());
					}
					else
					{
						Rewards.Items.Add(Entry.TrimStartAndEnd());
					}
				}
			}
		}
	}

	return Rewards;
}

void FQuestTableConverter::ApplyDefaults(FManifestQuestDefinition& Def)
{
	if (Def.Folder.IsEmpty())
	{
		Def.Folder = TEXT("/Game/Quests");
	}

	// Ensure quest has at least a Start state
	if (Def.StartState.IsEmpty() && Def.States.Num() > 0)
	{
		Def.StartState = Def.States[0].Id;
	}
}

TArray<FQuestTableRow> FQuestTableConverter::ConvertDefinitionToRows(const FManifestQuestDefinition& Def)
{
	TArray<FQuestTableRow> Rows;

	// Extract quest name from asset name (Quest_Name -> Name)
	FString QuestName = Def.Name;
	if (QuestName.StartsWith(TEXT("Quest_")))
	{
		QuestName = QuestName.Mid(6);
	}

	// Build a map of DestinationState -> (SourceStateId, Branch) for finding incoming branches
	TMap<FString, TPair<FString, const FManifestQuestBranchDefinition*>> IncomingBranches;
	for (const FManifestQuestStateDefinition& State : Def.States)
	{
		for (const FManifestQuestBranchDefinition& Branch : State.Branches)
		{
			if (!Branch.DestinationState.IsEmpty())
			{
				IncomingBranches.Add(Branch.DestinationState, TPair<FString, const FManifestQuestBranchDefinition*>(State.Id, &Branch));
			}
		}
	}

	for (const FManifestQuestStateDefinition& State : Def.States)
	{
		FQuestTableRow Row;
		Row.QuestName = QuestName;
		Row.DisplayName = Def.QuestName;
		Row.StateID = State.Id;
		Row.Description = State.Description;
		Row.StateType = FQuestTableRow::ParseStateType(State.Type);

		// Find the incoming branch (the branch that leads TO this state)
		if (const TPair<FString, const FManifestQuestBranchDefinition*>* IncomingPair = IncomingBranches.Find(State.Id))
		{
			Row.ParentBranch = IncomingPair->Key;  // Source state ID
			const FManifestQuestBranchDefinition* IncomingBranch = IncomingPair->Value;

			// Build Tasks token string from incoming branch
			TArray<FString> TaskTokens;
			for (const FManifestQuestTaskDefinition& Task : IncomingBranch->Tasks)
			{
				FString Token = Task.TaskClass;
				TArray<FString> Params;
				if (!Task.Argument.IsEmpty())
				{
					Params.Add(FString::Printf(TEXT("Arg=%s"), *Task.Argument));
				}
				if (Task.Quantity > 1)
				{
					Params.Add(FString::Printf(TEXT("Count=%d"), Task.Quantity));
				}
				if (Task.bOptional)
				{
					Params.Add(TEXT("Optional=true"));
				}
				if (Params.Num() > 0)
				{
					Token += TEXT("(") + FString::Join(Params, TEXT(",")) + TEXT(")");
				}
				TaskTokens.Add(Token);
			}
			Row.Tasks = FString::Join(TaskTokens, TEXT(";"));

			// Build Events token string from incoming branch events
			TArray<FString> EventTokens;
			for (const FManifestDialogueEventDefinition& Event : IncomingBranch->Events)
			{
				FString Token = Event.Type;
				if (Event.Properties.Num() > 0)
				{
					TArray<FString> PropStrings;
					for (const auto& Prop : Event.Properties)
					{
						PropStrings.Add(FString::Printf(TEXT("%s=%s"), *Prop.Key, *Prop.Value));
					}
					Token += TEXT("(") + FString::Join(PropStrings, TEXT(",")) + TEXT(")");
				}
				EventTokens.Add(Token);
			}
			Row.Events = FString::Join(EventTokens, TEXT(";"));
		}

		// Add state-level events (on state entry)
		if (State.Events.Num() > 0)
		{
			TArray<FString> StateEventTokens;
			for (const FManifestDialogueEventDefinition& Event : State.Events)
			{
				FString Token = Event.Type;
				if (Event.Properties.Num() > 0)
				{
					TArray<FString> PropStrings;
					for (const auto& Prop : Event.Properties)
					{
						PropStrings.Add(FString::Printf(TEXT("%s=%s"), *Prop.Key, *Prop.Value));
					}
					Token += TEXT("(") + FString::Join(PropStrings, TEXT(",")) + TEXT(")");
				}
				StateEventTokens.Add(Token);
			}
			// Append to existing events if any
			if (!Row.Events.IsEmpty())
			{
				Row.Events += TEXT(";");
			}
			Row.Events += FString::Join(StateEventTokens, TEXT(";"));
		}

		// Build Rewards token string for success states
		if (State.Type.Equals(TEXT("success"), ESearchCase::IgnoreCase) &&
			(Def.Rewards.Currency > 0 || Def.Rewards.XP > 0 || Def.Rewards.Items.Num() > 0))
		{
			TArray<FString> RewardParams;
			if (Def.Rewards.Currency > 0)
			{
				RewardParams.Add(FString::Printf(TEXT("Currency=%d"), Def.Rewards.Currency));
			}
			if (Def.Rewards.XP > 0)
			{
				RewardParams.Add(FString::Printf(TEXT("XP=%d"), Def.Rewards.XP));
			}
			if (Def.Rewards.Items.Num() > 0)
			{
				RewardParams.Add(FString::Printf(TEXT("Items=%s"), *FString::Join(Def.Rewards.Items, TEXT("|"))));
			}
			Row.Rewards = TEXT("Reward(") + FString::Join(RewardParams, TEXT(",")) + TEXT(")");
		}

		Rows.Add(Row);
	}

	return Rows;
}

TMap<FString, FString> FQuestTableConverter::ParseTokenParams(const FString& ParamsStr)
{
	TMap<FString, FString> Params;
	if (ParamsStr.IsEmpty()) return Params;

	// Split by comma: Key=Value,Key2=Value2
	TArray<FString> ParamPairs;
	ParamsStr.ParseIntoArray(ParamPairs, TEXT(","), true);

	for (const FString& Pair : ParamPairs)
	{
		FString Key, Value;
		if (Pair.Split(TEXT("="), &Key, &Value))
		{
			Params.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
		}
	}

	return Params;
}

FString FQuestTableConverter::GetStateTypeString(EQuestStateType StateType)
{
	switch (StateType)
	{
		case EQuestStateType::Success: return TEXT("success");
		case EQuestStateType::Failure: return TEXT("failure");
		case EQuestStateType::Regular:
		default: return TEXT("regular");
	}
}
