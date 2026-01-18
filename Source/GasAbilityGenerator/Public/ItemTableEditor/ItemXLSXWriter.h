// GasAbilityGenerator - Item XLSX Writer
// v4.12: Export Item table to Excel format with sync support
//
// Creates XLSX files with:
// - Sentinel row for format detection (#ITEM_SHEET_V1)
// - Column ID mapping (immune to Excel column reorder)
// - _Lists sheet for dropdown validation
// - _Meta sheet for sync metadata (export GUID, timestamps, hashes)
//
// File structure mirrors QuestXLSXWriter for consistency.

#pragma once

#include "CoreMinimal.h"
#include "ItemTableEditorTypes.h"

/**
 * Column definition for Item XLSX export
 */
struct FItemXLSXColumn
{
	FString ColumnId;      // Machine ID used in sentinel row
	FString DisplayName;   // Human-readable header
	float Width;           // Column width in Excel character units

	FItemXLSXColumn(const FString& InId, const FString& InName, float InWidth = 15.0f)
		: ColumnId(InId), DisplayName(InName), Width(InWidth) {}
};

/**
 * Exports Item table data to XLSX format
 */
class GASABILITYGENERATOR_API FItemXLSXWriter
{
public:
	/**
	 * Export Item table to XLSX file
	 * @param Rows - Item rows to export
	 * @param FilePath - Output file path (.xlsx)
	 * @param OutError - Error message if export fails
	 * @return true if export succeeded
	 */
	static bool ExportToXLSX(const TArray<FItemTableRow>& Rows, const FString& FilePath, FString& OutError);

	/** Get the column definitions for Item export */
	static TArray<FItemXLSXColumn> GetColumnDefinitions();

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
	static FString GenerateItemsSheet(const TArray<FItemTableRow>& Rows);
	static FString GenerateListsSheet(const TArray<FItemTableRow>& Rows);
	static FString GenerateMetaSheet(const TArray<FItemTableRow>& Rows);

	// Utility Helpers
	static FString EscapeXml(const FString& Text);
	static FString CellReference(int32 Col, int32 Row);
	static FString ColumnLetter(int32 ColIndex);
	static int64 ComputeRowHash(const FItemTableRow& Row);
};
