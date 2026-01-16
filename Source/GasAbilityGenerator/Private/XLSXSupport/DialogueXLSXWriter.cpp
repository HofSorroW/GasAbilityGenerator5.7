// GasAbilityGenerator - Dialogue XLSX Writer Implementation
// v4.4 Phase 3: Export dialogue table to Excel format with asset sync support

#include "XLSXSupport/DialogueXLSXWriter.h"
#include "XLSXSupport/DialogueTokenRegistry.h"
#include "XLSXSupport/DialogueAssetSync.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "AssetRegistry/AssetRegistryModule.h"

#if WITH_ENGINE
#include "FileUtilities/ZipArchiveWriter.h"
#endif

// Static constants
const FString FDialogueXLSXWriter::SHEET_SENTINEL = TEXT("#DIALOGUE_SHEET_V1");
const FString FDialogueXLSXWriter::FORMAT_VERSION = TEXT("1.0");

TArray<FDialogueXLSXColumn> FDialogueXLSXWriter::GetColumnDefinitions()
{
	return {
		// System identity
		{ TEXT("#ROW_GUID"),           TEXT("Row ID"),         12.0f },
		{ TEXT("DIALOGUE_ID"),         TEXT("Dialogue"),       15.0f },
		{ TEXT("NODE_ID"),             TEXT("Node ID"),        15.0f },
		// Graph structure (read-only in v4.4)
		{ TEXT("[RO]NODE_TYPE"),       TEXT("Type"),           8.0f },
		{ TEXT("SPEAKER"),             TEXT("Speaker"),        12.0f },
		// Text authoring (editable)
		{ TEXT("TEXT"),                TEXT("Text"),           50.0f },
		{ TEXT("OPTION_TEXT"),         TEXT("Option Text"),    30.0f },
		// Logic authoring (v4.4 tokens)
		{ TEXT("EVENTS"),              TEXT("Events"),         40.0f },
		{ TEXT("[RO]EVENTS_CURRENT"),  TEXT("UE Events"),      40.0f },
		{ TEXT("CONDITIONS"),          TEXT("Conditions"),     40.0f },
		{ TEXT("[RO]CONDITIONS_CURRENT"), TEXT("UE Conditions"), 40.0f },
		// Graph context (read-only)
		{ TEXT("[RO]PARENT_NODE_ID"),  TEXT("Parent"),         15.0f },
		{ TEXT("[RO]NEXT_NODE_IDS"),   TEXT("Next Nodes"),     20.0f },
		// Other fields
		{ TEXT("SKIPPABLE"),           TEXT("Skip"),           6.0f },
		{ TEXT("NOTES"),               TEXT("Notes"),          30.0f },
		// Sync bookkeeping
		{ TEXT("#STATE"),              TEXT("State"),          10.0f },
		{ TEXT("#BASE_HASH"),          TEXT("Hash"),           15.0f },
	};
}

bool FDialogueXLSXWriter::ExportToXLSX(const TArray<FDialogueTableRow>& Rows, const FString& FilePath, FString& OutError)
{
#if WITH_ENGINE
	// Scan project assets to populate valid IDs for _Lists sheet
	ScanAssetsForValidIds();

	// Create the output file
	IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath);
	if (!FileHandle)
	{
		OutError = FString::Printf(TEXT("Failed to create file: %s"), *FilePath);
		return false;
	}

	// Create ZIP writer with no compression (STORE mode)
	FZipArchiveWriter ZipWriter(FileHandle, EZipArchiveOptions::None);
	FDateTime Now = FDateTime::Now();

	// Add all required XLSX files
	auto AddXmlFile = [&ZipWriter, &Now](const FString& Path, const FString& Content)
	{
		TArray<uint8> Data;
		FTCHARToUTF8 Converter(*Content);
		Data.Append((const uint8*)Converter.Get(), Converter.Length());
		ZipWriter.AddFile(Path, Data, Now);
	};

	// Core XLSX structure
	AddXmlFile(TEXT("[Content_Types].xml"), GenerateContentTypesXml());
	AddXmlFile(TEXT("_rels/.rels"), GenerateRelsXml());
	AddXmlFile(TEXT("xl/workbook.xml"), GenerateWorkbookXml());
	AddXmlFile(TEXT("xl/_rels/workbook.xml.rels"), GenerateWorkbookRelsXml());
	AddXmlFile(TEXT("xl/styles.xml"), GenerateStylesXml());

	// Worksheets
	AddXmlFile(TEXT("xl/worksheets/sheet1.xml"), GenerateDialoguesSheet(Rows));
	AddXmlFile(TEXT("xl/worksheets/sheet2.xml"), GenerateListsSheet(Rows));
	AddXmlFile(TEXT("xl/worksheets/sheet3.xml"), GenerateMetaSheet(Rows));

	// ZipWriter destructor will finalize and close the file
	return true;
#else
	OutError = TEXT("XLSX export requires WITH_ENGINE");
	return false;
#endif
}

bool FDialogueXLSXWriter::ExportToXLSX(const TArray<FDialogueTableRow>& Rows, const FString& FilePath,
	const FDialogueAssetSyncResult& AssetSync, FString& OutError)
{
#if WITH_ENGINE
	// Scan project assets to populate valid IDs for _Lists sheet
	ScanAssetsForValidIds();

	// Create the output file
	IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath);
	if (!FileHandle)
	{
		OutError = FString::Printf(TEXT("Failed to create file: %s"), *FilePath);
		return false;
	}

	// Create ZIP writer with no compression (STORE mode)
	FZipArchiveWriter ZipWriter(FileHandle, EZipArchiveOptions::None);
	FDateTime Now = FDateTime::Now();

	// Add all required XLSX files
	auto AddXmlFile = [&ZipWriter, &Now](const FString& Path, const FString& Content)
	{
		TArray<uint8> Data;
		FTCHARToUTF8 Converter(*Content);
		Data.Append((const uint8*)Converter.Get(), Converter.Length());
		ZipWriter.AddFile(Path, Data, Now);
	};

	// Core XLSX structure
	AddXmlFile(TEXT("[Content_Types].xml"), GenerateContentTypesXml());
	AddXmlFile(TEXT("_rels/.rels"), GenerateRelsXml());
	AddXmlFile(TEXT("xl/workbook.xml"), GenerateWorkbookXml());
	AddXmlFile(TEXT("xl/_rels/workbook.xml.rels"), GenerateWorkbookRelsXml());
	AddXmlFile(TEXT("xl/styles.xml"), GenerateStylesXml());

	// Worksheets - use overload with asset sync for dialogues sheet
	AddXmlFile(TEXT("xl/worksheets/sheet1.xml"), GenerateDialoguesSheet(Rows, &AssetSync));
	AddXmlFile(TEXT("xl/worksheets/sheet2.xml"), GenerateListsSheet(Rows));
	AddXmlFile(TEXT("xl/worksheets/sheet3.xml"), GenerateMetaSheet(Rows));

	// ZipWriter destructor will finalize and close the file
	return true;
#else
	OutError = TEXT("XLSX export requires WITH_ENGINE");
	return false;
#endif
}

FString FDialogueXLSXWriter::GenerateContentTypesXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
  <Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet2.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet3.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>
</Types>)");
}

FString FDialogueXLSXWriter::GenerateRelsXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)");
}

FString FDialogueXLSXWriter::GenerateWorkbookXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <sheets>
    <sheet name="Dialogues" sheetId="1" r:id="rId1"/>
    <sheet name="_Lists" sheetId="2" r:id="rId2"/>
    <sheet name="_Meta" sheetId="3" r:id="rId3"/>
  </sheets>
</workbook>)");
}

FString FDialogueXLSXWriter::GenerateWorkbookRelsXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet2.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet3.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)");
}

FString FDialogueXLSXWriter::GenerateStylesXml()
{
	// Minimal styles - header row bold
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <fonts count="2">
    <font><sz val="11"/><name val="Calibri"/></font>
    <font><b/><sz val="11"/><name val="Calibri"/></font>
  </fonts>
  <fills count="2">
    <fill><patternFill patternType="none"/></fill>
    <fill><patternFill patternType="gray125"/></fill>
  </fills>
  <borders count="1">
    <border/>
  </borders>
  <cellStyleXfs count="1">
    <xf numFmtId="0" fontId="0" fillId="0" borderId="0"/>
  </cellStyleXfs>
  <cellXfs count="2">
    <xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>
    <xf numFmtId="0" fontId="1" fillId="0" borderId="0" xfId="0"/>
  </cellXfs>
</styleSheet>)");
}

FString FDialogueXLSXWriter::GenerateDialoguesSheet(const TArray<FDialogueTableRow>& Rows)
{
	// Call overload with no asset sync data
	return GenerateDialoguesSheet(Rows, nullptr);
}

FString FDialogueXLSXWriter::GenerateDialoguesSheet(const TArray<FDialogueTableRow>& Rows, const FDialogueAssetSyncResult* AssetSync)
{
	TArray<FDialogueXLSXColumn> Columns = GetColumnDefinitions();
	FString SheetData;

	// Row 1: Sentinel with column IDs
	SheetData += TEXT("<row r=\"1\">");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		FString CellValue = (i == 0) ? SHEET_SENTINEL : Columns[i].ColumnId;
		SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
			*CellReference(i, 1), *EscapeXml(CellValue));
	}
	SheetData += TEXT("</row>");

	// Row 2: Human-readable headers (bold - style 1)
	SheetData += TEXT("<row r=\"2\">");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\" s=\"1\"><is><t>%s</t></is></c>"),
			*CellReference(i, 2), *EscapeXml(Columns[i].DisplayName));
	}
	SheetData += TEXT("</row>");

	// Data rows (starting at row 3)
	int32 RowNum = 3;
	for (const FDialogueTableRow& Row : Rows)
	{
		SheetData += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);

		// Build NextNodeIDs as comma-separated string
		FString NextNodesStr;
		for (int32 i = 0; i < Row.NextNodeIDs.Num(); i++)
		{
			if (i > 0) NextNodesStr += TEXT(",");
			NextNodesStr += Row.NextNodeIDs[i].ToString();
		}

		// v4.4 Phase 3: Token strings from row and asset sync
		// EVENTS/CONDITIONS = Editable tokens (what user authored in Excel)
		// [RO]EVENTS_CURRENT/[RO]CONDITIONS_CURRENT = UE's current state from asset sync
		FString EventsStr = Row.EventsTokenStr;           // Editable tokens
		FString EventsCurrentStr;                         // [RO] UE's current state
		FString ConditionsStr = Row.ConditionsTokenStr;   // Editable tokens
		FString ConditionsCurrentStr;                     // [RO] UE's current state

		// Lookup [RO] columns from asset sync if available
		if (AssetSync && AssetSync->bSuccess)
		{
			FString Key = FDialogueAssetSyncResult::MakeKey(Row.DialogueID, Row.NodeID);
			const FDialogueNodeAssetData* NodeData = AssetSync->NodeData.Find(Key);
			if (NodeData && NodeData->bFoundInAsset)
			{
				EventsCurrentStr = NodeData->EventsTokenStr;
				ConditionsCurrentStr = NodeData->ConditionsTokenStr;
			}
		}

		// Compute hash for this row (includes editable fields only)
		int64 RowHash = ComputeRowHash(Row);

		// Column values in order (must match GetColumnDefinitions)
		TArray<FString> Values = {
			Row.RowId.ToString(),                                           // #ROW_GUID
			Row.DialogueID.ToString(),                                      // DIALOGUE_ID
			Row.NodeID.ToString(),                                          // NODE_ID
			Row.NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player"), // [RO]NODE_TYPE
			Row.Speaker.ToString(),                                         // SPEAKER
			Row.Text,                                                       // TEXT
			Row.OptionText,                                                 // OPTION_TEXT
			EventsStr,                                                      // EVENTS (editable)
			EventsCurrentStr,                                               // [RO]EVENTS_CURRENT
			ConditionsStr,                                                  // CONDITIONS (editable)
			ConditionsCurrentStr,                                           // [RO]CONDITIONS_CURRENT
			Row.ParentNodeID.ToString(),                                    // [RO]PARENT_NODE_ID
			NextNodesStr,                                                   // [RO]NEXT_NODE_IDS
			Row.bSkippable ? TEXT("Yes") : TEXT("No"),                      // SKIPPABLE
			Row.Notes,                                                      // NOTES
			TEXT("Synced"),                                                 // #STATE
			FString::Printf(TEXT("%lld"), RowHash),                         // #BASE_HASH
		};

		for (int32 i = 0; i < Values.Num(); i++)
		{
			SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				*CellReference(i, RowNum), *EscapeXml(Values[i]));
		}

		SheetData += TEXT("</row>");
		RowNum++;
	}

	// Build column widths
	FString ColsXml = TEXT("<cols>");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		ColsXml += FString::Printf(TEXT("<col min=\"%d\" max=\"%d\" width=\"%.1f\" customWidth=\"1\"/>"),
			i + 1, i + 1, Columns[i].Width);
	}
	ColsXml += TEXT("</cols>");

	// Full worksheet XML
	return FString::Printf(TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  %s
  <sheetData>%s</sheetData>
</worksheet>)"), *ColsXml, *SheetData);
}

FString FDialogueXLSXWriter::GenerateListsSheet(const TArray<FDialogueTableRow>& Rows)
{
	// Collect unique values for dropdowns and token validation
	TSet<FString> DialogueIds;
	TSet<FString> Speakers;
	TSet<FString> NodeTypes;
	TSet<FString> QuestIds;    // v4.4: For token validation
	TSet<FString> ItemIds;     // v4.4: For token validation
	TSet<FString> EventTokens; // v4.4: Available event token types
	TSet<FString> ConditionTokens; // v4.4: Available condition token types

	NodeTypes.Add(TEXT("NPC"));
	NodeTypes.Add(TEXT("Player"));

	// Populate available tokens from registry
	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	for (const FDialogueTokenSpec& Spec : Registry.GetAllSpecs())
	{
		if (Spec.Category == ETokenCategory::Event)
		{
			EventTokens.Add(Spec.TokenName);
		}
		else
		{
			ConditionTokens.Add(Spec.TokenName);
		}
	}

	for (const FDialogueTableRow& Row : Rows)
	{
		if (!Row.DialogueID.IsNone())
		{
			DialogueIds.Add(Row.DialogueID.ToString());
		}
		if (!Row.Speaker.IsNone())
		{
			Speakers.Add(Row.Speaker.ToString());
		}
	}

	// Note: QuestIds and ItemIds would be populated from asset scan in "Sync from Assets"
	// For now, they're populated from registry's valid ID sets if available
	for (const FString& QuestId : Registry.GetValidIds(TEXT("QuestId")))
	{
		QuestIds.Add(QuestId);
	}
	for (const FString& ItemId : Registry.GetValidIds(TEXT("ItemId")))
	{
		ItemIds.Add(ItemId);
	}

	FString SheetData;

	// Header row
	SheetData += TEXT("<row r=\"1\">");
	SheetData += TEXT("<c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>DIALOGUE_ID</t></is></c>");
	SheetData += TEXT("<c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>SPEAKER_ID</t></is></c>");
	SheetData += TEXT("<c r=\"C1\" t=\"inlineStr\" s=\"1\"><is><t>NODE_TYPE</t></is></c>");
	SheetData += TEXT("<c r=\"D1\" t=\"inlineStr\" s=\"1\"><is><t>QUEST_ID</t></is></c>");
	SheetData += TEXT("<c r=\"E1\" t=\"inlineStr\" s=\"1\"><is><t>ITEM_ID</t></is></c>");
	SheetData += TEXT("<c r=\"F1\" t=\"inlineStr\" s=\"1\"><is><t>EVENT_TOKENS</t></is></c>");
	SheetData += TEXT("<c r=\"G1\" t=\"inlineStr\" s=\"1\"><is><t>CONDITION_TOKENS</t></is></c>");
	SheetData += TEXT("</row>");

	// Data rows - fill each column with unique values
	TArray<FString> DialogueIdArr = DialogueIds.Array();
	TArray<FString> SpeakerArr = Speakers.Array();
	TArray<FString> NodeTypeArr = NodeTypes.Array();
	TArray<FString> QuestIdArr = QuestIds.Array();
	TArray<FString> ItemIdArr = ItemIds.Array();
	TArray<FString> EventTokenArr = EventTokens.Array();
	TArray<FString> ConditionTokenArr = ConditionTokens.Array();

	int32 MaxRows = FMath::Max(DialogueIdArr.Num(), SpeakerArr.Num());
	MaxRows = FMath::Max(MaxRows, NodeTypeArr.Num());
	MaxRows = FMath::Max(MaxRows, QuestIdArr.Num());
	MaxRows = FMath::Max(MaxRows, ItemIdArr.Num());
	MaxRows = FMath::Max(MaxRows, EventTokenArr.Num());
	MaxRows = FMath::Max(MaxRows, ConditionTokenArr.Num());

	for (int32 i = 0; i < MaxRows; i++)
	{
		int32 RowNum = i + 2;
		SheetData += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);

		if (i < DialogueIdArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(DialogueIdArr[i]));
		}
		if (i < SpeakerArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(SpeakerArr[i]));
		}
		if (i < NodeTypeArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"C%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(NodeTypeArr[i]));
		}
		if (i < QuestIdArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"D%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(QuestIdArr[i]));
		}
		if (i < ItemIdArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"E%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(ItemIdArr[i]));
		}
		if (i < EventTokenArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"F%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(EventTokenArr[i]));
		}
		if (i < ConditionTokenArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"G%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(ConditionTokenArr[i]));
		}

		SheetData += TEXT("</row>");
	}

	return FString::Printf(TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <cols>
    <col min="1" max="1" width="20" customWidth="1"/>
    <col min="2" max="2" width="20" customWidth="1"/>
    <col min="3" max="3" width="15" customWidth="1"/>
    <col min="4" max="4" width="25" customWidth="1"/>
    <col min="5" max="5" width="25" customWidth="1"/>
    <col min="6" max="6" width="25" customWidth="1"/>
    <col min="7" max="7" width="25" customWidth="1"/>
  </cols>
  <sheetData>%s</sheetData>
</worksheet>)"), *SheetData);
}

FString FDialogueXLSXWriter::GenerateMetaSheet(const TArray<FDialogueTableRow>& Rows)
{
	FString ExportGuid = FGuid::NewGuid().ToString();
	FString ExportTime = FDateTime::Now().ToString();

	// Compute overall content hash
	int64 ContentHash = 0;
	for (const FDialogueTableRow& Row : Rows)
	{
		ContentHash ^= ComputeRowHash(Row);
	}

	FString SheetData;

	// Headers
	SheetData += TEXT("<row r=\"1\">");
	SheetData += TEXT("<c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>Property</t></is></c>");
	SheetData += TEXT("<c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>Value</t></is></c>");
	SheetData += TEXT("</row>");

	// Metadata values
	TArray<TPair<FString, FString>> MetaValues = {
		{ TEXT("EXPORT_GUID"), ExportGuid },
		{ TEXT("EXPORTED_AT"), ExportTime },
		{ TEXT("FORMAT_VERSION"), FORMAT_VERSION },
		{ TEXT("ROW_COUNT"), FString::Printf(TEXT("%d"), Rows.Num()) },
		{ TEXT("CONTENT_HASH"), FString::Printf(TEXT("%lld"), ContentHash) },
		{ TEXT("TABLE_TYPE"), TEXT("Dialogue") },
	};

	for (int32 i = 0; i < MetaValues.Num(); i++)
	{
		int32 RowNum = i + 2;
		SheetData += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);
		SheetData += FString::Printf(TEXT("<c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
			RowNum, *EscapeXml(MetaValues[i].Key));
		SheetData += FString::Printf(TEXT("<c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
			RowNum, *EscapeXml(MetaValues[i].Value));
		SheetData += TEXT("</row>");
	}

	return FString::Printf(TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <cols>
    <col min="1" max="1" width="20" customWidth="1"/>
    <col min="2" max="2" width="40" customWidth="1"/>
  </cols>
  <sheetData>%s</sheetData>
</worksheet>)"), *SheetData);
}

FString FDialogueXLSXWriter::EscapeXml(const FString& Text)
{
	FString Result = Text;
	Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	Result.ReplaceInline(TEXT("\""), TEXT("&quot;"));
	Result.ReplaceInline(TEXT("'"), TEXT("&apos;"));
	return Result;
}

FString FDialogueXLSXWriter::CellReference(int32 Col, int32 Row)
{
	return ColumnLetter(Col) + FString::Printf(TEXT("%d"), Row);
}

FString FDialogueXLSXWriter::ColumnLetter(int32 ColIndex)
{
	FString Result;
	int32 Col = ColIndex;

	do
	{
		Result = FString::Chr(TEXT('A') + (Col % 26)) + Result;
		Col = Col / 26 - 1;
	}
	while (Col >= 0);

	return Result;
}

int64 FDialogueXLSXWriter::ComputeRowHash(const FDialogueTableRow& Row)
{
	// Hash key fields that affect sync
	int64 Hash = 0;
	Hash ^= GetTypeHash(Row.DialogueID);
	Hash ^= GetTypeHash(Row.NodeID);
	Hash ^= GetTypeHash((uint8)Row.NodeType);
	Hash ^= GetTypeHash(Row.Speaker);
	Hash ^= GetTypeHash(Row.Text);
	Hash ^= GetTypeHash(Row.OptionText);
	Hash ^= GetTypeHash(Row.ParentNodeID);
	Hash ^= GetTypeHash(Row.bSkippable);
	Hash ^= GetTypeHash(Row.Notes);

	// v4.4: Include token strings in hash
	Hash ^= GetTypeHash(Row.EventsTokenStr);
	Hash ^= GetTypeHash(Row.ConditionsTokenStr);

	for (const FName& NextId : Row.NextNodeIDs)
	{
		Hash ^= GetTypeHash(NextId);
	}

	return Hash;
}

void FDialogueXLSXWriter::ScanAssetsForValidIds()
{
	FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	Registry.ClearValidIds();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TSet<FString> QuestIds;
	TSet<FString> ItemIds;
	TSet<FString> NPCIds;

	// Scan for Quest assets (UQuestBlueprint)
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/NarrativeArsenal"), TEXT("QuestBlueprint")));
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssets(Filter, AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			FString AssetName = Asset.AssetName.ToString();
			// Remove Quest_ prefix if present for cleaner IDs
			if (AssetName.StartsWith(TEXT("Quest_")))
			{
				AssetName = AssetName.RightChop(6);
			}
			QuestIds.Add(AssetName);
		}
		UE_LOG(LogTemp, Log, TEXT("ScanAssetsForValidIds: Found %d Quest assets"), QuestIds.Num());
	}

	// Scan for Item assets (EquippableItem, NarrativeItem)
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/NarrativeArsenal"), TEXT("EquippableItem")));
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/NarrativeArsenal"), TEXT("NarrativeItem")));
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssets(Filter, AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			FString AssetName = Asset.AssetName.ToString();
			ItemIds.Add(AssetName);
		}
		UE_LOG(LogTemp, Log, TEXT("ScanAssetsForValidIds: Found %d Item assets"), ItemIds.Num());
	}

	// Scan for NPC Definition assets
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/NarrativeArsenal"), TEXT("NPCDefinition")));
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssets(Filter, AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			FString AssetName = Asset.AssetName.ToString();
			// Remove NPC_ prefix if present for cleaner IDs
			if (AssetName.StartsWith(TEXT("NPC_")))
			{
				AssetName = AssetName.RightChop(4);
			}
			NPCIds.Add(AssetName);
		}
		UE_LOG(LogTemp, Log, TEXT("ScanAssetsForValidIds: Found %d NPC Definition assets"), NPCIds.Num());
	}

	// Register with token registry
	Registry.SetValidIds(TEXT("QuestId"), QuestIds);
	Registry.SetValidIds(TEXT("ItemId"), ItemIds);
	Registry.SetValidIds(TEXT("NPCId"), NPCIds);

	UE_LOG(LogTemp, Log, TEXT("ScanAssetsForValidIds: Registered %d quest IDs, %d item IDs, %d NPC IDs"),
		QuestIds.Num(), ItemIds.Num(), NPCIds.Num());
}
