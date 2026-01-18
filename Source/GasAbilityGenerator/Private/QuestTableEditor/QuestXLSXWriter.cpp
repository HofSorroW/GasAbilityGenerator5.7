// GasAbilityGenerator - Quest XLSX Writer Implementation
// v4.12: Export Quest table to Excel format

#include "QuestTableEditor/QuestXLSXWriter.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "Compression/OodleDataCompressionUtil.h"

// Use miniz for ZIP creation
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "Compression/lz4.h"
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

const FString FQuestXLSXWriter::SHEET_SENTINEL = TEXT("#QUEST_SHEET_V1");
const FString FQuestXLSXWriter::FORMAT_VERSION = TEXT("1.0");

TArray<FQuestXLSXColumn> FQuestXLSXWriter::GetColumnDefinitions()
{
	TArray<FQuestXLSXColumn> Columns;

	// Sync columns (hidden in normal use)
	Columns.Add(FQuestXLSXColumn(TEXT("#ROW_GUID"), TEXT("Row GUID"), 36.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("#STATE"), TEXT("State"), 10.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("#BASE_HASH"), TEXT("Base Hash"), 20.0f));

	// Data columns (12 total)
	Columns.Add(FQuestXLSXColumn(TEXT("QUEST_NAME"), TEXT("Quest Name"), 20.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("DISPLAY_NAME"), TEXT("Display Name"), 25.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("STATE_ID"), TEXT("State ID"), 15.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("STATE_TYPE"), TEXT("State Type"), 12.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("DESCRIPTION"), TEXT("Description"), 40.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("PARENT_BRANCH"), TEXT("Parent Branch"), 15.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("TASKS"), TEXT("Tasks"), 50.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("EVENTS"), TEXT("Events"), 40.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("CONDITIONS"), TEXT("Conditions"), 40.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("REWARDS"), TEXT("Rewards"), 30.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("NOTES"), TEXT("Notes"), 30.0f));
	Columns.Add(FQuestXLSXColumn(TEXT("DELETED"), TEXT("Deleted"), 8.0f));

	return Columns;
}

bool FQuestXLSXWriter::ExportToXLSX(const TArray<FQuestTableRow>& Rows, const FString& FilePath, FString& OutError)
{
	// Create temp directory for XLSX parts
	FString TempDir = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("QuestXLSX_"));
	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*TempDir, true);
	FM.MakeDirectory(*(TempDir / TEXT("_rels")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl/_rels")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl/worksheets")), true);

	// Write XML parts
	FFileHelper::SaveStringToFile(GenerateContentTypesXml(), *(TempDir / TEXT("[Content_Types].xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateRelsXml(), *(TempDir / TEXT("_rels/.rels")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateWorkbookXml(), *(TempDir / TEXT("xl/workbook.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateWorkbookRelsXml(), *(TempDir / TEXT("xl/_rels/workbook.xml.rels")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateStylesXml(), *(TempDir / TEXT("xl/styles.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateQuestsSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet1.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateListsSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet2.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(GenerateMetaSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet3.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	// Create ZIP file using system command (portable approach)
	FString ZipCommand;
#if PLATFORM_WINDOWS
	// Use PowerShell Compress-Archive
	ZipCommand = FString::Printf(TEXT("powershell -ExecutionPolicy Bypass -Command \"Compress-Archive -Path '%s/*' -DestinationPath '%s' -Force\""),
		*TempDir, *FilePath);
#else
	// Use zip command on Unix
	ZipCommand = FString::Printf(TEXT("cd '%s' && zip -r '%s' ."), *TempDir, *FilePath);
#endif

	int32 ReturnCode = 0;
	FString StdOut, StdErr;
	FPlatformProcess::ExecProcess(*ZipCommand, TEXT(""), &ReturnCode, &StdOut, &StdErr);

	// Clean up temp directory
	FM.DeleteDirectory(*TempDir, false, true);

	if (ReturnCode != 0)
	{
		OutError = FString::Printf(TEXT("Failed to create XLSX: %s"), *StdErr);
		return false;
	}

	return true;
}

FString FQuestXLSXWriter::GenerateContentTypesXml()
{
	return TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		TEXT("<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n")
		TEXT("  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n")
		TEXT("  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n")
		TEXT("  <Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>\n")
		TEXT("  <Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n")
		TEXT("  <Override PartName=\"/xl/worksheets/sheet2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n")
		TEXT("  <Override PartName=\"/xl/worksheets/sheet3.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n")
		TEXT("  <Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>\n")
		TEXT("</Types>");
}

FString FQuestXLSXWriter::GenerateRelsXml()
{
	return TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		TEXT("<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n")
		TEXT("  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>\n")
		TEXT("</Relationships>");
}

FString FQuestXLSXWriter::GenerateWorkbookXml()
{
	return TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		TEXT("<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n")
		TEXT("  <sheets>\n")
		TEXT("    <sheet name=\"Quests\" sheetId=\"1\" r:id=\"rId1\"/>\n")
		TEXT("    <sheet name=\"_Lists\" sheetId=\"2\" r:id=\"rId2\"/>\n")
		TEXT("    <sheet name=\"_Meta\" sheetId=\"3\" r:id=\"rId3\"/>\n")
		TEXT("  </sheets>\n")
		TEXT("</workbook>");
}

FString FQuestXLSXWriter::GenerateWorkbookRelsXml()
{
	return TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		TEXT("<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n")
		TEXT("  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>\n")
		TEXT("  <Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet2.xml\"/>\n")
		TEXT("  <Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet3.xml\"/>\n")
		TEXT("  <Relationship Id=\"rId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>\n")
		TEXT("</Relationships>");
}

FString FQuestXLSXWriter::GenerateStylesXml()
{
	return TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		TEXT("<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n")
		TEXT("  <fonts count=\"2\">\n")
		TEXT("    <font><sz val=\"11\"/><name val=\"Calibri\"/></font>\n")
		TEXT("    <font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font>\n")
		TEXT("  </fonts>\n")
		TEXT("  <fills count=\"2\">\n")
		TEXT("    <fill><patternFill patternType=\"none\"/></fill>\n")
		TEXT("    <fill><patternFill patternType=\"gray125\"/></fill>\n")
		TEXT("  </fills>\n")
		TEXT("  <borders count=\"1\"><border/></borders>\n")
		TEXT("  <cellStyleXfs count=\"1\"><xf/></cellStyleXfs>\n")
		TEXT("  <cellXfs count=\"2\">\n")
		TEXT("    <xf xfId=\"0\"/>\n")
		TEXT("    <xf xfId=\"0\" fontId=\"1\" applyFont=\"1\"/>\n")
		TEXT("  </cellXfs>\n")
		TEXT("</styleSheet>");
}

FString FQuestXLSXWriter::GenerateQuestsSheet(const TArray<FQuestTableRow>& Rows)
{
	TArray<FQuestXLSXColumn> Columns = GetColumnDefinitions();

	FString SheetData;
	SheetData += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetData += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetData += TEXT("  <sheetData>\n");

	// Row 1: Sentinel + Column IDs
	SheetData += TEXT("    <row r=\"1\">\n");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		FString CellRef = CellReference(i, 1);
		FString Value = (i == 0) ? SHEET_SENTINEL : Columns[i].ColumnId;
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellRef, *EscapeXml(Value));
	}
	SheetData += TEXT("    </row>\n");

	// Row 2: Human-readable headers (bold)
	SheetData += TEXT("    <row r=\"2\">\n");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		FString CellRef = CellReference(i, 2);
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\" s=\"1\"><is><t>%s</t></is></c>\n"),
			*CellRef, *EscapeXml(Columns[i].DisplayName));
	}
	SheetData += TEXT("    </row>\n");

	// Data rows
	int32 RowNum = 3;
	for (const FQuestTableRow& Row : Rows)
	{
		SheetData += FString::Printf(TEXT("    <row r=\"%d\">\n"), RowNum);

		int32 ColIdx = 0;

		// Sync columns
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.RowId.ToString()));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), Row.bDeleted ? TEXT("DELETED") : TEXT("ACTIVE"));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%lld</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), ComputeRowHash(Row));

		// Data columns
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.QuestName));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.DisplayName));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.StateID));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.GetStateTypeString()));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Description));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.ParentBranch));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Tasks));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Events));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Conditions));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Rewards));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), *EscapeXml(Row.Notes));
		SheetData += FString::Printf(TEXT("      <c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx++, RowNum), Row.bDeleted ? TEXT("TRUE") : TEXT("FALSE"));

		SheetData += TEXT("    </row>\n");
		RowNum++;
	}

	SheetData += TEXT("  </sheetData>\n");
	SheetData += TEXT("</worksheet>");

	return SheetData;
}

FString FQuestXLSXWriter::GenerateListsSheet(const TArray<FQuestTableRow>& Rows)
{
	// Collect unique values for dropdowns
	TSet<FString> QuestNames;
	TSet<FString> StateTypes;
	StateTypes.Add(TEXT("Regular"));
	StateTypes.Add(TEXT("Success"));
	StateTypes.Add(TEXT("Failure"));

	for (const FQuestTableRow& Row : Rows)
	{
		if (!Row.QuestName.IsEmpty()) QuestNames.Add(Row.QuestName);
	}

	FString SheetData;
	SheetData += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetData += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetData += TEXT("  <sheetData>\n");

	// Headers
	SheetData += TEXT("    <row r=\"1\">\n");
	SheetData += TEXT("      <c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>Quest Names</t></is></c>\n");
	SheetData += TEXT("      <c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>State Types</t></is></c>\n");
	SheetData += TEXT("    </row>\n");

	// Data
	int32 MaxRows = FMath::Max(QuestNames.Num(), StateTypes.Num());
	TArray<FString> QuestArray = QuestNames.Array();
	TArray<FString> StateArray = StateTypes.Array();

	for (int32 i = 0; i < MaxRows; i++)
	{
		SheetData += FString::Printf(TEXT("    <row r=\"%d\">\n"), i + 2);
		if (i < QuestArray.Num())
		{
			SheetData += FString::Printf(TEXT("      <c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
				i + 2, *EscapeXml(QuestArray[i]));
		}
		if (i < StateArray.Num())
		{
			SheetData += FString::Printf(TEXT("      <c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
				i + 2, *EscapeXml(StateArray[i]));
		}
		SheetData += TEXT("    </row>\n");
	}

	SheetData += TEXT("  </sheetData>\n");
	SheetData += TEXT("</worksheet>");

	return SheetData;
}

FString FQuestXLSXWriter::GenerateMetaSheet(const TArray<FQuestTableRow>& Rows)
{
	FString ExportGuid = FGuid::NewGuid().ToString();
	FString Timestamp = FDateTime::Now().ToString();

	// Compute content hash
	uint32 ContentHash = 0;
	for (const FQuestTableRow& Row : Rows)
	{
		ContentHash = HashCombine(ContentHash, Row.ComputeEditableFieldsHash());
	}

	FString SheetData;
	SheetData += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetData += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetData += TEXT("  <sheetData>\n");

	SheetData += TEXT("    <row r=\"1\"><c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>Metadata Key</t></is></c><c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>Value</t></is></c></row>\n");
	SheetData += FString::Printf(TEXT("    <row r=\"2\"><c r=\"A2\" t=\"inlineStr\"><is><t>ExportGuid</t></is></c><c r=\"B2\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"), *ExportGuid);
	SheetData += FString::Printf(TEXT("    <row r=\"3\"><c r=\"A3\" t=\"inlineStr\"><is><t>ExportTimestamp</t></is></c><c r=\"B3\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"), *Timestamp);
	SheetData += FString::Printf(TEXT("    <row r=\"4\"><c r=\"A4\" t=\"inlineStr\"><is><t>ContentHash</t></is></c><c r=\"B4\" t=\"inlineStr\"><is><t>%u</t></is></c></row>\n"), ContentHash);
	SheetData += FString::Printf(TEXT("    <row r=\"5\"><c r=\"A5\" t=\"inlineStr\"><is><t>RowCount</t></is></c><c r=\"B5\" t=\"inlineStr\"><is><t>%d</t></is></c></row>\n"), Rows.Num());
	SheetData += FString::Printf(TEXT("    <row r=\"6\"><c r=\"A6\" t=\"inlineStr\"><is><t>FormatVersion</t></is></c><c r=\"B6\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"), *FORMAT_VERSION);

	SheetData += TEXT("  </sheetData>\n");
	SheetData += TEXT("</worksheet>");

	return SheetData;
}

FString FQuestXLSXWriter::EscapeXml(const FString& Text)
{
	FString Escaped = Text;
	Escaped.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Escaped.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Escaped.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	Escaped.ReplaceInline(TEXT("\""), TEXT("&quot;"));
	Escaped.ReplaceInline(TEXT("'"), TEXT("&apos;"));
	return Escaped;
}

FString FQuestXLSXWriter::CellReference(int32 Col, int32 Row)
{
	return ColumnLetter(Col) + FString::FromInt(Row);
}

FString FQuestXLSXWriter::ColumnLetter(int32 ColIndex)
{
	FString Letters;
	int32 Remaining = ColIndex;
	do
	{
		Letters = FString::Chr(TEXT('A') + (Remaining % 26)) + Letters;
		Remaining = Remaining / 26 - 1;
	} while (Remaining >= 0);
	return Letters;
}

int64 FQuestXLSXWriter::ComputeRowHash(const FQuestTableRow& Row)
{
	return static_cast<int64>(Row.ComputeEditableFieldsHash());
}
