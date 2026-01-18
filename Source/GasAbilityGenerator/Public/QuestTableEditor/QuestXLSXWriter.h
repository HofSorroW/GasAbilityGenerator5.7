// GasAbilityGenerator - Quest XLSX Writer
// v4.12: Export Quest table to Excel format with sync support
//
// Creates XLSX files with:
// - Sentinel row for format detection (#QUEST_SHEET_V1)
// - Column ID mapping (immune to Excel column reorder)
// - _Lists sheet for dropdown validation
// - _Meta sheet for sync metadata (export GUID, timestamps, hashes)
//
// File structure mirrors NPCXLSXWriter for consistency.

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditorTypes.h"

/**
 * Column definition for Quest XLSX export
 */
struct FQuestXLSXColumn
{
	FString ColumnId;      // Machine ID used in sentinel row
	FString DisplayName;   // Human-readable header
	float Width;           // Column width in Excel character units

	FQuestXLSXColumn(const FString& InId, const FString& InName, float InWidth = 15.0f)
		: ColumnId(InId), DisplayName(InName), Width(InWidth) {}
};

/**
 * Exports Quest table data to XLSX format
 */
class GASABILITYGENERATOR_API FQuestXLSXWriter
{
public:
	/**
	 * Export Quest table to XLSX file
	 * @param Rows - Quest rows to export
	 * @param FilePath - Output file path (.xlsx)
	 * @param OutError - Error message if export fails
	 * @return true if export succeeded
	 */
	static bool ExportToXLSX(const TArray<FQuestTableRow>& Rows, const FString& FilePath, FString& OutError);

	/** Get the column definitions for Quest export (12 data columns + 3 sync columns) */
	static TArray<FQuestXLSXColumn> GetColumnDefinitions();

	/** Sentinel value for format detection - must match reader */
	static const FString SHEET_SENTINEL;

	/** Current format version */
	static const FString FORMAT_VERSION;

private:
	// XML Content Generators
	static FString GenerateContentTypesXml();
	static FString GenerateRelsXml();
	static FString GenerateWorkbookXml();
	static FString GenerateWorkbookRelsXml();
	static FString GenerateStylesXml();

	// Worksheet Generators
	static FString GenerateQuestsSheet(const TArray<FQuestTableRow>& Rows);
	static FString GenerateListsSheet(const TArray<FQuestTableRow>& Rows);
	static FString GenerateMetaSheet(const TArray<FQuestTableRow>& Rows);

	// Utility Helpers
	static FString EscapeXml(const FString& Text);
	static FString CellReference(int32 Col, int32 Row);
	static FString ColumnLetter(int32 ColIndex);
	static int64 ComputeRowHash(const FQuestTableRow& Row);
};
