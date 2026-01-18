// GasAbilityGenerator - Quest XLSX Reader Implementation
// v4.12: Import Quest table from Excel format

#include "QuestTableEditor/QuestXLSXReader.h"
#include "QuestTableEditor/QuestXLSXWriter.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FQuestXLSXReader::ImportFromXLSX(const FString& FilePath, TArray<FQuestTableRow>& OutRows, FString& OutError)
{
	// Extract XLSX to temp directory
	FString TempDir;
	if (!ExtractXLSX(FilePath, TempDir, OutError))
	{
		return false;
	}

	// Read sheet1.xml (Quests sheet)
	FString Sheet1Path = TempDir / TEXT("xl/worksheets/sheet1.xml");
	FString SheetXml;
	if (!FFileHelper::LoadFileToString(SheetXml, *Sheet1Path))
	{
		OutError = TEXT("Failed to read Quests sheet from XLSX");
		IFileManager::Get().DeleteDirectory(*TempDir, false, true);
		return false;
	}

	// Parse the sheet
	bool bSuccess = ParseQuestsSheet(SheetXml, OutRows, OutError);

	// Clean up temp directory
	IFileManager::Get().DeleteDirectory(*TempDir, false, true);

	return bSuccess;
}

bool FQuestXLSXReader::IsValidQuestXLSX(const FString& FilePath)
{
	FString TempDir;
	FString Error;
	if (!ExtractXLSX(FilePath, TempDir, Error))
	{
		return false;
	}

	FString Sheet1Path = TempDir / TEXT("xl/worksheets/sheet1.xml");
	FString SheetXml;
	if (!FFileHelper::LoadFileToString(SheetXml, *Sheet1Path))
	{
		IFileManager::Get().DeleteDirectory(*TempDir, false, true);
		return false;
	}

	// Check for sentinel marker
	bool bValid = SheetXml.Contains(FQuestXLSXWriter::SHEET_SENTINEL);

	IFileManager::Get().DeleteDirectory(*TempDir, false, true);
	return bValid;
}

bool FQuestXLSXReader::ReadMetadata(const FString& FilePath, FString& OutExportGuid, FString& OutTimestamp, uint32& OutContentHash)
{
	FString TempDir;
	FString Error;
	if (!ExtractXLSX(FilePath, TempDir, Error))
	{
		return false;
	}

	FString Sheet3Path = TempDir / TEXT("xl/worksheets/sheet3.xml");
	FString SheetXml;
	if (!FFileHelper::LoadFileToString(SheetXml, *Sheet3Path))
	{
		IFileManager::Get().DeleteDirectory(*TempDir, false, true);
		return false;
	}

	TMap<FString, FString> Metadata;
	ParseMetaSheet(SheetXml, Metadata);

	OutExportGuid = Metadata.FindRef(TEXT("ExportGuid"));
	OutTimestamp = Metadata.FindRef(TEXT("ExportTimestamp"));
	FString HashStr = Metadata.FindRef(TEXT("ContentHash"));
	OutContentHash = FCString::Atoi(*HashStr);

	IFileManager::Get().DeleteDirectory(*TempDir, false, true);
	return true;
}

bool FQuestXLSXReader::ExtractXLSX(const FString& FilePath, FString& OutTempDir, FString& OutError)
{
	OutTempDir = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("QuestXLSX_Read_"));
	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*OutTempDir, true);

	// Use system command to extract ZIP
	FString ExtractCommand;
#if PLATFORM_WINDOWS
	ExtractCommand = FString::Printf(TEXT("powershell -ExecutionPolicy Bypass -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\""),
		*FilePath, *OutTempDir);
#else
	ExtractCommand = FString::Printf(TEXT("unzip -o '%s' -d '%s'"), *FilePath, *OutTempDir);
#endif

	int32 ReturnCode = 0;
	FString StdOut, StdErr;
	FPlatformProcess::ExecProcess(*ExtractCommand, TEXT(""), &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode != 0)
	{
		OutError = FString::Printf(TEXT("Failed to extract XLSX: %s"), *StdErr);
		FM.DeleteDirectory(*OutTempDir, false, true);
		return false;
	}

	return true;
}

bool FQuestXLSXReader::ParseQuestsSheet(const FString& SheetXml, TArray<FQuestTableRow>& OutRows, FString& OutError)
{
	// Simple XML parsing - find all row elements and extract cell values
	// This is a simplified parser; production code would use proper XML parsing

	// Build column mapping from sentinel row
	TMap<int32, FString> ColumnMap;

	// Find rows in sheetData
	int32 SheetDataStart = SheetXml.Find(TEXT("<sheetData>"));
	int32 SheetDataEnd = SheetXml.Find(TEXT("</sheetData>"));
	if (SheetDataStart == INDEX_NONE || SheetDataEnd == INDEX_NONE)
	{
		OutError = TEXT("Invalid XLSX format: missing sheetData");
		return false;
	}

	FString SheetDataContent = SheetXml.Mid(SheetDataStart, SheetDataEnd - SheetDataStart);

	// Parse rows
	TArray<TMap<int32, FString>> AllRows;
	int32 RowStart = 0;
	while ((RowStart = SheetDataContent.Find(TEXT("<row"), ESearchCase::IgnoreCase, ESearchDir::FromStart, RowStart)) != INDEX_NONE)
	{
		int32 RowEnd = SheetDataContent.Find(TEXT("</row>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, RowStart);
		if (RowEnd == INDEX_NONE) break;

		FString RowContent = SheetDataContent.Mid(RowStart, RowEnd - RowStart + 6);
		TMap<int32, FString> RowCells;

		// Parse cells in this row
		int32 CellStart = 0;
		while ((CellStart = RowContent.Find(TEXT("<c "), ESearchCase::IgnoreCase, ESearchDir::FromStart, CellStart)) != INDEX_NONE)
		{
			int32 CellEnd = RowContent.Find(TEXT("</c>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CellStart);
			if (CellEnd == INDEX_NONE)
			{
				CellEnd = RowContent.Find(TEXT("/>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CellStart);
				if (CellEnd == INDEX_NONE) break;
			}

			FString CellContent = RowContent.Mid(CellStart, CellEnd - CellStart + 4);

			// Get cell reference (e.g., "A1")
			int32 RefStart = CellContent.Find(TEXT("r=\"")) + 3;
			int32 RefEnd = CellContent.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, RefStart);
			FString CellRef = CellContent.Mid(RefStart, RefEnd - RefStart);

			// Extract column letter from reference
			FString ColLetters;
			for (TCHAR Ch : CellRef)
			{
				if (FChar::IsAlpha(Ch)) ColLetters += Ch;
				else break;
			}
			int32 ColIdx = ColumnIndex(ColLetters);

			// Get cell value
			FString Value = ParseCellValue(CellContent);
			RowCells.Add(ColIdx, Value);

			CellStart = CellEnd + 1;
		}

		AllRows.Add(RowCells);
		RowStart = RowEnd + 1;
	}

	if (AllRows.Num() < 3)
	{
		OutError = TEXT("XLSX file has no data rows");
		return false;
	}

	// Row 0 is sentinel + column IDs
	TMap<int32, FString>& SentinelRow = AllRows[0];

	// Verify sentinel
	FString Sentinel = SentinelRow.FindRef(0);
	if (!Sentinel.Contains(TEXT("#QUEST_SHEET")))
	{
		OutError = TEXT("Invalid XLSX format: missing sentinel marker");
		return false;
	}

	// Build column ID to index mapping
	TMap<FString, int32> ColumnIdToIndex;
	for (const auto& Cell : SentinelRow)
	{
		if (Cell.Key > 0)  // Skip sentinel cell
		{
			ColumnIdToIndex.Add(Cell.Value, Cell.Key);
		}
	}

	// Row 1 is headers (skip)
	// Rows 2+ are data

	for (int32 RowIdx = 2; RowIdx < AllRows.Num(); RowIdx++)
	{
		TMap<int32, FString>& RowData = AllRows[RowIdx];

		FQuestTableRow NewRow;

		// Sync columns
		FString RowGuidStr = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("#ROW_GUID")));
		if (!RowGuidStr.IsEmpty())
		{
			FGuid::Parse(RowGuidStr, NewRow.RowId);
		}
		else
		{
			NewRow.RowId = FGuid::NewGuid();
		}

		FString StateStr = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("#STATE")));
		NewRow.bDeleted = StateStr.Equals(TEXT("DELETED"), ESearchCase::IgnoreCase);

		// Data columns
		NewRow.QuestName = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("QUEST_NAME")));
		NewRow.DisplayName = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("DISPLAY_NAME")));
		NewRow.StateID = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("STATE_ID")));
		NewRow.StateType = FQuestTableRow::ParseStateType(RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("STATE_TYPE"))));
		NewRow.Description = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("DESCRIPTION")));
		NewRow.ParentBranch = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("PARENT_BRANCH")));
		NewRow.Tasks = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("TASKS")));
		NewRow.Events = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("EVENTS")));
		NewRow.Conditions = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("CONDITIONS")));
		NewRow.Rewards = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("REWARDS")));
		NewRow.Notes = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("NOTES")));

		FString DeletedStr = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("DELETED")));
		if (!DeletedStr.IsEmpty())
		{
			NewRow.bDeleted = DeletedStr.Equals(TEXT("TRUE"), ESearchCase::IgnoreCase);
		}

		// Set status based on sync state
		NewRow.Status = EQuestTableRowStatus::Synced;

		// Read base hash for 3-way merge
		FString BaseHashStr = RowData.FindRef(ColumnIdToIndex.FindRef(TEXT("#BASE_HASH")));
		if (!BaseHashStr.IsEmpty())
		{
			NewRow.LastSyncedHash = FCString::Atoi64(*BaseHashStr);
		}

		OutRows.Add(NewRow);
	}

	return true;
}

bool FQuestXLSXReader::ParseMetaSheet(const FString& SheetXml, TMap<FString, FString>& OutMetadata)
{
	// Simple parsing: find key-value pairs in rows
	int32 RowStart = 0;
	while ((RowStart = SheetXml.Find(TEXT("<row"), ESearchCase::IgnoreCase, ESearchDir::FromStart, RowStart)) != INDEX_NONE)
	{
		int32 RowEnd = SheetXml.Find(TEXT("</row>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, RowStart);
		if (RowEnd == INDEX_NONE) break;

		FString RowContent = SheetXml.Mid(RowStart, RowEnd - RowStart);

		// Find cells A and B
		FString Key, Value;
		int32 CellStart = 0;
		while ((CellStart = RowContent.Find(TEXT("<c "), ESearchCase::IgnoreCase, ESearchDir::FromStart, CellStart)) != INDEX_NONE)
		{
			int32 CellEnd = RowContent.Find(TEXT("</c>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, CellStart);
			if (CellEnd == INDEX_NONE) break;

			FString CellContent = RowContent.Mid(CellStart, CellEnd - CellStart);

			// Get cell reference
			int32 RefStart = CellContent.Find(TEXT("r=\"")) + 3;
			int32 RefEnd = CellContent.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, RefStart);
			FString CellRef = CellContent.Mid(RefStart, RefEnd - RefStart);

			if (CellRef.StartsWith(TEXT("A")))
			{
				Key = ParseCellValue(CellContent);
			}
			else if (CellRef.StartsWith(TEXT("B")))
			{
				Value = ParseCellValue(CellContent);
			}

			CellStart = CellEnd + 1;
		}

		if (!Key.IsEmpty())
		{
			OutMetadata.Add(Key, Value);
		}

		RowStart = RowEnd + 1;
	}

	return true;
}

FString FQuestXLSXReader::ParseCellValue(const FString& CellXml)
{
	// Look for inline string: <is><t>value</t></is>
	int32 IsStart = CellXml.Find(TEXT("<is>"));
	if (IsStart != INDEX_NONE)
	{
		int32 TStart = CellXml.Find(TEXT("<t>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, IsStart);
		int32 TEnd = CellXml.Find(TEXT("</t>"), ESearchCase::IgnoreCase, ESearchDir::FromStart, TStart);
		if (TStart != INDEX_NONE && TEnd != INDEX_NONE)
		{
			FString Value = CellXml.Mid(TStart + 3, TEnd - TStart - 3);
			// Unescape XML entities
			Value.ReplaceInline(TEXT("&amp;"), TEXT("&"));
			Value.ReplaceInline(TEXT("&lt;"), TEXT("<"));
			Value.ReplaceInline(TEXT("&gt;"), TEXT(">"));
			Value.ReplaceInline(TEXT("&quot;"), TEXT("\""));
			Value.ReplaceInline(TEXT("&apos;"), TEXT("'"));
			return Value;
		}
	}

	// Look for value element: <v>value</v>
	int32 VStart = CellXml.Find(TEXT("<v>"));
	int32 VEnd = CellXml.Find(TEXT("</v>"));
	if (VStart != INDEX_NONE && VEnd != INDEX_NONE)
	{
		return CellXml.Mid(VStart + 3, VEnd - VStart - 3);
	}

	return FString();
}

int32 FQuestXLSXReader::ColumnIndex(const FString& ColumnLetter)
{
	int32 Index = 0;
	for (int32 i = 0; i < ColumnLetter.Len(); i++)
	{
		Index = Index * 26 + (ColumnLetter[i] - TEXT('A') + 1);
	}
	return Index - 1;  // 0-based
}
