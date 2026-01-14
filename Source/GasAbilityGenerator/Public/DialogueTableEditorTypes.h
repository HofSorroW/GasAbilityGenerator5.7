// GasAbilityGenerator - Dialogue Table Editor Types
// v4.0: Minimal dialogue book system for batch dialogue creation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueTableEditorTypes.generated.h"

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

	FDialogueTableRow()
		: NodeType(EDialogueTableNodeType::NPC)
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
 * Stored in Content folder, can be imported/exported as CSV
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UDialogueTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All dialogue rows across all dialogues */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Table")
	TArray<FDialogueTableRow> Rows;

	/** Path to the source CSV file (for re-import) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Table")
	FString SourceCSVPath;

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
};
