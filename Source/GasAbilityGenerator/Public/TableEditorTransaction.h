// GasAbilityGenerator - Table Editor Transaction System
// v7.2: Undo/Redo support for all table editors
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Base class for table editor transactions (undo/redo operations)
 * Uses command pattern to capture and reverse changes
 */
class GASABILITYGENERATOR_API FTableEditorTransaction
{
public:
	virtual ~FTableEditorTransaction() = default;

	/** Execute the transaction (or redo it) */
	virtual void Execute() = 0;

	/** Undo the transaction */
	virtual void Undo() = 0;

	/** Get a description of this transaction for display */
	virtual FString GetDescription() const = 0;

	/** Get the timestamp when this transaction was created */
	FDateTime GetTimestamp() const { return Timestamp; }

protected:
	FDateTime Timestamp = FDateTime::Now();
};

/**
 * Transaction stack manager - handles undo/redo history
 * Each table editor instance should have its own stack
 */
class GASABILITYGENERATOR_API FTableEditorTransactionStack
{
public:
	FTableEditorTransactionStack(int32 InMaxHistory = 50)
		: MaxHistory(InMaxHistory)
	{
	}

	/**
	 * Add a new transaction to the stack
	 * Clears the redo stack since we've branched history
	 * @param Transaction - The transaction to add (takes ownership)
	 */
	void AddTransaction(TSharedPtr<FTableEditorTransaction> Transaction)
	{
		if (!Transaction.IsValid())
		{
			return;
		}

		// Clear redo stack - we've branched
		RedoStack.Empty();

		// Add to undo stack
		UndoStack.Push(Transaction);

		// Trim if over max history
		while (UndoStack.Num() > MaxHistory)
		{
			UndoStack.RemoveAt(0);
		}

		OnStackChanged.Broadcast();
	}

	/**
	 * Undo the most recent transaction
	 * @return true if undo was performed
	 */
	bool Undo()
	{
		if (UndoStack.Num() == 0)
		{
			return false;
		}

		TSharedPtr<FTableEditorTransaction> Transaction = UndoStack.Pop();
		Transaction->Undo();
		RedoStack.Push(Transaction);

		OnStackChanged.Broadcast();
		return true;
	}

	/**
	 * Redo the most recently undone transaction
	 * @return true if redo was performed
	 */
	bool Redo()
	{
		if (RedoStack.Num() == 0)
		{
			return false;
		}

		TSharedPtr<FTableEditorTransaction> Transaction = RedoStack.Pop();
		Transaction->Execute();
		UndoStack.Push(Transaction);

		OnStackChanged.Broadcast();
		return true;
	}

	/** Check if undo is available */
	bool CanUndo() const { return UndoStack.Num() > 0; }

	/** Check if redo is available */
	bool CanRedo() const { return RedoStack.Num() > 0; }

	/** Get description of next undo operation */
	FString GetUndoDescription() const
	{
		if (UndoStack.Num() > 0)
		{
			return UndoStack.Last()->GetDescription();
		}
		return FString();
	}

	/** Get description of next redo operation */
	FString GetRedoDescription() const
	{
		if (RedoStack.Num() > 0)
		{
			return RedoStack.Last()->GetDescription();
		}
		return FString();
	}

	/** Clear all history */
	void Clear()
	{
		UndoStack.Empty();
		RedoStack.Empty();
		OnStackChanged.Broadcast();
	}

	/** Get undo stack size */
	int32 GetUndoCount() const { return UndoStack.Num(); }

	/** Get redo stack size */
	int32 GetRedoCount() const { return RedoStack.Num(); }

	/** Delegate fired when stack changes */
	DECLARE_MULTICAST_DELEGATE(FOnStackChanged);
	FOnStackChanged OnStackChanged;

private:
	TArray<TSharedPtr<FTableEditorTransaction>> UndoStack;
	TArray<TSharedPtr<FTableEditorTransaction>> RedoStack;
	int32 MaxHistory;
};

//=============================================================================
// Generic Row Transactions
// These are templated for use with any table row type (FNPCTableRow, etc.)
//=============================================================================

/**
 * Transaction for adding a row
 * Undo removes the row, Redo adds it back
 */
template<typename RowType>
class TTableRowAddTransaction : public FTableEditorTransaction
{
public:
	TTableRowAddTransaction(
		TArray<TSharedPtr<RowType>>& InRows,
		TSharedPtr<RowType> InNewRow,
		int32 InInsertIndex = INDEX_NONE)
		: Rows(InRows)
		, NewRow(InNewRow)
		, InsertIndex(InInsertIndex)
	{
	}

	virtual void Execute() override
	{
		if (InsertIndex != INDEX_NONE && InsertIndex < Rows.Num())
		{
			Rows.Insert(NewRow, InsertIndex);
		}
		else
		{
			InsertIndex = Rows.Num();
			Rows.Add(NewRow);
		}
	}

	virtual void Undo() override
	{
		Rows.Remove(NewRow);
	}

	virtual FString GetDescription() const override
	{
		return TEXT("Add Row");
	}

private:
	TArray<TSharedPtr<RowType>>& Rows;
	TSharedPtr<RowType> NewRow;
	int32 InsertIndex;
};

/**
 * Transaction for deleting rows
 * Stores the deleted rows and their indices for undo
 */
template<typename RowType>
class TTableRowDeleteTransaction : public FTableEditorTransaction
{
public:
	TTableRowDeleteTransaction(
		TArray<TSharedPtr<RowType>>& InRows,
		const TArray<TSharedPtr<RowType>>& InRowsToDelete)
		: Rows(InRows)
	{
		// Store rows and their indices
		for (const TSharedPtr<RowType>& Row : InRowsToDelete)
		{
			int32 Index = Rows.IndexOfByKey(Row);
			if (Index != INDEX_NONE)
			{
				DeletedRows.Add(TPair<int32, TSharedPtr<RowType>>(Index, Row));
			}
		}
		// Sort by index descending for proper removal order
		DeletedRows.Sort([](const auto& A, const auto& B) { return A.Key > B.Key; });
	}

	virtual void Execute() override
	{
		// Remove rows (already sorted descending)
		for (const auto& Pair : DeletedRows)
		{
			Rows.Remove(Pair.Value);
		}
	}

	virtual void Undo() override
	{
		// Re-insert in reverse order (ascending by index)
		for (int32 i = DeletedRows.Num() - 1; i >= 0; --i)
		{
			const auto& Pair = DeletedRows[i];
			if (Pair.Key <= Rows.Num())
			{
				Rows.Insert(Pair.Value, Pair.Key);
			}
			else
			{
				Rows.Add(Pair.Value);
			}
		}
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Delete %d Row(s)"), DeletedRows.Num());
	}

private:
	TArray<TSharedPtr<RowType>>& Rows;
	TArray<TPair<int32, TSharedPtr<RowType>>> DeletedRows;
};

/**
 * Transaction for editing a cell value
 * Captures before/after state of a single field
 */
template<typename RowType, typename ValueType>
class TTableCellEditTransaction : public FTableEditorTransaction
{
public:
	TTableCellEditTransaction(
		TSharedPtr<RowType> InRow,
		ValueType RowType::*InMemberPtr,
		const ValueType& InOldValue,
		const ValueType& InNewValue,
		const FString& InFieldName)
		: Row(InRow)
		, MemberPtr(InMemberPtr)
		, OldValue(InOldValue)
		, NewValue(InNewValue)
		, FieldName(InFieldName)
	{
	}

	virtual void Execute() override
	{
		if (Row.IsValid())
		{
			Row.Get()->*MemberPtr = NewValue;
		}
	}

	virtual void Undo() override
	{
		if (Row.IsValid())
		{
			Row.Get()->*MemberPtr = OldValue;
		}
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Edit %s"), *FieldName);
	}

private:
	TSharedPtr<RowType> Row;
	ValueType RowType::*MemberPtr;
	ValueType OldValue;
	ValueType NewValue;
	FString FieldName;
};

/**
 * Transaction for editing multiple fields at once (batch edit)
 * Captures full row state before/after
 */
template<typename RowType>
class TTableRowEditTransaction : public FTableEditorTransaction
{
public:
	TTableRowEditTransaction(
		TSharedPtr<RowType> InRow,
		const RowType& InOldState,
		const RowType& InNewState)
		: Row(InRow)
		, OldState(MakeShared<RowType>(InOldState))
		, NewState(MakeShared<RowType>(InNewState))
	{
	}

	virtual void Execute() override
	{
		if (Row.IsValid() && NewState.IsValid())
		{
			// Copy all fields except RowId
			FGuid SavedRowId = Row->RowId;
			*Row = *NewState;
			Row->RowId = SavedRowId;
		}
	}

	virtual void Undo() override
	{
		if (Row.IsValid() && OldState.IsValid())
		{
			// Copy all fields except RowId
			FGuid SavedRowId = Row->RowId;
			*Row = *OldState;
			Row->RowId = SavedRowId;
		}
	}

	virtual FString GetDescription() const override
	{
		return TEXT("Edit Row");
	}

private:
	TSharedPtr<RowType> Row;
	TSharedPtr<RowType> OldState;
	TSharedPtr<RowType> NewState;
};

/**
 * Compound transaction - groups multiple transactions as one undo unit
 */
class GASABILITYGENERATOR_API FTableCompoundTransaction : public FTableEditorTransaction
{
public:
	FTableCompoundTransaction(const FString& InDescription)
		: Description(InDescription)
	{
	}

	void AddTransaction(TSharedPtr<FTableEditorTransaction> Transaction)
	{
		if (Transaction.IsValid())
		{
			Transactions.Add(Transaction);
		}
	}

	virtual void Execute() override
	{
		for (const auto& Transaction : Transactions)
		{
			Transaction->Execute();
		}
	}

	virtual void Undo() override
	{
		// Undo in reverse order
		for (int32 i = Transactions.Num() - 1; i >= 0; --i)
		{
			Transactions[i]->Undo();
		}
	}

	virtual FString GetDescription() const override
	{
		return Description;
	}

private:
	TArray<TSharedPtr<FTableEditorTransaction>> Transactions;
	FString Description;
};
