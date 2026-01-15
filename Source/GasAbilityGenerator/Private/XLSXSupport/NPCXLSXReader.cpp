// GasAbilityGenerator - NPC XLSX Reader Implementation
// v4.4: Import NPC table from Excel format
//
// Uses UE's XML parser (FXmlFile) and ZIP reader (FZipArchiveReader).
// Designed to be robust against user modifications in Excel.

#include "XLSXSupport/NPCXLSXReader.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "XmlFile.h"

#if WITH_EDITOR
#include "FileUtilities/ZipArchiveReader.h"
#endif

//=============================================================================
// Static Constants
//=============================================================================

const FString FNPCXLSXReader::EXPECTED_SENTINEL = TEXT("#NPC_SHEET_V1");

//=============================================================================
// Main Import Function
//=============================================================================

FNPCXLSXImportResult FNPCXLSXReader::ImportFromXLSX(const FString& FilePath)
{
	FNPCXLSXImportResult Result;

#if WITH_EDITOR
	// Step 1: Extract ZIP contents
	TMap<FString, TArray<uint8>> ZipContents;
	if (!ExtractZipContents(FilePath, ZipContents, Result.ErrorMessage))
	{
		return Result;
	}

	// Step 2: Find and parse the NPCs sheet (sheet1.xml)
	TArray<uint8>* Sheet1Data = ZipContents.Find(TEXT("xl/worksheets/sheet1.xml"));
	if (!Sheet1Data)
	{
		Result.ErrorMessage = TEXT("Could not find NPCs sheet (xl/worksheets/sheet1.xml)");
		return Result;
	}

	if (!ParseNPCsSheet(*Sheet1Data, Result.Rows, Result.ErrorMessage))
	{
		return Result;
	}

	// Step 3: Parse metadata if available (optional - file still valid without it)
	TArray<uint8>* Sheet3Data = ZipContents.Find(TEXT("xl/worksheets/sheet3.xml"));
	if (Sheet3Data)
	{
		ParseMetaSheet(*Sheet3Data, Result);
	}

	Result.bSuccess = true;
#else
	Result.ErrorMessage = TEXT("XLSX import requires WITH_EDITOR (Editor builds only)");
#endif

	return Result;
}

//=============================================================================
// ZIP Extraction
//=============================================================================

#if WITH_EDITOR
bool FNPCXLSXReader::ExtractZipContents(const FString& FilePath, TMap<FString, TArray<uint8>>& OutFiles, FString& OutError)
{
	// Open the XLSX file (which is a ZIP container)
	IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FilePath);
	if (!FileHandle)
	{
		OutError = FString::Printf(TEXT("Could not open file: %s"), *FilePath);
		return false;
	}

	// Create ZIP reader
	FZipArchiveReader ZipReader(FileHandle);
	if (!ZipReader.IsValid())
	{
		OutError = TEXT("Invalid or corrupt XLSX file (not a valid ZIP container)");
		return false;
	}

	// Extract all files to memory
	TArray<FString> FileNames = ZipReader.GetFileNames();
	for (const FString& FileName : FileNames)
	{
		TArray<uint8> FileData;
		if (ZipReader.TryReadFile(FileName, FileData))
		{
			OutFiles.Add(FileName, MoveTemp(FileData));
		}
	}

	return true;
}
#endif

//=============================================================================
// Sheet Parsing
//=============================================================================

bool FNPCXLSXReader::ParseNPCsSheet(const TArray<uint8>& XmlData, TArray<FNPCTableRow>& OutRows, FString& OutError)
{
	// Parse XML to 2D grid
	TArray<TArray<FString>> Grid = ParseSheetToGrid(XmlData);

	// Validate minimum structure (sentinel + headers + at least potential for data)
	if (Grid.Num() < 2)
	{
		OutError = TEXT("Sheet has fewer than 2 rows (need sentinel row + header row)");
		return false;
	}

	//-------------------------------------------------------------------------
	// Row 1: Sentinel + Column IDs
	//-------------------------------------------------------------------------
	const TArray<FString>& SentinelRow = Grid[0];
	if (SentinelRow.Num() == 0 || SentinelRow[0] != EXPECTED_SENTINEL)
	{
		OutError = FString::Printf(TEXT("Invalid sentinel. Expected '%s', got '%s'"),
			*EXPECTED_SENTINEL, SentinelRow.Num() > 0 ? *SentinelRow[0] : TEXT("(empty)"));
		return false;
	}

	// Build column ID -> index mapping from sentinel row
	// This makes parsing robust against column reordering
	TMap<FString, int32> ColumnMap;
	for (int32 i = 0; i < SentinelRow.Num(); i++)
	{
		if (!SentinelRow[i].IsEmpty())
		{
			ColumnMap.Add(SentinelRow[i], i);
		}
	}

	//-------------------------------------------------------------------------
	// Row 2: Human headers (skipped - we use column IDs from row 1)
	// Row 3+: Data rows
	//-------------------------------------------------------------------------
	for (int32 RowIdx = 2; RowIdx < Grid.Num(); RowIdx++)
	{
		const TArray<FString>& RowValues = Grid[RowIdx];

		// Skip completely empty rows
		bool bAllEmpty = true;
		for (const FString& Val : RowValues)
		{
			if (!Val.IsEmpty())
			{
				bAllEmpty = false;
				break;
			}
		}
		if (bAllEmpty) continue;

		// Parse row using column mapping
		FNPCTableRow NPCRow = ParseRowFromValues(ColumnMap, RowValues);

		// Only add rows that have at least an NPC name (basic validity check)
		if (!NPCRow.NPCName.IsEmpty())
		{
			OutRows.Add(NPCRow);
		}
	}

	return true;
}

bool FNPCXLSXReader::ParseMetaSheet(const TArray<uint8>& XmlData, FNPCXLSXImportResult& OutResult)
{
	TArray<TArray<FString>> Grid = ParseSheetToGrid(XmlData);

	// Parse key-value pairs (Property in col A, Value in col B)
	// Skip header row (index 0)
	for (int32 i = 1; i < Grid.Num(); i++)
	{
		if (Grid[i].Num() >= 2)
		{
			const FString& Key = Grid[i][0];
			const FString& Value = Grid[i][1];

			if (Key == TEXT("EXPORT_GUID"))
			{
				OutResult.ExportGuid = Value;
			}
			else if (Key == TEXT("EXPORTED_AT"))
			{
				OutResult.ExportedAt = Value;
			}
			else if (Key == TEXT("FORMAT_VERSION"))
			{
				OutResult.FormatVersion = Value;
			}
			else if (Key == TEXT("ROW_COUNT"))
			{
				OutResult.OriginalRowCount = FCString::Atoi(*Value);
			}
			else if (Key == TEXT("CONTENT_HASH"))
			{
				OutResult.ContentHash = FCString::Atoi64(*Value);
			}
		}
	}

	return true;
}

//=============================================================================
// XML Grid Parsing
//=============================================================================

TArray<TArray<FString>> FNPCXLSXReader::ParseSheetToGrid(const TArray<uint8>& XmlData)
{
	TArray<TArray<FString>> Grid;

	// Convert raw bytes to string
	FString XmlString;
	FFileHelper::BufferToString(XmlString, XmlData.GetData(), XmlData.Num());

	// Parse XML
	FXmlFile XmlFile(XmlString, EConstructMethod::ConstructFromBuffer);
	if (!XmlFile.IsValid())
	{
		return Grid;
	}

	const FXmlNode* RootNode = XmlFile.GetRootNode();
	if (!RootNode)
	{
		return Grid;
	}

	// Find sheetData node (contains all rows)
	const FXmlNode* SheetDataNode = RootNode->FindChildNode(TEXT("sheetData"));
	if (!SheetDataNode)
	{
		return Grid;
	}

	//-------------------------------------------------------------------------
	// First pass: determine grid dimensions
	//-------------------------------------------------------------------------
	int32 MaxRow = 0;
	int32 MaxCol = 0;

	for (const FXmlNode* RowNode : SheetDataNode->GetChildrenNodes())
	{
		if (RowNode->GetTag() != TEXT("row")) continue;

		int32 RowNum = FCString::Atoi(*RowNode->GetAttribute(TEXT("r")));
		MaxRow = FMath::Max(MaxRow, RowNum);

		for (const FXmlNode* CellNode : RowNode->GetChildrenNodes())
		{
			if (CellNode->GetTag() != TEXT("c")) continue;

			FString CellRef = CellNode->GetAttribute(TEXT("r"));
			int32 Col, Row;
			ParseCellReference(CellRef, Col, Row);
			MaxCol = FMath::Max(MaxCol, Col + 1);
		}
	}

	//-------------------------------------------------------------------------
	// Initialize grid with empty strings
	//-------------------------------------------------------------------------
	Grid.SetNum(MaxRow);
	for (int32 i = 0; i < MaxRow; i++)
	{
		Grid[i].SetNum(MaxCol);
	}

	//-------------------------------------------------------------------------
	// Second pass: fill grid with cell values
	//-------------------------------------------------------------------------
	for (const FXmlNode* RowNode : SheetDataNode->GetChildrenNodes())
	{
		if (RowNode->GetTag() != TEXT("row")) continue;

		int32 RowNum = FCString::Atoi(*RowNode->GetAttribute(TEXT("r")));
		int32 RowIdx = RowNum - 1;  // Convert to 0-based index

		for (const FXmlNode* CellNode : RowNode->GetChildrenNodes())
		{
			if (CellNode->GetTag() != TEXT("c")) continue;

			FString CellRef = CellNode->GetAttribute(TEXT("r"));
			int32 ColIdx, RowFromRef;
			ParseCellReference(CellRef, ColIdx, RowFromRef);

			// Extract cell value (supports both inline strings and value nodes)
			FString CellValue;
			FString CellType = CellNode->GetAttribute(TEXT("t"));

			// Check for inline string <is><t>value</t></is>
			const FXmlNode* InlineStrNode = CellNode->FindChildNode(TEXT("is"));
			const FXmlNode* ValueNode = CellNode->FindChildNode(TEXT("v"));

			if (InlineStrNode)
			{
				const FXmlNode* TextNode = InlineStrNode->FindChildNode(TEXT("t"));
				if (TextNode)
				{
					CellValue = TextNode->GetContent();
				}
			}
			else if (ValueNode)
			{
				CellValue = ValueNode->GetContent();
			}

			// Store in grid if within bounds
			if (RowIdx >= 0 && RowIdx < Grid.Num() && ColIdx >= 0 && ColIdx < Grid[RowIdx].Num())
			{
				Grid[RowIdx][ColIdx] = CellValue;
			}
		}
	}

	return Grid;
}

//=============================================================================
// Cell Reference Helpers
//=============================================================================

int32 FNPCXLSXReader::ColumnLetterToIndex(const FString& Letter)
{
	// Convert Excel column letters to 0-based index
	// "A" = 0, "Z" = 25, "AA" = 26, "AB" = 27, etc.
	int32 Index = 0;
	for (int32 i = 0; i < Letter.Len(); i++)
	{
		TCHAR Ch = Letter[i];
		if (Ch >= TEXT('A') && Ch <= TEXT('Z'))
		{
			Index = Index * 26 + (Ch - TEXT('A') + 1);
		}
		else if (Ch >= TEXT('a') && Ch <= TEXT('z'))
		{
			Index = Index * 26 + (Ch - TEXT('a') + 1);
		}
	}
	return Index - 1;  // Convert to 0-based
}

void FNPCXLSXReader::ParseCellReference(const FString& CellRef, int32& OutCol, int32& OutRow)
{
	// Split cell reference (e.g., "AA123") into column letters and row number
	FString ColPart;
	FString RowPart;

	for (int32 i = 0; i < CellRef.Len(); i++)
	{
		TCHAR Ch = CellRef[i];
		if ((Ch >= TEXT('A') && Ch <= TEXT('Z')) || (Ch >= TEXT('a') && Ch <= TEXT('z')))
		{
			ColPart.AppendChar(Ch);
		}
		else if (Ch >= TEXT('0') && Ch <= TEXT('9'))
		{
			RowPart.AppendChar(Ch);
		}
	}

	OutCol = ColumnLetterToIndex(ColPart);
	OutRow = FCString::Atoi(*RowPart);
}

//=============================================================================
// Row Conversion
//=============================================================================

FNPCTableRow FNPCXLSXReader::ParseRowFromValues(const TMap<FString, int32>& ColumnMap, const TArray<FString>& Values)
{
	FNPCTableRow Row;

	// Helper lambda to safely get value by column ID
	auto GetValue = [&ColumnMap, &Values](const FString& ColumnId) -> FString
	{
		const int32* IndexPtr = ColumnMap.Find(ColumnId);
		if (IndexPtr && *IndexPtr >= 0 && *IndexPtr < Values.Num())
		{
			return Values[*IndexPtr];
		}
		return FString();
	};

	//-------------------------------------------------------------------------
	// Parse Row GUID (for sync identity)
	//-------------------------------------------------------------------------
	FString RowIdStr = GetValue(TEXT("#ROW_GUID"));
	if (!RowIdStr.IsEmpty())
	{
		FGuid::Parse(RowIdStr, Row.RowId);
	}
	if (!Row.RowId.IsValid())
	{
		// Generate new GUID if missing or invalid (new row from Excel)
		Row.RowId = FGuid::NewGuid();
	}

	//-------------------------------------------------------------------------
	// Core Identity
	//-------------------------------------------------------------------------
	Row.NPCName = GetValue(TEXT("NPC_NAME"));
	Row.NPCId = GetValue(TEXT("NPC_ID"));
	Row.DisplayName = GetValue(TEXT("DISPLAY_NAME"));

	// Asset paths - stored as asset names in XLSX, will need resolution later
	FString BlueprintName = GetValue(TEXT("BLUEPRINT"));
	if (!BlueprintName.IsEmpty())
	{
		// Store just the name for now - UI will resolve to full path
		Row.Blueprint = FSoftObjectPath(BlueprintName);
	}

	//-------------------------------------------------------------------------
	// AI & Behavior
	//-------------------------------------------------------------------------
	FString AbilityConfigName = GetValue(TEXT("ABILITY_CONFIG"));
	if (!AbilityConfigName.IsEmpty())
	{
		Row.AbilityConfig = FSoftObjectPath(AbilityConfigName);
	}

	FString ActivityConfigName = GetValue(TEXT("ACTIVITY_CONFIG"));
	if (!ActivityConfigName.IsEmpty())
	{
		Row.ActivityConfig = FSoftObjectPath(ActivityConfigName);
	}

	FString ScheduleName = GetValue(TEXT("SCHEDULE"));
	if (!ScheduleName.IsEmpty())
	{
		Row.Schedule = FSoftObjectPath(ScheduleName);
	}

	FString BehaviorTreeName = GetValue(TEXT("BEHAVIOR_TREE"));
	if (!BehaviorTreeName.IsEmpty())
	{
		Row.BehaviorTree = FSoftObjectPath(BehaviorTreeName);
	}

	//-------------------------------------------------------------------------
	// Combat
	//-------------------------------------------------------------------------
	FString MinLevelStr = GetValue(TEXT("MIN_LEVEL"));
	if (!MinLevelStr.IsEmpty())
	{
		Row.MinLevel = FCString::Atoi(*MinLevelStr);
	}

	FString MaxLevelStr = GetValue(TEXT("MAX_LEVEL"));
	if (!MaxLevelStr.IsEmpty())
	{
		Row.MaxLevel = FCString::Atoi(*MaxLevelStr);
	}

	// Factions - stored as short names, convert to full tags
	FString FactionsStr = GetValue(TEXT("FACTIONS"));
	if (!FactionsStr.IsEmpty())
	{
		Row.SetFactionsFromDisplay(FactionsStr);
	}

	FString AttackPriorityStr = GetValue(TEXT("ATTACK_PRIORITY"));
	if (!AttackPriorityStr.IsEmpty())
	{
		Row.AttackPriority = FCString::Atof(*AttackPriorityStr);
	}

	//-------------------------------------------------------------------------
	// Vendor
	//-------------------------------------------------------------------------
	FString IsVendorStr = GetValue(TEXT("IS_VENDOR"));
	Row.bIsVendor = IsVendorStr.Equals(TEXT("Yes"), ESearchCase::IgnoreCase);

	Row.ShopName = GetValue(TEXT("SHOP_NAME"));

	//-------------------------------------------------------------------------
	// Items & Spawning
	//-------------------------------------------------------------------------
	Row.DefaultItems = GetValue(TEXT("DEFAULT_ITEMS"));
	Row.SpawnerPOI = GetValue(TEXT("SPAWNER_POI"));

	//-------------------------------------------------------------------------
	// Meta
	//-------------------------------------------------------------------------
	FString AppearanceName = GetValue(TEXT("APPEARANCE"));
	if (!AppearanceName.IsEmpty())
	{
		Row.Appearance = FSoftObjectPath(AppearanceName);
	}

	Row.Notes = GetValue(TEXT("NOTES"));

	//-------------------------------------------------------------------------
	// Sync state - mark as potentially modified (sync engine will verify)
	//-------------------------------------------------------------------------
	FString StateStr = GetValue(TEXT("#STATE"));
	if (StateStr.Equals(TEXT("Deleted"), ESearchCase::IgnoreCase))
	{
		// Will be handled by sync engine - row marked for deletion
		Row.Status = ENPCTableRowStatus::Error;  // Temporary marker
	}
	else
	{
		Row.Status = ENPCTableRowStatus::Modified;  // Assume modified until sync verifies
	}

	return Row;
}
