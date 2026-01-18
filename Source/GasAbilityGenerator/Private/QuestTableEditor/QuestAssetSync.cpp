// GasAbilityGenerator - Quest Asset Sync Implementation
// v4.12.4: Full bidirectional sync between UQuest assets and Quest table rows

#include "QuestTableEditor/QuestAssetSync.h"
#include "QuestTableEditor/QuestTableValidator.h"
#include "GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorGenerators.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// Narrative Pro includes
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"
#include "Tales/QuestTask.h"
#include "QuestBlueprint.h"
#endif

#if WITH_EDITOR
// v4.12.4: Helper to convert EStateNodeType to EQuestStateType
static EQuestStateType ConvertStateNodeType(EStateNodeType NodeType)
{
	switch (NodeType)
	{
		case EStateNodeType::Success: return EQuestStateType::Success;
		case EStateNodeType::Failure: return EQuestStateType::Failure;
		default: return EQuestStateType::Regular;
	}
}
#endif

// v4.12.4: Serialize tasks from a branch to token format
static FString SerializeTasks(UQuestBranch* Branch)
{
#if WITH_EDITOR
	if (!Branch)
	{
		return FString();
	}

	TArray<FString> TaskTokens;

	// QuestTasks is TArray<UNarrativeTask*>
	FArrayProperty* TasksArrayProp = CastField<FArrayProperty>(
		Branch->GetClass()->FindPropertyByName(TEXT("QuestTasks")));

	if (TasksArrayProp)
	{
		FScriptArrayHelper TasksHelper(TasksArrayProp,
			TasksArrayProp->ContainerPtrToValuePtr<void>(Branch));

		for (int32 t = 0; t < TasksHelper.Num(); ++t)
		{
			void* TaskPtr = TasksHelper.GetRawPtr(t);
			UNarrativeTask* Task = *(UNarrativeTask**)TaskPtr;
			if (!Task)
			{
				continue;
			}

			FString TaskClassName = Task->GetClass()->GetName();
			TArray<FString> Params;

			// Extract RequiredQuantity
			if (Task->RequiredQuantity > 1)
			{
				Params.Add(FString::Printf(TEXT("Count=%d"), Task->RequiredQuantity));
			}

			// Extract bOptional
			if (Task->bOptional)
			{
				Params.Add(TEXT("Optional=true"));
			}

			// Extract bHidden
			if (Task->bHidden)
			{
				Params.Add(TEXT("Hidden=true"));
			}

			// Try to extract common task arguments via reflection
			// Check for common property names used in Narrative Pro tasks
			for (const TCHAR* PropName : { TEXT("TargetLocation"), TEXT("TargetActor"), TEXT("ItemClass"),
				TEXT("DialogueClass"), TEXT("NPCDefinition"), TEXT("GoalLocation") })
			{
				FProperty* Prop = Task->GetClass()->FindPropertyByName(PropName);
				if (!Prop)
				{
					continue;
				}

				// Handle class references (TSubclassOf)
				if (FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
				{
					// Get the raw UClass* value directly
					UClass* Value = (UClass*)ClassProp->GetPropertyValue_InContainer(Task);
					if (Value)
					{
						// Extract just the key name (remove "Target", "Class" suffixes)
						FString KeyName = PropName;
						KeyName.RemoveFromEnd(TEXT("Class"));
						KeyName.RemoveFromStart(TEXT("Target"));
						if (KeyName.IsEmpty()) KeyName = TEXT("Class");
						Params.Add(FString::Printf(TEXT("%s=%s"), *KeyName, *Value->GetName()));
					}
				}
				// Handle object references
				else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
				{
					UObject* Value = ObjProp->GetObjectPropertyValue_InContainer(Task);
					if (Value)
					{
						FString KeyName = PropName;
						KeyName.RemoveFromEnd(TEXT("Definition"));
						KeyName.RemoveFromStart(TEXT("Target"));
						if (KeyName.IsEmpty()) KeyName = TEXT("Object");
						Params.Add(FString::Printf(TEXT("%s=%s"), *KeyName, *Value->GetName()));
					}
				}
				// Handle soft class references
				else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Prop))
				{
					const FSoftObjectPtr* SoftPtr = SoftClassProp->GetPropertyValuePtr_InContainer(Task);
					if (SoftPtr && !SoftPtr->IsNull())
					{
						FString KeyName = PropName;
						KeyName.RemoveFromEnd(TEXT("Class"));
						Params.Add(FString::Printf(TEXT("%s=%s"), *KeyName, *SoftPtr->GetAssetName()));
					}
				}
				// Handle FVector (locations)
				else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
				{
					if (StructProp->Struct == TBaseStructure<FVector>::Get())
					{
						const FVector* Vec = StructProp->ContainerPtrToValuePtr<FVector>(Task);
						if (Vec && !Vec->IsZero())
						{
							Params.Add(FString::Printf(TEXT("%s=%.0f,%.0f,%.0f"), PropName, Vec->X, Vec->Y, Vec->Z));
						}
					}
				}
			}

			// Build token
			FString Token = TaskClassName;
			if (Params.Num() > 0)
			{
				Token += TEXT("(") + FString::Join(Params, TEXT(",")) + TEXT(")");
			}
			TaskTokens.Add(Token);
		}
	}

	return FString::Join(TaskTokens, TEXT(";"));
#else
	return FString();
#endif
}

FQuestAssetData FQuestAssetSync::SyncFromAsset(UQuestBlueprint* QuestBlueprint)
{
	FQuestAssetData Data;

#if WITH_EDITOR
	if (!QuestBlueprint)
	{
		return Data;
	}

	UQuest* QuestTemplate = QuestBlueprint->QuestTemplate;
	if (!QuestTemplate)
	{
		return Data;
	}

	Data.bFoundInAsset = true;
	Data.QuestName = QuestBlueprint->GetName();

	// Remove Quest_ prefix if present
	if (Data.QuestName.StartsWith(TEXT("Quest_")))
	{
		Data.QuestName = Data.QuestName.RightChop(6);
	}

	Data.DisplayName = QuestTemplate->GetQuestName().ToString();
	Data.bIsTracked = QuestTemplate->IsTracked();

	// Get States array via GetStates() (public accessor)
	TArray<UQuestState*> States = QuestTemplate->GetStates();

	for (UQuestState* State : States)
	{
		if (!State)
		{
			continue;
		}

		// Extract state info using GetID() public accessor
		FString StateID = State->GetID().ToString();
		Data.StateIDs.Add(StateID);
		Data.StateDescriptions.Add(State->Description.ToString());
		Data.StateTypes.Add(ConvertStateNodeType(State->StateNodeType));

		// Extract branches from this state
		for (UQuestBranch* Branch : State->Branches)
		{
			if (!Branch)
			{
				continue;
			}

			// Get destination state - this tells us parent relationship
			UQuestState* DestState = Branch->DestinationState;
			if (DestState)
			{
				Data.StateParentBranch.Add(DestState->GetID().ToString(), StateID);

				// Extract tasks from this branch and associate with destination state
				FString TasksToken = SerializeTasks(Branch);
				if (!TasksToken.IsEmpty())
				{
					Data.StateTasks.Add(DestState->GetID().ToString(), TasksToken);
				}
			}
		}
	}
#endif

	return Data;
}

FQuestAssetSyncResult FQuestAssetSync::SyncFromAllAssets()
{
	FQuestAssetSyncResult Result;

#if WITH_EDITOR
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Force rescan to catch newly created assets
	TArray<FString> PathsToScan = { TEXT("/Game/") };
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// Find all QuestBlueprint assets
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UQuestBlueprint::StaticClass()->GetClassPathName(), AssetList, true);

	for (const FAssetData& AssetData : AssetList)
	{
		UQuestBlueprint* QuestBlueprint = Cast<UQuestBlueprint>(AssetData.GetAsset());
		if (!QuestBlueprint)
		{
			continue;
		}

		FString QuestAssetName = AssetData.AssetName.ToString();
		FQuestAssetData QuestData = SyncFromAsset(QuestBlueprint);

		if (QuestData.bFoundInAsset)
		{
			Result.QuestData.Add(QuestAssetName, QuestData);
			Result.QuestsFound++;
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("QuestAssetSync: Synced %d Quest assets"), Result.QuestsFound);

#else
	Result.ErrorMessage = TEXT("QuestAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

int32 FQuestAssetSync::PopulateRowsFromAssets(
	TArray<FQuestTableRow>& Rows,
	const FQuestAssetSyncResult& SyncResult)
{
	int32 UpdatedCount = 0;

	// Build a map of QuestName -> Asset data for quick lookup
	// SAFETY: These pointers are valid because SyncResult is passed by const& and
	// remains alive for the entire function. Do NOT store this map as a class member
	// or return it - pointers would become dangling when SyncResult is destroyed.
	TMap<FString, const FQuestAssetData*> QuestNameToData;
	for (const auto& Pair : SyncResult.QuestData)
	{
		const FQuestAssetData& Data = Pair.Value;
		if (Data.bFoundInAsset)
		{
			QuestNameToData.Add(Data.QuestName, &Data);
		}
	}

	// Update rows that match quest names
	for (FQuestTableRow& Row : Rows)
	{
		const FQuestAssetData** DataPtr = QuestNameToData.Find(Row.QuestName);
		if (DataPtr && *DataPtr)
		{
			const FQuestAssetData& Data = **DataPtr;

			// Update display name if empty
			if (Row.DisplayName.IsEmpty() && !Data.DisplayName.IsEmpty())
			{
				Row.DisplayName = Data.DisplayName;
			}

			// Find matching state and update description
			int32 StateIndex = Data.StateIDs.IndexOfByKey(Row.StateID);
			if (StateIndex != INDEX_NONE)
			{
				if (Row.Description.IsEmpty() && Data.StateDescriptions.IsValidIndex(StateIndex))
				{
					Row.Description = Data.StateDescriptions[StateIndex];
				}
				if (Data.StateTypes.IsValidIndex(StateIndex))
				{
					Row.StateType = Data.StateTypes[StateIndex];
				}
			}

			// v4.12.4: Populate Tasks from StateTasks map
			if (const FString* TasksToken = Data.StateTasks.Find(Row.StateID))
			{
				if (Row.Tasks.IsEmpty())
				{
					Row.Tasks = *TasksToken;
				}
			}

			// v4.12.4: Populate ParentBranch from StateParentBranch map
			if (const FString* ParentID = Data.StateParentBranch.Find(Row.StateID))
			{
				if (Row.ParentBranch.IsEmpty())
				{
					Row.ParentBranch = *ParentID;
				}
			}

			// v4.12.4: Populate Rewards from StateRewards map
			if (const FString* RewardsToken = Data.StateRewards.Find(Row.StateID))
			{
				if (Row.Rewards.IsEmpty())
				{
					Row.Rewards = *RewardsToken;
				}
			}

			Row.Status = EQuestTableRowStatus::Synced;
			UpdatedCount++;
		}
	}

	return UpdatedCount;
}

FQuestAssetApplySummary FQuestAssetSync::ApplyToAssets(
	TArray<FQuestTableRow>& Rows,
	const FString& OutputFolder,
	bool bCreateMissing)
{
	FQuestAssetApplySummary Result;

#if WITH_EDITOR
	// Step 1: Group rows by QuestName
	TMap<FString, TArray<FQuestTableRow*>> QuestGroups;
	for (FQuestTableRow& Row : Rows)
	{
		if (Row.bDeleted || Row.QuestName.IsEmpty())
		{
			continue;
		}

		TArray<FQuestTableRow*>& Group = QuestGroups.FindOrAdd(Row.QuestName);
		Group.Add(&Row);
	}

	if (QuestGroups.Num() == 0)
	{
		Result.bSuccess = true;
		return Result;
	}

	// Step 2: Validate each quest group
	TArray<FQuestTableRow> AllRowsForValidation;
	for (const FQuestTableRow& Row : Rows)
	{
		if (!Row.bDeleted)
		{
			AllRowsForValidation.Add(Row);
		}
	}

	FQuestValidationResult ValidationResult = FQuestTableValidator::ValidateAll(AllRowsForValidation);

	// Build set of quests with validation errors
	TSet<FString> QuestsWithErrors;
	for (const FQuestValidationIssue& Issue : ValidationResult.Issues)
	{
		if (Issue.Severity == EQuestValidationSeverity::Error)
		{
			QuestsWithErrors.Add(Issue.QuestName);
		}
	}

	// Step 3: Process each quest
	for (auto& Pair : QuestGroups)
	{
		const FString& QuestName = Pair.Key;
		TArray<FQuestTableRow*>& QuestRows = Pair.Value;

		Result.AssetsProcessed++;

		// Skip quests with validation errors
		if (QuestsWithErrors.Contains(QuestName))
		{
			Result.AssetsSkippedValidation++;
			continue;
		}

		// Check if any row has been modified
		bool bAnyModified = false;
		for (const FQuestTableRow* Row : QuestRows)
		{
			if (Row->Status == EQuestTableRowStatus::Modified || Row->Status == EQuestTableRowStatus::New)
			{
				bAnyModified = true;
				break;
			}
		}

		if (!bAnyModified)
		{
			Result.AssetsSkippedNotModified++;
			continue;
		}

		// Check if asset exists
		FString AssetPath = OutputFolder / FString::Printf(TEXT("Quest_%s"), *QuestName);
		bool bAssetExists = !QuestRows[0]->GeneratedQuest.IsNull();

		if (!bAssetExists && !bCreateMissing)
		{
			Result.AssetsSkippedNoAsset++;
			continue;
		}

		// Build manifest definition and generate
		FManifestQuestDefinition Definition = BuildQuestDefinition(QuestName, QuestRows, OutputFolder);

		FGenerationResult GenResult = FQuestGenerator::Generate(Definition);

		if (GenResult.Status == EGenerationStatus::New)
		{
			if (bAssetExists)
			{
				Result.AssetsModified++;
			}
			else
			{
				Result.AssetsCreated++;
			}

			// Update row status and asset reference
			for (FQuestTableRow* Row : QuestRows)
			{
				Row->Status = EQuestTableRowStatus::Synced;
				Row->GeneratedQuest = FSoftObjectPath(GenResult.AssetPath);
			}
		}
		else if (GenResult.Status == EGenerationStatus::Skipped)
		{
			Result.AssetsSkippedNotModified++;
		}
		else
		{
			Result.FailedQuests.Add(QuestName);
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("QuestAssetSync: Applied %d quests (%d created, %d modified, %d skipped)"),
		Result.AssetsProcessed,
		Result.AssetsCreated,
		Result.AssetsModified,
		Result.AssetsSkippedNotModified + Result.AssetsSkippedValidation + Result.AssetsSkippedNoAsset);

#else
	Result.ErrorMessage = TEXT("QuestAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

bool FQuestAssetSync::IsAssetWritable(UObject* Asset)
{
#if WITH_EDITOR
	if (!Asset)
	{
		return false;
	}

	UPackage* Package = Asset->GetOutermost();
	if (!Package)
	{
		return false;
	}

	FString PackagePath = Package->GetName();

	// Only /Game/ assets are writable, plugin content is read-only
	return PackagePath.StartsWith(TEXT("/Game/"));

#else
	return false;
#endif
}

FManifestQuestDefinition FQuestAssetSync::BuildQuestDefinition(
	const FString& QuestName,
	const TArray<FQuestTableRow*>& QuestRows,
	const FString& OutputFolder)
{
	FManifestQuestDefinition Def;

	Def.Name = FString::Printf(TEXT("Quest_%s"), *QuestName);
	Def.Folder = OutputFolder.Replace(TEXT("/Game/"), TEXT(""));

	// Get display name from first row
	if (QuestRows.Num() > 0 && !QuestRows[0]->DisplayName.IsEmpty())
	{
		Def.QuestName = QuestRows[0]->DisplayName;
	}
	else
	{
		Def.QuestName = QuestName;
	}

	// First pass: Build states from rows
	TMap<FString, int32> StateIdToIndex;
	for (const FQuestTableRow* Row : QuestRows)
	{
		if (!Row || Row->StateID.IsEmpty())
		{
			continue;
		}

		FManifestQuestStateDefinition StateDef;
		StateDef.Id = Row->StateID;
		StateDef.Description = Row->Description;

		switch (Row->StateType)
		{
			case EQuestStateType::Success:
				StateDef.Type = TEXT("success");
				break;
			case EQuestStateType::Failure:
				StateDef.Type = TEXT("failure");
				break;
			default:
				StateDef.Type = TEXT("regular");
				break;
		}

		StateIdToIndex.Add(Row->StateID, Def.States.Num());
		Def.States.Add(StateDef);

		// Parse rewards for success states
		if (Row->StateType == EQuestStateType::Success && !Row->Rewards.IsEmpty())
		{
			// Rewards format: Reward(Currency=100,XP=50,Items=EI_Sword:1)
			FString RewardsStr = Row->Rewards;
			RewardsStr.RemoveFromStart(TEXT("Reward("));
			RewardsStr.RemoveFromEnd(TEXT(")"));

			TArray<FString> RewardParts;
			RewardsStr.ParseIntoArray(RewardParts, TEXT(","));

			for (const FString& Part : RewardParts)
			{
				FString Key, Value;
				if (Part.Split(TEXT("="), &Key, &Value))
				{
					Key = Key.TrimStartAndEnd();
					Value = Value.TrimStartAndEnd();

					if (Key.Equals(TEXT("Currency"), ESearchCase::IgnoreCase))
					{
						Def.Rewards.Currency = FCString::Atoi(*Value);
					}
					else if (Key.Equals(TEXT("XP"), ESearchCase::IgnoreCase))
					{
						Def.Rewards.XP = FCString::Atoi(*Value);
					}
					else if (Key.Equals(TEXT("Items"), ESearchCase::IgnoreCase))
					{
						// Items format: EI_Item1:Qty;EI_Item2:Qty
						TArray<FString> ItemEntries;
						Value.ParseIntoArray(ItemEntries, TEXT(";"));
						for (const FString& ItemEntry : ItemEntries)
						{
							Def.Rewards.Items.Add(ItemEntry);
						}
					}
				}
			}
		}
	}

	// Second pass: Add branches to parent states
	for (const FQuestTableRow* Row : QuestRows)
	{
		if (!Row || Row->StateID.IsEmpty())
		{
			continue;
		}

		// Create branch for non-start states (branch goes FROM parent TO this state)
		if (!Row->ParentBranch.IsEmpty() && Row->ParentBranch != TEXT("Start"))
		{
			// Find the parent state and add a branch to it
			int32* ParentIndexPtr = StateIdToIndex.Find(Row->ParentBranch);
			if (ParentIndexPtr && Def.States.IsValidIndex(*ParentIndexPtr))
			{
				FManifestQuestBranchDefinition BranchDef;
				BranchDef.Id = FString::Printf(TEXT("Branch_To_%s"), *Row->StateID);
				BranchDef.DestinationState = Row->StateID;

				// Parse tasks token string
				if (!Row->Tasks.IsEmpty())
				{
					// Tasks format: BPT_FindItem(Item=EI_Ore,Count=10);BPT_Move(...)
					TArray<FString> TaskTokens;
					Row->Tasks.ParseIntoArray(TaskTokens, TEXT(";"));

					for (const FString& Token : TaskTokens)
					{
						FString TrimmedToken = Token.TrimStartAndEnd();
						if (TrimmedToken.IsEmpty())
						{
							continue;
						}

						FManifestQuestTaskDefinition TaskDef;

						// Parse: TaskClass(Prop=Value,...)
						int32 ParenStart = TrimmedToken.Find(TEXT("("));
						if (ParenStart != INDEX_NONE)
						{
							TaskDef.TaskClass = TrimmedToken.Left(ParenStart);

							FString PropsString = TrimmedToken.Mid(ParenStart + 1);
							PropsString.RemoveFromEnd(TEXT(")"));

							// Parse properties into Argument and Quantity fields
							TArray<FString> PropPairs;
							PropsString.ParseIntoArray(PropPairs, TEXT(","));
							for (const FString& PropPair : PropPairs)
							{
								FString Key, Value;
								if (PropPair.Split(TEXT("="), &Key, &Value))
								{
									Key = Key.TrimStartAndEnd();
									Value = Value.TrimStartAndEnd();

									// Map common property names to task fields
									if (Key.Equals(TEXT("Count"), ESearchCase::IgnoreCase) ||
										Key.Equals(TEXT("Quantity"), ESearchCase::IgnoreCase))
									{
										TaskDef.Quantity = FCString::Atoi(*Value);
									}
									else if (Key.Equals(TEXT("Optional"), ESearchCase::IgnoreCase))
									{
										TaskDef.bOptional = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
									}
									else if (Key.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
									{
										TaskDef.bHidden = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
									}
									else
									{
										// First non-count property becomes the Argument
										if (TaskDef.Argument.IsEmpty())
										{
											TaskDef.Argument = Value;
										}
									}
								}
							}
						}
						else
						{
							TaskDef.TaskClass = TrimmedToken;
						}

						BranchDef.Tasks.Add(TaskDef);
					}
				}

				Def.States[*ParentIndexPtr].Branches.Add(BranchDef);
			}
		}
	}

	return Def;
}
