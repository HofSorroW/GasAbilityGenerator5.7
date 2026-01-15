// GasAbilityGenerator - Dialogue Table Editor Types
// v4.6: Added generation tracking (LastGeneratedHash), soft delete (bDeleted)
// v4.5: Added EValidationState enum and validation cache fields
// v4.4: Added EventsTokenStr/ConditionsTokenStr for token-based authoring
// v4.3: Added FGuid RowId for XLSX sync identity tracking
// v4.0: Minimal dialogue book system for batch dialogue creation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueTableEditorTypes.generated.h"

/**
 * Validation state for table rows (shared by Dialogue and NPC editors)
 * Used for UI tinting and status bar display
 */
UENUM(BlueprintType)
enum class EValidationState : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),   // Yellow - not validated yet / invalidated
	Valid UMETA(DisplayName = "Valid"),       // Green - passed validation
	Invalid UMETA(DisplayName = "Invalid")    // Red - has errors
};

/**
 * Dialogue node type - NPC speaks or Player chooses
 */
UENUM(BlueprintType)
enum class EDialogueTableNodeType : uint8
{
	NPC UMETA(DisplayName = "NPC"),
	Player UMETA(DisplayName = "Player")
};

/**
 * Minimal dialogue row - just the essential dialogue content
 * Plugin applies all technical defaults during generation
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATOR_API FDialogueTableRow
{
	GENERATED_BODY()

	/** Stable row identifier for XLSX sync - survives reorder/filter operations */
	UPROPERTY()
	FGuid RowId;

	/** Which dialogue this node belongs to (e.g., DBP_Seth, DBP_Blacksmith) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName DialogueID;

	/** Unique identifier for this node within the dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NodeID;

	/** Whether this is an NPC line or a Player choice */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EDialogueTableNodeType NodeType = EDialogueTableNodeType::NPC;

	/** Speaker name for NPC nodes (e.g., Seth, Blacksmith) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName Speaker;

	/** Full dialogue text that is spoken/shown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString Text;

	/** Short option text shown in player choice wheel (Player nodes only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString OptionText;

	/** Parent node this branches from (for tree structure) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName ParentNodeID;

	/** Child nodes that follow this one (comma-separated in CSV) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FName> NextNodeIDs;

	/** Whether the player can skip this dialogue line (default: true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bSkippable = true;

	/** Designer notes - not exported to game, just for reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString Notes;

	/** v4.6: Soft delete flag - row skipped during generation, asset untouched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	bool bDeleted = false;

	//=========================================================================
	// v4.4: Token strings for Events/Conditions (authored in Excel)
	//=========================================================================

	/** Events token string - e.g., "NE_BeginQuest(QuestId=Q_Intro);NE_GiveItem(ItemId=Gold,Count=100)" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	FString EventsTokenStr;

	/** Conditions token string - e.g., "NC_QuestState(QuestId=Q_Intro,State=InProgress)" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	FString ConditionsTokenStr;

	//=========================================================================
	// v4.5.2: Validation cache (Transient - UI only, not serialized)
	// Aligned with NPC Table Editor for consistency
	//=========================================================================

	/** Current validation state (Unknown/Valid/Invalid) - mirrors NPC */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	EValidationState ValidationState = EValidationState::Unknown;

	/** Summary of validation issues (e.g., "2 error(s), 1 warning(s)") - mirrors NPC */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString ValidationSummary;

	/** Count of validation issues - mirrors NPC */
	UPROPERTY(Transient)
	int32 ValidationIssueCount = 0;

	/** Whether EventsTokenStr passed validation (token-specific) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	bool bEventsValid = true;

	/** Events validation error message (empty if valid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString EventsValidationError;

	/** Whether ConditionsTokenStr passed validation (token-specific) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	bool bConditionsValid = true;

	/** Conditions validation error message (empty if valid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString ConditionsValidationError;

	/** Hash of inputs used for validation (staleness detection) */
	UPROPERTY(Transient)
	uint32 ValidationInputHash = 0;

	/** Check if all tokens are valid (for partial apply logic) */
	bool AreTokensValid() const { return bEventsValid && bConditionsValid; }

	/** Get validation state - uses cached state (aligned with NPC) */
	EValidationState GetValidationState() const { return ValidationState; }

	/** Get color for validation state (for UI display) - aligned with NPC */
	FLinearColor GetValidationColor() const
	{
		switch (ValidationState)
		{
			case EValidationState::Unknown: return FLinearColor(1.0f, 0.8f, 0.2f); // Yellow
			case EValidationState::Valid:   return FLinearColor(0.2f, 0.8f, 0.2f); // Green
			case EValidationState::Invalid: return FLinearColor(1.0f, 0.2f, 0.2f); // Red
			default: return FLinearColor::White;
		}
	}

	/** Invalidate cached validation (call on row edit) */
	void InvalidateValidation()
	{
		ValidationState = EValidationState::Unknown;
		ValidationSummary.Empty();
		ValidationIssueCount = 0;
		bEventsValid = true;
		EventsValidationError.Empty();
		bConditionsValid = true;
		ConditionsValidationError.Empty();
		ValidationInputHash = 0;
	}

	/** Compute hash of editable fields (for validation staleness detection) */
	uint32 ComputeEditableFieldsHash() const
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(DialogueID));
		Hash = HashCombine(Hash, GetTypeHash(NodeID));
		Hash = HashCombine(Hash, GetTypeHash((uint8)NodeType));
		Hash = HashCombine(Hash, GetTypeHash(Speaker));
		Hash = HashCombine(Hash, GetTypeHash(Text));
		Hash = HashCombine(Hash, GetTypeHash(OptionText));
		Hash = HashCombine(Hash, GetTypeHash(EventsTokenStr));
		Hash = HashCombine(Hash, GetTypeHash(ConditionsTokenStr));
		Hash = HashCombine(Hash, GetTypeHash(ParentNodeID));
		for (const FName& NextID : NextNodeIDs)
		{
			Hash = HashCombine(Hash, GetTypeHash(NextID));
		}
		Hash = HashCombine(Hash, GetTypeHash(bDeleted));  // v4.6: Include soft delete
		return Hash;
	}

	FDialogueTableRow()
		: RowId(FGuid::NewGuid())
		, NodeType(EDialogueTableNodeType::NPC)
	{
	}

	/** Check if this row has valid required fields */
	bool IsValid() const
	{
		if (DialogueID.IsNone() || NodeID.IsNone())
		{
			return false;
		}
		if (NodeType == EDialogueTableNodeType::NPC && Text.IsEmpty())
		{
			return false;
		}
		if (NodeType == EDialogueTableNodeType::Player && OptionText.IsEmpty())
		{
			return false;
		}
		return true;
	}
};

/**
 * DataAsset containing all dialogue rows - the "dialogue book"
 * Stored in Content folder, can be imported/exported as XLSX
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UDialogueTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All dialogue rows across all dialogues */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Table")
	TArray<FDialogueTableRow> Rows;

	/** Path to the source file (XLSX or CSV, for re-import) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Table")
	FString SourceFilePath;

	//=========================================================================
	// v4.5: Validation versioning (persisted - staleness survives restart)
	//=========================================================================

	/** GUID updated when _Lists is regenerated (asset scan). Used for validation staleness detection. */
	UPROPERTY(EditAnywhere, Category = "Validation")
	FGuid ListsVersionGuid;

	/** Bump ListsVersionGuid to invalidate all cached validation */
	void BumpListsVersion()
	{
		ListsVersionGuid = FGuid::NewGuid();
	}

	//=========================================================================
	// v4.6: Generation tracking (for "Assets out of date" indicator)
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

	/** Compute hash of all authored row data (excludes validation cache, transients) */
	uint32 ComputeAllRowsHash() const
	{
		uint32 Hash = 0;
		for (const FDialogueTableRow& Row : Rows)
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
		// If failures > 0, hash stays stale (correct behavior)
	}

	/** Get all unique DialogueIDs in this table */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Table")
	TArray<FName> GetAllDialogueIDs() const
	{
		TSet<FName> UniqueIDs;
		for (const FDialogueTableRow& Row : Rows)
		{
			if (!Row.DialogueID.IsNone())
			{
				UniqueIDs.Add(Row.DialogueID);
			}
		}
		return UniqueIDs.Array();
	}

	/** Get all rows for a specific DialogueID */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Table")
	TArray<FDialogueTableRow> GetRowsForDialogue(FName DialogueID) const
	{
		TArray<FDialogueTableRow> Result;
		for (const FDialogueTableRow& Row : Rows)
		{
			if (Row.DialogueID == DialogueID)
			{
				Result.Add(Row);
			}
		}
		return Result;
	}

	/** Find a specific row by DialogueID and NodeID */
	const FDialogueTableRow* FindRow(FName DialogueID, FName NodeID) const
	{
		for (const FDialogueTableRow& Row : Rows)
		{
			if (Row.DialogueID == DialogueID && Row.NodeID == NodeID)
			{
				return &Row;
			}
		}
		return nullptr;
	}

	//=========================================================================
	// v4.5.2: Row management methods (aligned with NPC Table Editor)
	//=========================================================================

	/** Find row index by RowId GUID */
	int32 FindRowIndexByGuid(const FGuid& InRowId) const
	{
		return Rows.IndexOfByPredicate([&InRowId](const FDialogueTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Add a new empty row - aligned with NPC */
	FDialogueTableRow& AddRow()
	{
		FDialogueTableRow& NewRow = Rows.AddDefaulted_GetRef();
		NewRow.RowId = FGuid::NewGuid();
		NewRow.InvalidateValidation();
		return NewRow;
	}

	/** Duplicate an existing row */
	FDialogueTableRow* DuplicateRow(int32 SourceIndex)
	{
		if (Rows.IsValidIndex(SourceIndex))
		{
			FDialogueTableRow NewRow = Rows[SourceIndex];
			NewRow.RowId = FGuid::NewGuid();
			// Append _Copy to NodeID to make it unique
			FString NewNodeIDStr = NewRow.NodeID.ToString() + TEXT("_Copy");
			NewRow.NodeID = FName(*NewNodeIDStr);
			NewRow.InvalidateValidation();
			Rows.Add(NewRow);
			return &Rows.Last();
		}
		return nullptr;
	}
};
