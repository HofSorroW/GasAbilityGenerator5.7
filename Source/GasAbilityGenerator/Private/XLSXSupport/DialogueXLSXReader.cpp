// GasAbilityGenerator - Dialogue XLSX Reader Implementation
// v4.3: Import dialogue table from Excel format

#include "XLSXSupport/DialogueXLSXReader.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "XmlFile.h"

#if WITH_EDITOR
#include "FileUtilities/ZipArchiveReader.h"
#endif

const FString FDialogueXLSXReader::EXPECTED_SENTINEL = TEXT("#DIALOGUE_SHEET_V1");

FDialogueXLSXImportResult FDialogueXLSXReader::ImportFromXLSX(const FString& FilePath)
{
	FDialogueXLSXImportResult Result;

#if WITH_EDITOR
	// Extract ZIP contents
	TMap<FString, TArray<uint8>> ZipContents;
	if (!ExtractZipContents(FilePath, ZipContents, Result.ErrorMessage))
	{
		return Result;
	}

	// Find the dialogues sheet (sheet1.xml)
	TArray<uint8>* Sheet1Data = ZipContents.Find(TEXT("xl/worksheets/sheet1.xml"));
	if (!Sheet1Data)
	{
		Result.ErrorMessage = TEXT("Could not find dialogues sheet (xl/worksheets/sheet1.xml)");
		return Result;
	}

	// Parse dialogues
	if (!ParseDialoguesSheet(*Sheet1Data, Result.Rows, Result.ErrorMessage))
	{
		return Result;
	}

	// Parse metadata if available
	TArray<uint8>* Sheet3Data = ZipContents.Find(TEXT("xl/worksheets/sheet3.xml"));
	if (Sheet3Data)
	{
		ParseMetaSheet(*Sheet3Data, Result);
	}

	Result.bSuccess = true;
#else
	Result.ErrorMessage = TEXT("XLSX import requires WITH_EDITOR");
#endif

	return Result;
}

#if WITH_EDITOR
bool FDialogueXLSXReader::ExtractZipContents(const FString& FilePath, TMap<FString, TArray<uint8>>& OutFiles, FString& OutError)
{
	IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FilePath);
	if (!FileHandle)
	{
		OutError = FString::Printf(TEXT("Could not open file: %s"), *FilePath);
		return false;
	}

	FZipArchiveReader ZipReader(FileHandle);
	if (!ZipReader.IsValid())
	{
		OutError = TEXT("Invalid or corrupt XLSX file");
		return false;
	}

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

bool FDialogueXLSXReader::ParseDialoguesSheet(const TArray<uint8>& XmlData, TArray<FDialogueTableRow>& OutRows, FString& OutError)
{
	// Parse to grid
	TArray<TArray<FString>> Grid = ParseSheetToGrid(XmlData);

	if (Grid.Num() < 3)
	{
		OutError = TEXT("Sheet has fewer than 3 rows (need sentinel + headers + data)");
		return false;
	}

	// Row 1: Sentinel + Column IDs
	const TArray<FString>& SentinelRow = Grid[0];
	if (SentinelRow.Num() == 0 || SentinelRow[0] != EXPECTED_SENTINEL)
	{
		OutError = FString::Printf(TEXT("Invalid sentinel. Expected '%s', got '%s'"),
			*EXPECTED_SENTINEL, SentinelRow.Num() > 0 ? *SentinelRow[0] : TEXT("(empty)"));
		return false;
	}

	// Build column map from sentinel row (Column ID -> Index)
	TMap<FString, int32> ColumnMap;
	for (int32 i = 0; i < SentinelRow.Num(); i++)
	{
		if (!SentinelRow[i].IsEmpty())
		{
			ColumnMap.Add(SentinelRow[i], i);
		}
	}

	// Row 2: Human headers (skip)
	// Row 3+: Data rows
	for (int32 RowIdx = 2; RowIdx < Grid.Num(); RowIdx++)
	{
		const TArray<FString>& RowValues = Grid[RowIdx];

		// Skip empty rows
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

		// Parse row
		FDialogueTableRow DialogueRow = ParseRowFromValues(ColumnMap, RowValues);

		// Only add rows with valid DialogueID and NodeID
		if (!DialogueRow.DialogueID.IsNone() && !DialogueRow.NodeID.IsNone())
		{
			OutRows.Add(DialogueRow);
		}
	}

	return true;
}

bool FDialogueXLSXReader::ParseMetaSheet(const TArray<uint8>& XmlData, FDialogueXLSXImportResult& OutResult)
{
	TArray<TArray<FString>> Grid = ParseSheetToGrid(XmlData);

	// Parse key-value pairs (Property in col A, Value in col B)
	for (int32 i = 1; i < Grid.Num(); i++)  // Skip header row
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

TArray<TArray<FString>> FDialogueXLSXReader::ParseSheetToGrid(const TArray<uint8>& XmlData)
{
	TArray<TArray<FString>> Grid;

	// Convert to string
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

	// Find sheetData node
	const FXmlNode* SheetDataNode = RootNode->FindChildNode(TEXT("sheetData"));
	if (!SheetDataNode)
	{
		return Grid;
	}

	// Find max row number
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

	// Initialize grid
	Grid.SetNum(MaxRow);
	for (int32 i = 0; i < MaxRow; i++)
	{
		Grid[i].SetNum(MaxCol);
	}

	// Fill grid with values
	for (const FXmlNode* RowNode : SheetDataNode->GetChildrenNodes())
	{
		if (RowNode->GetTag() != TEXT("row")) continue;

		int32 RowNum = FCString::Atoi(*RowNode->GetAttribute(TEXT("r")));
		int32 RowIdx = RowNum - 1;  // 0-indexed

		for (const FXmlNode* CellNode : RowNode->GetChildrenNodes())
		{
			if (CellNode->GetTag() != TEXT("c")) continue;

			FString CellRef = CellNode->GetAttribute(TEXT("r"));
			int32 ColIdx, RowFromRef;
			ParseCellReference(CellRef, ColIdx, RowFromRef);

			// Get cell value
			FString CellValue;
			FString CellType = CellNode->GetAttribute(TEXT("t"));

			// Find value node
			const FXmlNode* ValueNode = CellNode->FindChildNode(TEXT("v"));
			const FXmlNode* InlineStrNode = CellNode->FindChildNode(TEXT("is"));

			if (InlineStrNode)
			{
				// Inline string <is><t>value</t></is>
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

			if (RowIdx >= 0 && RowIdx < Grid.Num() && ColIdx >= 0 && ColIdx < Grid[RowIdx].Num())
			{
				Grid[RowIdx][ColIdx] = CellValue;
			}
		}
	}

	return Grid;
}

int32 FDialogueXLSXReader::ColumnLetterToIndex(const FString& Letter)
{
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
	return Index - 1;  // 0-indexed
}

void FDialogueXLSXReader::ParseCellReference(const FString& CellRef, int32& OutCol, int32& OutRow)
{
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

FDialogueTableRow FDialogueXLSXReader::ParseRowFromValues(const TMap<FString, int32>& ColumnMap, const TArray<FString>& Values)
{
	FDialogueTableRow Row;

	auto GetValue = [&ColumnMap, &Values](const FString& ColumnId) -> FString
	{
		const int32* IndexPtr = ColumnMap.Find(ColumnId);
		if (IndexPtr && *IndexPtr >= 0 && *IndexPtr < Values.Num())
		{
			return Values[*IndexPtr];
		}
		return FString();
	};

	// Parse RowId (GUID)
	FString RowIdStr = GetValue(TEXT("#ROW_GUID"));
	if (!RowIdStr.IsEmpty())
	{
		FGuid::Parse(RowIdStr, Row.RowId);
	}
	if (!Row.RowId.IsValid())
	{
		Row.RowId = FGuid::NewGuid();  // Generate new if missing/invalid
	}

	// Parse FName fields
	Row.DialogueID = FName(*GetValue(TEXT("DIALOGUE_ID")));
	Row.NodeID = FName(*GetValue(TEXT("NODE_ID")));
	Row.Speaker = FName(*GetValue(TEXT("SPEAKER")));
	Row.ParentNodeID = FName(*GetValue(TEXT("PARENT_NODE_ID")));

	// Parse NodeType
	FString NodeTypeStr = GetValue(TEXT("NODE_TYPE"));
	Row.NodeType = NodeTypeStr.Equals(TEXT("Player"), ESearchCase::IgnoreCase)
		? EDialogueTableNodeType::Player
		: EDialogueTableNodeType::NPC;

	// Parse string fields
	Row.Text = GetValue(TEXT("TEXT"));
	Row.OptionText = GetValue(TEXT("OPTION_TEXT"));
	Row.Notes = GetValue(TEXT("NOTES"));

	// Parse Skippable
	FString SkippableStr = GetValue(TEXT("SKIPPABLE"));
	Row.bSkippable = !SkippableStr.Equals(TEXT("No"), ESearchCase::IgnoreCase);

	// Parse NextNodeIDs (comma-separated)
	FString NextNodesStr = GetValue(TEXT("NEXT_NODE_IDS"));
	if (!NextNodesStr.IsEmpty())
	{
		TArray<FString> NextNodeParts;
		NextNodesStr.ParseIntoArray(NextNodeParts, TEXT(","), true);
		for (const FString& Part : NextNodeParts)
		{
			FString Trimmed = Part.TrimStartAndEnd();
			if (!Trimmed.IsEmpty())
			{
				Row.NextNodeIDs.Add(FName(*Trimmed));
			}
		}
	}

	return Row;
}
