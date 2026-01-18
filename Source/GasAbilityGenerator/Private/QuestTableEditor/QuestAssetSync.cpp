// GasAbilityGenerator - Quest Asset Sync Implementation
// v4.12: Bidirectional sync between UQuest assets and Quest table rows

#include "QuestTableEditor/QuestAssetSync.h"
#include "QuestTableEditor/QuestTableValidator.h"
#include "GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorGenerators.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// Narrative Pro includes
#include "Tales/Quest.h"
#endif

// v4.12: Quest sync works via generation - no direct asset reading needed
// The ApplyToAssets method uses FQuestGenerator to create/update quest assets

FQuestAssetData FQuestAssetSync::SyncFromAsset(UQuestBlueprint* /*QuestBlueprint*/)
{
	// v4.12: Sync from assets not implemented - quest structure is complex
	// Use table-to-asset generation flow instead
	FQuestAssetData Data;
	return Data;
}

FQuestAssetSyncResult FQuestAssetSync::SyncFromAllAssets()
{
	FQuestAssetSyncResult Result;
	// v4.12: Not implemented - use ApplyToAssets for generation flow
	Result.bSuccess = true;
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
