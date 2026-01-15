// GasAbilityGenerator - NPC XLSX Writer
// v4.4: Export NPC table to Excel format with sync support
//
// Creates XLSX files with:
// - Sentinel row for format detection (#NPC_SHEET_V1)
// - Column ID mapping (immune to Excel column reorder)
// - _Lists sheet for dropdown validation (Factions, Items, POIs)
// - _Meta sheet for sync metadata (export GUID, timestamps, hashes)
//
// File structure mirrors DialogueXLSXWriter for consistency.

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"

/**
 * Column definition for NPC XLSX export
 * Maps machine-readable IDs to human-friendly display names
 */
struct FNPCXLSXColumn
{
	FString ColumnId;      // Machine ID used in sentinel row (e.g., "NPC_NAME")
	FString DisplayName;   // Human-readable header (e.g., "NPC Name")
	float Width;           // Column width in Excel character units

	FNPCXLSXColumn(const FString& InId, const FString& InName, float InWidth = 15.0f)
		: ColumnId(InId), DisplayName(InName), Width(InWidth) {}
};

/**
 * Exports NPC table data to XLSX format
 *
 * XLSX file structure (ZIP with XML files):
 * - [Content_Types].xml         - MIME type declarations
 * - _rels/.rels                 - Package relationships
 * - xl/workbook.xml             - Workbook definition (sheet list)
 * - xl/worksheets/sheet1.xml    - NPCs (main data sheet)
 * - xl/worksheets/sheet2.xml    - _Lists (dropdown values for validation)
 * - xl/worksheets/sheet3.xml    - _Meta (sync metadata)
 * - xl/_rels/workbook.xml.rels  - Workbook relationships
 * - xl/styles.xml               - Cell styles (bold headers, etc.)
 *
 * Sheet1 Layout:
 * - Row 1: Sentinel (#NPC_SHEET_V1) + Column IDs
 * - Row 2: Human-readable column headers (bold)
 * - Row 3+: Data rows
 *
 * Safety features:
 * - #ROW_GUID column for stable row identity across edits
 * - #STATE column for explicit deletion tracking
 * - #BASE_HASH column for 3-way merge baseline
 */
class GASABILITYGENERATOR_API FNPCXLSXWriter
{
public:
	/**
	 * Export NPC table to XLSX file
	 * @param Rows - NPC rows to export
	 * @param FilePath - Output file path (.xlsx)
	 * @param OutError - Error message if export fails
	 * @return true if export succeeded
	 */
	static bool ExportToXLSX(const TArray<FNPCTableRow>& Rows, const FString& FilePath, FString& OutError);

	/** Get the column definitions for NPC export (17 data columns + 3 sync columns) */
	static TArray<FNPCXLSXColumn> GetColumnDefinitions();

	/** Sentinel value for format detection - must match reader */
	static const FString SHEET_SENTINEL;

	/** Current format version for compatibility checking */
	static const FString FORMAT_VERSION;

private:
	//=========================================================================
	// XML Content Generators - XLSX structure files
	//=========================================================================

	/** Generate [Content_Types].xml - MIME type declarations for all parts */
	static FString GenerateContentTypesXml();

	/** Generate _rels/.rels - Root relationships pointing to workbook */
	static FString GenerateRelsXml();

	/** Generate xl/workbook.xml - Workbook with sheet definitions */
	static FString GenerateWorkbookXml();

	/** Generate xl/_rels/workbook.xml.rels - Sheet and style relationships */
	static FString GenerateWorkbookRelsXml();

	/** Generate xl/styles.xml - Cell formatting (bold headers) */
	static FString GenerateStylesXml();

	//=========================================================================
	// Worksheet Generators
	//=========================================================================

	/** Generate sheet1.xml - Main NPC data with sentinel row and column mapping */
	static FString GenerateNPCsSheet(const TArray<FNPCTableRow>& Rows);

	/** Generate sheet2.xml - Lists of unique values for dropdown validation */
	static FString GenerateListsSheet(const TArray<FNPCTableRow>& Rows);

	/** Generate sheet3.xml - Sync metadata (export GUID, timestamp, content hash) */
	static FString GenerateMetaSheet(const TArray<FNPCTableRow>& Rows);

	//=========================================================================
	// Utility Helpers
	//=========================================================================

	/** Escape special XML characters (&, <, >, ", ') */
	static FString EscapeXml(const FString& Text);

	/** Convert column index (0-based) and row number to Excel cell reference (e.g., "A1", "AA100") */
	static FString CellReference(int32 Col, int32 Row);

	/** Convert 0-based column index to Excel letter(s) (0="A", 25="Z", 26="AA") */
	static FString ColumnLetter(int32 ColIndex);

	/** Compute content hash for a row (excludes RowId for identity-independent comparison) */
	static int64 ComputeRowHash(const FNPCTableRow& Row);

	/** Get asset name from FSoftObjectPath, or empty string if null */
	static FString GetAssetName(const FSoftObjectPath& Path);
};
