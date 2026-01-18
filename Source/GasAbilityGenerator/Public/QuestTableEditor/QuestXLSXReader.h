// GasAbilityGenerator - Quest XLSX Reader
// v4.12: Import Quest table from Excel format with validation

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditorTypes.h"

/**
 * Reads Quest table data from XLSX format
 */
class GASABILITYGENERATOR_API FQuestXLSXReader
{
public:
	/**
	 * Import Quest table from XLSX file
	 * @param FilePath - Input file path (.xlsx)
	 * @param OutRows - Parsed quest rows
	 * @param OutError - Error message if import fails
	 * @return true if import succeeded
	 */
	static bool ImportFromXLSX(const FString& FilePath, TArray<FQuestTableRow>& OutRows, FString& OutError);

	/**
	 * Validate that file is a valid Quest XLSX
	 * @param FilePath - File path to validate
	 * @return true if file has correct sentinel marker
	 */
	static bool IsValidQuestXLSX(const FString& FilePath);

	/**
	 * Read metadata from Quest XLSX
	 * @param FilePath - File path
	 * @param OutExportGuid - Export GUID from file
	 * @param OutTimestamp - Export timestamp
	 * @param OutContentHash - Content hash at export time
	 * @return true if metadata was read successfully
	 */
	static bool ReadMetadata(const FString& FilePath, FString& OutExportGuid, FString& OutTimestamp, uint32& OutContentHash);

private:
	/** Extract ZIP archive to temp directory */
	static bool ExtractXLSX(const FString& FilePath, FString& OutTempDir, FString& OutError);

	/** Parse sheet1.xml (Quests sheet) */
	static bool ParseQuestsSheet(const FString& SheetXml, TArray<FQuestTableRow>& OutRows, FString& OutError);

	/** Parse sheet3.xml (Meta sheet) */
	static bool ParseMetaSheet(const FString& SheetXml, TMap<FString, FString>& OutMetadata);

	/** Parse a single cell value from XML */
	static FString ParseCellValue(const FString& CellXml);

	/** Convert column letter to index (A=0, B=1, AA=26) */
	static int32 ColumnIndex(const FString& ColumnLetter);
};
