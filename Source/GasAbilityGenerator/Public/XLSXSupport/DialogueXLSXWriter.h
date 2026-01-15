// GasAbilityGenerator - Dialogue XLSX Writer
// v4.4 Phase 3: Export dialogue table to Excel format with asset sync support
//
// Creates XLSX files with:
// - Sentinel row for format detection
// - Column ID mapping (immune to Excel reorder)
// - _Lists sheet for dropdown validation
// - _Meta sheet for sync metadata
// - [RO] columns showing current UE asset state

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"

struct FDialogueAssetSyncResult;

/**
 * Column definition for XLSX export
 */
struct FDialogueXLSXColumn
{
	FString ColumnId;      // Machine ID (e.g., "NODE_ID")
	FString DisplayName;   // Human header (e.g., "Node ID")
	float Width;           // Column width in characters

	FDialogueXLSXColumn(const FString& InId, const FString& InName, float InWidth = 15.0f)
		: ColumnId(InId), DisplayName(InName), Width(InWidth) {}
};

/**
 * Exports dialogue table data to XLSX format
 *
 * File structure:
 * - [Content_Types].xml
 * - _rels/.rels
 * - xl/workbook.xml
 * - xl/worksheets/sheet1.xml (Dialogues)
 * - xl/worksheets/sheet2.xml (_Lists)
 * - xl/worksheets/sheet3.xml (_Meta)
 * - xl/_rels/workbook.xml.rels
 * - xl/styles.xml
 */
class GASABILITYGENERATOR_API FDialogueXLSXWriter
{
public:
	/**
	 * Export dialogue table to XLSX file
	 * @param Rows - Dialogue rows to export
	 * @param FilePath - Output file path (.xlsx)
	 * @param OutError - Error message if export fails
	 * @return true if export succeeded
	 */
	static bool ExportToXLSX(const TArray<FDialogueTableRow>& Rows, const FString& FilePath, FString& OutError);

	/**
	 * Export dialogue table to XLSX file with asset sync data for [RO] columns
	 * @param Rows - Dialogue rows to export
	 * @param FilePath - Output file path (.xlsx)
	 * @param AssetSync - Asset sync result containing current UE events/conditions
	 * @param OutError - Error message if export fails
	 * @return true if export succeeded
	 */
	static bool ExportToXLSX(const TArray<FDialogueTableRow>& Rows, const FString& FilePath,
		const FDialogueAssetSyncResult& AssetSync, FString& OutError);

	/** Get the column definitions for dialogue export */
	static TArray<FDialogueXLSXColumn> GetColumnDefinitions();

	/** Sentinel value for format detection */
	static const FString SHEET_SENTINEL;

	/** Current format version */
	static const FString FORMAT_VERSION;

private:
	// XML content generators
	static FString GenerateContentTypesXml();
	static FString GenerateRelsXml();
	static FString GenerateWorkbookXml();
	static FString GenerateWorkbookRelsXml();
	static FString GenerateStylesXml();

	// Sheet generators
	static FString GenerateDialoguesSheet(const TArray<FDialogueTableRow>& Rows);
	static FString GenerateDialoguesSheet(const TArray<FDialogueTableRow>& Rows, const FDialogueAssetSyncResult* AssetSync);
	static FString GenerateListsSheet(const TArray<FDialogueTableRow>& Rows);
	static FString GenerateMetaSheet(const TArray<FDialogueTableRow>& Rows);

	// Helpers
	static FString EscapeXml(const FString& Text);
	static FString CellReference(int32 Col, int32 Row);
	static FString ColumnLetter(int32 ColIndex);
	static int64 ComputeRowHash(const FDialogueTableRow& Row);
};
