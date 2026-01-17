// QuestTableEditorTypes.h
// Quest Table Editor - Data types (v4.8 - Following NPC/Dialogue table patterns)
// Implements: XLSX 3-way sync, validation cache, soft delete, generation tracking
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "DialogueTableEditorTypes.h"  // For EValidationState (shared enum)
#include "QuestTableEditorTypes.generated.h"

/**
 * Row status enum for Quest table (sync state)
 */
UENUM(BlueprintType)
enum class EQuestTableRowStatus : uint8
{
	New UMETA(DisplayName = "New"),
	Modified UMETA(DisplayName = "Modified"),
	Synced UMETA(DisplayName = "Synced"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Quest state type enum
 */
UENUM(BlueprintType)
enum class EQuestStateType : uint8
{
	Regular UMETA(DisplayName = "Regular"),
	Success UMETA(DisplayName = "Success"),
	Failure UMETA(DisplayName = "Failure")
};

/**
 * Single row in the Quest Table Editor (v4.8 - 12 columns)
 * Each row represents ONE state within a quest
 * Multiple rows with same QuestName form a complete quest
 * Maps to UQuest state machine
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATOR_API FQuestTableRow
{
	GENERATED_BODY()

	/** Unique row identifier for internal tracking */
	UPROPERTY()
	FGuid RowId;

	//=========================================================================
	// Core Identity (3 columns)
	//=========================================================================

	/** Row status: New, Modified, Synced, Error (auto-calculated) */
	UPROPERTY(VisibleAnywhere, Category = "Identity")
	EQuestTableRowStatus Status = EQuestTableRowStatus::New;

	/** Quest Name - groups rows into same quest (Quest_{QuestName}) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString QuestName;

	/** Display name shown in-game quest log */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString DisplayName;

	//=========================================================================
	// State Definition (4 columns)
	//=========================================================================

	/** State ID - unique within quest (Start, Gathering, Return, Complete, etc.) */
	UPROPERTY(EditAnywhere, Category = "State")
	FString StateID;

	/** State type: Regular, Success, Failure */
	UPROPERTY(EditAnywhere, Category = "State")
	EQuestStateType StateType = EQuestStateType::Regular;

	/** State description - shown in quest tracker */
	UPROPERTY(EditAnywhere, Category = "State")
	FString Description;

	/** Parent branch ID that leads TO this state (empty for Start state) */
	UPROPERTY(EditAnywhere, Category = "State")
	FString ParentBranch;

	//=========================================================================
	// Logic (3 columns) - Token-based
	//=========================================================================

	/** Tasks token string: BPT_FindItem(Item=EI_Ore,Count=10);BPT_Move(Location=POI_Forge) */
	UPROPERTY(EditAnywhere, Category = "Logic")
	FString Tasks;

	/** Events token string: NE_GiveXP(Amount=50);NE_AddItem(Item=EI_Sword) */
	UPROPERTY(EditAnywhere, Category = "Logic")
	FString Events;

	/** Conditions token string: NC_HasItem(Item=EI_Key);NC_QuestComplete(Quest=Quest_Prologue) */
	UPROPERTY(EditAnywhere, Category = "Logic")
	FString Conditions;

	//=========================================================================
	// Rewards & Meta (2 columns)
	//=========================================================================

	/** Rewards token string: Reward(Currency=100,XP=50,Items=EI_Sword:1) */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	FString Rewards;

	/** Designer notes/comments (not exported to game) */
	UPROPERTY(EditAnywhere, Category = "Meta")
	FString Notes;

	//=========================================================================
	// Soft Delete (v4.6 pattern)
	//=========================================================================

	/** Soft delete flag - row skipped during generation, asset untouched */
	UPROPERTY(EditAnywhere, Category = "Meta")
	bool bDeleted = false;

	//=========================================================================
	// Internal (not displayed as columns)
	//=========================================================================

	/** Generated Quest asset path (after generation) */
	UPROPERTY(VisibleAnywhere, Category = "Internal")
	FSoftObjectPath GeneratedQuest;

	//=========================================================================
	// Validation cache (Transient - UI only, not serialized)
	//=========================================================================

	/** Current validation state (Unknown/Valid/Invalid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	EValidationState ValidationState = EValidationState::Unknown;

	/** Summary of validation issues (empty if valid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString ValidationSummary;

	/** Count of validation issues */
	UPROPERTY(Transient)
	int32 ValidationIssueCount = 0;

	/** Hash of inputs used for validation (staleness detection) */
	UPROPERTY(Transient)
	uint32 ValidationInputHash = 0;

	//=========================================================================
	// XLSX Sync Hash (persisted - for 3-way merge comparison)
	//=========================================================================

	/** Hash of row content at last successful sync operation (0 = never synced) */
	UPROPERTY(EditAnywhere, Category = "Sync")
	int64 LastSyncedHash = 0;

	/** Compute sync hash for 3-way merge comparison */
	int64 ComputeSyncHash() const
	{
		return static_cast<int64>(ComputeEditableFieldsHash());
	}

	/** Invalidate cached validation (call on row edit) */
	void InvalidateValidation()
	{
		ValidationState = EValidationState::Unknown;
		ValidationSummary.Empty();
		ValidationIssueCount = 0;
		ValidationInputHash = 0;
	}

	/** Get validation state accessor */
	EValidationState GetValidationState() const { return ValidationState; }

	/** Compute hash of editable fields (for validation staleness detection) */
	uint32 ComputeEditableFieldsHash() const
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(QuestName));
		Hash = HashCombine(Hash, GetTypeHash(DisplayName));
		Hash = HashCombine(Hash, GetTypeHash(StateID));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(StateType)));
		Hash = HashCombine(Hash, GetTypeHash(Description));
		Hash = HashCombine(Hash, GetTypeHash(ParentBranch));
		Hash = HashCombine(Hash, GetTypeHash(Tasks));
		Hash = HashCombine(Hash, GetTypeHash(Events));
		Hash = HashCombine(Hash, GetTypeHash(Conditions));
		Hash = HashCombine(Hash, GetTypeHash(Rewards));
		Hash = HashCombine(Hash, GetTypeHash(bDeleted));
		return Hash;
	}

	//=========================================================================
	// Methods
	//=========================================================================

	FQuestTableRow()
	{
		RowId = FGuid::NewGuid();
	}

	bool IsValid() const
	{
		return !QuestName.IsEmpty() && !StateID.IsEmpty();
	}

	/** Get status color for UI */
	FLinearColor GetStatusColor() const
	{
		switch (Status)
		{
			case EQuestTableRowStatus::New: return FLinearColor(0.2f, 0.6f, 1.0f); // Blue
			case EQuestTableRowStatus::Modified: return FLinearColor(1.0f, 0.8f, 0.2f); // Yellow
			case EQuestTableRowStatus::Synced: return FLinearColor(0.2f, 0.8f, 0.2f); // Green
			case EQuestTableRowStatus::Error: return FLinearColor(1.0f, 0.2f, 0.2f); // Red
			default: return FLinearColor::White;
		}
	}

	/** Get status as string for display */
	FString GetStatusString() const
	{
		switch (Status)
		{
			case EQuestTableRowStatus::New: return TEXT("New");
			case EQuestTableRowStatus::Modified: return TEXT("Modified");
			case EQuestTableRowStatus::Synced: return TEXT("Synced");
			case EQuestTableRowStatus::Error: return TEXT("Error");
			default: return TEXT("Unknown");
		}
	}

	/** Get state type color for UI */
	FLinearColor GetStateTypeColor() const
	{
		switch (StateType)
		{
			case EQuestStateType::Success: return FLinearColor(0.2f, 0.8f, 0.2f); // Green
			case EQuestStateType::Failure: return FLinearColor(1.0f, 0.2f, 0.2f); // Red
			case EQuestStateType::Regular: return FLinearColor(0.5f, 0.5f, 0.5f); // Gray
			default: return FLinearColor::White;
		}
	}

	/** Get state type as string */
	FString GetStateTypeString() const
	{
		switch (StateType)
		{
			case EQuestStateType::Regular: return TEXT("Regular");
			case EQuestStateType::Success: return TEXT("Success");
			case EQuestStateType::Failure: return TEXT("Failure");
			default: return TEXT("Unknown");
		}
	}

	/** Get validation color for row stripe */
	FLinearColor GetValidationColor() const
	{
		switch (ValidationState)
		{
			case EValidationState::Valid: return FLinearColor(0.2f, 0.8f, 0.2f);
			case EValidationState::Invalid: return FLinearColor(1.0f, 0.2f, 0.2f);
			case EValidationState::Unknown: return FLinearColor(1.0f, 0.8f, 0.2f);
			default: return FLinearColor::White;
		}
	}

	/** Parse state type from string */
	static EQuestStateType ParseStateType(const FString& TypeString)
	{
		if (TypeString.Equals(TEXT("Success"), ESearchCase::IgnoreCase)) return EQuestStateType::Success;
		if (TypeString.Equals(TEXT("Failure"), ESearchCase::IgnoreCase)) return EQuestStateType::Failure;
		return EQuestStateType::Regular;
	}
};

/**
 * Container for all Quest table data
 * Saved as a single DataAsset for persistence
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UQuestTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All quest rows in the table */
	UPROPERTY(EditAnywhere, Category = "Data")
	TArray<FQuestTableRow> Rows;

	/** Project name for asset paths */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString ProjectName = TEXT("NP22B57");

	/** Default output folder */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString OutputFolder = TEXT("/Game/Quests");

	/** Last sync timestamp */
	UPROPERTY(VisibleAnywhere, Category = "Metadata")
	FDateTime LastSyncTime;

	//=========================================================================
	// Validation versioning (persisted - staleness survives restart)
	//=========================================================================

	/** GUID updated when asset lists are regenerated */
	UPROPERTY(EditAnywhere, Category = "Validation")
	FGuid ListsVersionGuid;

	/** Bump ListsVersionGuid to invalidate all cached validation */
	void BumpListsVersion()
	{
		ListsVersionGuid = FGuid::NewGuid();
	}

	//=========================================================================
	// Generation tracking (for "Assets out of date" indicator)
	//=========================================================================

	/** Hash of all authored fields at last successful generation */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	uint32 LastGeneratedHash = 0;

	/** Timestamp of last generation attempt */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	FDateTime LastGeneratedTime;

	/** Number of failures in last generation (0 = full success) */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	int32 LastGenerateFailureCount = 0;

	/** Compute hash of all authored row data */
	uint32 ComputeAllRowsHash() const
	{
		uint32 Hash = 0;
		for (const FQuestTableRow& Row : Rows)
		{
			Hash = HashCombine(Hash, Row.ComputeEditableFieldsHash());
		}
		return Hash;
	}

	/** Check if assets are out of date (need regeneration) */
	bool AreAssetsOutOfDate() const
	{
		return ComputeAllRowsHash() != LastGeneratedHash;
	}

	/** Update generation tracking after successful generation */
	void OnGenerationComplete(int32 FailureCount)
	{
		LastGeneratedTime = FDateTime::Now();
		LastGenerateFailureCount = FailureCount;
		if (FailureCount == 0)
		{
			LastGeneratedHash = ComputeAllRowsHash();
		}
	}

	//=========================================================================
	// Row Management
	//=========================================================================

	/** Add a new empty row */
	FQuestTableRow& AddRow()
	{
		FQuestTableRow& NewRow = Rows.AddDefaulted_GetRef();
		NewRow.RowId = FGuid::NewGuid();
		NewRow.Status = EQuestTableRowStatus::New;
		return NewRow;
	}

	/** Duplicate an existing row */
	FQuestTableRow* DuplicateRow(int32 SourceIndex)
	{
		if (Rows.IsValidIndex(SourceIndex))
		{
			FQuestTableRow NewRow = Rows[SourceIndex];
			NewRow.RowId = FGuid::NewGuid();
			NewRow.StateID += TEXT("_Copy");
			NewRow.Status = EQuestTableRowStatus::New;
			NewRow.GeneratedQuest.Reset();
			Rows.Add(NewRow);
			return &Rows.Last();
		}
		return nullptr;
	}

	/** Delete row by index */
	void DeleteRow(int32 Index)
	{
		if (Rows.IsValidIndex(Index))
		{
			Rows.RemoveAt(Index);
		}
	}

	/** Find row by GUID */
	FQuestTableRow* FindRowByGuid(const FGuid& InRowId)
	{
		return Rows.FindByPredicate([&InRowId](const FQuestTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Find row index by GUID */
	int32 FindRowIndexByGuid(const FGuid& InRowId) const
	{
		return Rows.IndexOfByPredicate([&InRowId](const FQuestTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Get all unique quest names in the table */
	TArray<FString> GetUniqueQuestNames() const
	{
		TSet<FString> UniqueNames;
		for (const FQuestTableRow& Row : Rows)
		{
			if (!Row.QuestName.IsEmpty() && !Row.bDeleted)
			{
				UniqueNames.Add(Row.QuestName);
			}
		}
		return UniqueNames.Array();
	}

	/** Get all rows for a specific quest */
	TArray<FQuestTableRow*> GetRowsForQuest(const FString& QuestName)
	{
		TArray<FQuestTableRow*> Result;
		for (FQuestTableRow& Row : Rows)
		{
			if (Row.QuestName == QuestName && !Row.bDeleted)
			{
				Result.Add(&Row);
			}
		}
		return Result;
	}

	/** Get row count */
	int32 GetRowCount() const { return Rows.Num(); }
};
