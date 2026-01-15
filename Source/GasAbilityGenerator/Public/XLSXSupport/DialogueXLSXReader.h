// GasAbilityGenerator - Dialogue XLSX Reader
// v4.3: Import dialogue table from Excel format with sync support
//
// Parses XLSX files with:
// - Sentinel row detection (#DIALOGUE_SHEET_V1)
// - Column ID mapping (immune to Excel reorder)
// - Row GUID preservation for sync

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"

/**
 * Result of XLSX import operation
 */
struct FDialogueXLSXImportResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TArray<FDialogueTableRow> Rows;

	// Metadata from _Meta sheet
	FString ExportGuid;
	FString ExportedAt;
	FString FormatVersion;
	int32 OriginalRowCount = 0;
	int64 ContentHash = 0;
};

/**
 * Imports dialogue table data from XLSX format
 */
class GASABILITYGENERATOR_API FDialogueXLSXReader
{
public:
	/**
	 * Import dialogue table from XLSX file
	 * @param FilePath - Input file path (.xlsx)
	 * @return Import result with rows and metadata
	 */
	static FDialogueXLSXImportResult ImportFromXLSX(const FString& FilePath);

	/** Expected sentinel value for format detection */
	static const FString EXPECTED_SENTINEL;

private:
	// ZIP extraction
	static bool ExtractZipContents(const FString& FilePath, TMap<FString, TArray<uint8>>& OutFiles, FString& OutError);

	// XML parsing
	static bool ParseDialoguesSheet(const TArray<uint8>& XmlData, TArray<FDialogueTableRow>& OutRows, FString& OutError);
	static bool ParseMetaSheet(const TArray<uint8>& XmlData, FDialogueXLSXImportResult& OutResult);

	// Cell extraction helpers
	static TArray<TArray<FString>> ParseSheetToGrid(const TArray<uint8>& XmlData);
	static FString GetCellValue(const FString& CellXml);
	static int32 ColumnLetterToIndex(const FString& Letter);
	static void ParseCellReference(const FString& CellRef, int32& OutCol, int32& OutRow);

	// Row conversion
	static FDialogueTableRow ParseRowFromValues(const TMap<FString, int32>& ColumnMap, const TArray<FString>& Values);
};
