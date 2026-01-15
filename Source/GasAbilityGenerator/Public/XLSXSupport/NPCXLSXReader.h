// GasAbilityGenerator - NPC XLSX Reader
// v4.4: Import NPC table from Excel format with sync support
//
// Parses XLSX files with:
// - Sentinel row detection (#NPC_SHEET_V1)
// - Column ID mapping (immune to Excel column reorder)
// - Row GUID preservation for stable identity during sync
// - Metadata extraction from _Meta sheet
//
// The reader is tolerant of:
// - Missing columns (uses defaults)
// - Extra columns (ignored)
// - Reordered columns (mapped by ID, not position)
// - Empty rows (skipped)

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"

/**
 * Result of NPC XLSX import operation
 * Contains imported rows and sync metadata
 */
struct FNPCXLSXImportResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TArray<FNPCTableRow> Rows;

	// Metadata from _Meta sheet (used for sync)
	FString ExportGuid;           // GUID from original export
	FString ExportedAt;           // Timestamp of original export
	FString FormatVersion;        // Format version for compatibility
	int32 OriginalRowCount = 0;   // Row count at export time
	int64 ContentHash = 0;        // Content hash at export time
};

/**
 * Imports NPC table data from XLSX format
 *
 * Import process:
 * 1. Extract ZIP contents to memory
 * 2. Parse sheet1.xml (NPCs)
 *    - Detect sentinel row (#NPC_SHEET_V1)
 *    - Build column ID -> index mapping
 *    - Parse data rows using column mapping
 * 3. Parse sheet3.xml (_Meta) for sync metadata
 *
 * Column mapping makes import robust against:
 * - User adding columns in Excel
 * - User reordering columns
 * - User renaming human-readable headers (row 2)
 */
class GASABILITYGENERATOR_API FNPCXLSXReader
{
public:
	/**
	 * Import NPC table from XLSX file
	 * @param FilePath - Input file path (.xlsx)
	 * @return Import result with rows and metadata
	 */
	static FNPCXLSXImportResult ImportFromXLSX(const FString& FilePath);

	/** Expected sentinel value - must match writer */
	static const FString EXPECTED_SENTINEL;

private:
	//=========================================================================
	// ZIP Extraction
	//=========================================================================

	/**
	 * Extract all files from XLSX (ZIP) container
	 * @param FilePath - Path to .xlsx file
	 * @param OutFiles - Map of internal path -> file contents
	 * @param OutError - Error message on failure
	 * @return true if extraction succeeded
	 */
	static bool ExtractZipContents(const FString& FilePath, TMap<FString, TArray<uint8>>& OutFiles, FString& OutError);

	//=========================================================================
	// XML Sheet Parsing
	//=========================================================================

	/**
	 * Parse the NPCs sheet (sheet1.xml)
	 * @param XmlData - Raw XML bytes
	 * @param OutRows - Parsed NPC rows
	 * @param OutError - Error message on failure
	 * @return true if parsing succeeded
	 */
	static bool ParseNPCsSheet(const TArray<uint8>& XmlData, TArray<FNPCTableRow>& OutRows, FString& OutError);

	/**
	 * Parse the _Meta sheet (sheet3.xml) for sync metadata
	 * @param XmlData - Raw XML bytes
	 * @param OutResult - Import result to populate with metadata
	 * @return true if parsing succeeded
	 */
	static bool ParseMetaSheet(const TArray<uint8>& XmlData, FNPCXLSXImportResult& OutResult);

	//=========================================================================
	// Cell/Grid Helpers
	//=========================================================================

	/**
	 * Parse XML sheet data into a 2D string grid
	 * Handles sparse cell data (empty cells in the middle)
	 * @param XmlData - Raw XML bytes
	 * @return 2D grid [row][col] of cell values
	 */
	static TArray<TArray<FString>> ParseSheetToGrid(const TArray<uint8>& XmlData);

	/**
	 * Convert Excel column letters to 0-based index
	 * "A" -> 0, "Z" -> 25, "AA" -> 26, etc.
	 */
	static int32 ColumnLetterToIndex(const FString& Letter);

	/**
	 * Parse Excel cell reference (e.g., "AA123") into column index and row number
	 * @param CellRef - Cell reference string (e.g., "B5")
	 * @param OutCol - 0-based column index
	 * @param OutRow - 1-based row number
	 */
	static void ParseCellReference(const FString& CellRef, int32& OutCol, int32& OutRow);

	//=========================================================================
	// Row Conversion
	//=========================================================================

	/**
	 * Convert a row of string values to FNPCTableRow
	 * Uses column map to find values by column ID (position-independent)
	 * @param ColumnMap - Map of column ID -> column index
	 * @param Values - Row values array
	 * @return Populated NPC row
	 */
	static FNPCTableRow ParseRowFromValues(const TMap<FString, int32>& ColumnMap, const TArray<FString>& Values);
};
