// GasAbilityGenerator - NPC XLSX Writer Implementation
// v4.4: Export NPC table to Excel format
//
// Generates XLSX (ZIP container with XML files) using UE's FZipArchiveWriter.
// Uses STORE mode (no compression) for maximum Excel compatibility.

#include "XLSXSupport/NPCXLSXWriter.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

#if WITH_ENGINE
#include "FileUtilities/ZipArchiveWriter.h"
#endif

//=============================================================================
// Static Constants
//=============================================================================

const FString FNPCXLSXWriter::SHEET_SENTINEL = TEXT("#NPC_SHEET_V1");
const FString FNPCXLSXWriter::FORMAT_VERSION = TEXT("1.0");

//=============================================================================
// Column Definitions
//=============================================================================

TArray<FNPCXLSXColumn> FNPCXLSXWriter::GetColumnDefinitions()
{
	// Column order matches NPC Table Editor display order
	// Width values are approximate Excel character widths
	return {
		// Sync/Internal columns (prefixed with #)
		{ TEXT("#ROW_GUID"),       TEXT("Row ID"),          12.0f },

		// Core Identity (5 columns)
		{ TEXT("NPC_NAME"),        TEXT("NPC Name"),        15.0f },
		{ TEXT("NPC_ID"),          TEXT("NPC ID"),          12.0f },
		{ TEXT("DISPLAY_NAME"),    TEXT("Display Name"),    18.0f },
		{ TEXT("BLUEPRINT"),       TEXT("Blueprint"),       20.0f },

		// AI & Behavior (4 columns)
		{ TEXT("ABILITY_CONFIG"),  TEXT("Ability Config"),  18.0f },
		{ TEXT("ACTIVITY_CONFIG"), TEXT("Activity Config"), 18.0f },
		{ TEXT("SCHEDULE"),        TEXT("Schedule"),        15.0f },
		{ TEXT("BEHAVIOR_TREE"),   TEXT("Behavior Tree"),   18.0f },

		// Combat (4 columns)
		{ TEXT("MIN_LEVEL"),       TEXT("Min Lvl"),         8.0f },
		{ TEXT("MAX_LEVEL"),       TEXT("Max Lvl"),         8.0f },
		{ TEXT("FACTIONS"),        TEXT("Factions"),        25.0f },
		{ TEXT("ATTACK_PRIORITY"), TEXT("Attack Prio"),     10.0f },

		// Vendor (2 columns)
		{ TEXT("IS_VENDOR"),       TEXT("Vendor?"),         8.0f },
		{ TEXT("SHOP_NAME"),       TEXT("Shop Name"),       15.0f },

		// Items & Spawning (2 columns)
		{ TEXT("DEFAULT_ITEMS"),   TEXT("Default Items"),   25.0f },
		{ TEXT("SPAWNER_POI"),     TEXT("Spawner POI"),     20.0f },

		// Meta (2 columns)
		{ TEXT("APPEARANCE"),      TEXT("Appearance"),      18.0f },
		{ TEXT("NOTES"),           TEXT("Notes"),           30.0f },

		// Sync tracking columns (prefixed with #)
		{ TEXT("#STATE"),          TEXT("State"),           10.0f },
		{ TEXT("#BASE_HASH"),      TEXT("Hash"),            15.0f },
	};
}

//=============================================================================
// Main Export Function
//=============================================================================

bool FNPCXLSXWriter::ExportToXLSX(const TArray<FNPCTableRow>& Rows, const FString& FilePath, FString& OutError)
{
#if WITH_ENGINE
	// Create output file handle
	IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath);
	if (!FileHandle)
	{
		// v4.8.4: Improved error message - file lock is most common cause
		OutError = FString::Printf(TEXT("Cannot write to file: %s\n\nIf the file is open in Excel, please close it and try again."), *FilePath);
		return false;
	}

	// Create ZIP writer with STORE mode (no compression) for Excel compatibility
	// EZipArchiveOptions::None = STORE method
	FZipArchiveWriter ZipWriter(FileHandle, EZipArchiveOptions::None);
	FDateTime Now = FDateTime::Now();

	// Helper lambda to add XML content as UTF-8 encoded file
	auto AddXmlFile = [&ZipWriter, &Now](const FString& Path, const FString& Content)
	{
		TArray<uint8> Data;
		FTCHARToUTF8 Converter(*Content);
		Data.Append((const uint8*)Converter.Get(), Converter.Length());
		ZipWriter.AddFile(Path, Data, Now);
	};

	// Add core XLSX structure files
	AddXmlFile(TEXT("[Content_Types].xml"), GenerateContentTypesXml());
	AddXmlFile(TEXT("_rels/.rels"), GenerateRelsXml());
	AddXmlFile(TEXT("xl/workbook.xml"), GenerateWorkbookXml());
	AddXmlFile(TEXT("xl/_rels/workbook.xml.rels"), GenerateWorkbookRelsXml());
	AddXmlFile(TEXT("xl/styles.xml"), GenerateStylesXml());

	// Add worksheets
	AddXmlFile(TEXT("xl/worksheets/sheet1.xml"), GenerateNPCsSheet(Rows));
	AddXmlFile(TEXT("xl/worksheets/sheet2.xml"), GenerateListsSheet(Rows));
	AddXmlFile(TEXT("xl/worksheets/sheet3.xml"), GenerateMetaSheet(Rows));

	// ZipWriter destructor finalizes and closes the file
	return true;
#else
	OutError = TEXT("XLSX export requires WITH_ENGINE (Editor builds only)");
	return false;
#endif
}

//=============================================================================
// XLSX Structure XML Generators
//=============================================================================

FString FNPCXLSXWriter::GenerateContentTypesXml()
{
	// Declares MIME types for all parts in the XLSX package
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

FString FNPCXLSXWriter::GenerateRelsXml()
{
	// Root relationships - points to the workbook
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)");
}

FString FNPCXLSXWriter::GenerateWorkbookXml()
{
	// Workbook with three sheets: NPCs (data), _Lists (dropdowns), _Meta (sync info)
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <sheets>
    <sheet name="NPCs" sheetId="1" r:id="rId1"/>
    <sheet name="_Lists" sheetId="2" r:id="rId2"/>
    <sheet name="_Meta" sheetId="3" r:id="rId3"/>
  </sheets>
</workbook>)");
}

FString FNPCXLSXWriter::GenerateWorkbookRelsXml()
{
	// Relationships from workbook to sheets and styles
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet2.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet3.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)");
}

FString FNPCXLSXWriter::GenerateStylesXml()
{
	// Minimal styles - just regular and bold fonts
	// Style 0 = normal, Style 1 = bold (for headers)
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

//=============================================================================
// Worksheet Generators
//=============================================================================

FString FNPCXLSXWriter::GenerateNPCsSheet(const TArray<FNPCTableRow>& Rows)
{
	TArray<FNPCXLSXColumn> Columns = GetColumnDefinitions();
	FString SheetData;

	//-------------------------------------------------------------------------
	// Row 1: Sentinel with column IDs (machine-readable)
	// First cell contains sentinel value, rest contain column IDs
	//-------------------------------------------------------------------------
	SheetData += TEXT("<row r=\"1\">");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		FString CellValue = (i == 0) ? SHEET_SENTINEL : Columns[i].ColumnId;
		SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
			*CellReference(i, 1), *EscapeXml(CellValue));
	}
	SheetData += TEXT("</row>");

	//-------------------------------------------------------------------------
	// Row 2: Human-readable headers (bold style - s="1")
	//-------------------------------------------------------------------------
	SheetData += TEXT("<row r=\"2\">");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\" s=\"1\"><is><t>%s</t></is></c>"),
			*CellReference(i, 2), *EscapeXml(Columns[i].DisplayName));
	}
	SheetData += TEXT("</row>");

	//-------------------------------------------------------------------------
	// Data rows (starting at row 3)
	//-------------------------------------------------------------------------
	int32 RowNum = 3;
	for (const FNPCTableRow& Row : Rows)
	{
		SheetData += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);

		// Compute hash for this row (for 3-way merge)
		int64 RowHash = ComputeRowHash(Row);

		// Build column values in the same order as column definitions
		TArray<FString> Values = {
			// Sync column
			Row.RowId.ToString(),                                           // #ROW_GUID

			// Core Identity
			Row.NPCName,                                                    // NPC_NAME
			Row.NPCId,                                                      // NPC_ID
			Row.DisplayName,                                                // DISPLAY_NAME
			GetAssetName(Row.Blueprint),                                    // BLUEPRINT

			// AI & Behavior
			GetAssetName(Row.AbilityConfig),                                // ABILITY_CONFIG
			GetAssetName(Row.ActivityConfig),                               // ACTIVITY_CONFIG
			GetAssetName(Row.Schedule),                                     // SCHEDULE
			GetAssetName(Row.BehaviorTree),                                 // BEHAVIOR_TREE

			// Combat
			FString::Printf(TEXT("%d"), Row.MinLevel),                      // MIN_LEVEL
			FString::Printf(TEXT("%d"), Row.MaxLevel),                      // MAX_LEVEL
			Row.GetFactionsDisplay(),                                       // FACTIONS (short names)
			FString::Printf(TEXT("%.2f"), Row.AttackPriority),              // ATTACK_PRIORITY

			// Vendor
			Row.bIsVendor ? TEXT("Yes") : TEXT("No"),                       // IS_VENDOR
			Row.ShopName,                                                   // SHOP_NAME

			// Items & Spawning
			Row.DefaultItems,                                               // DEFAULT_ITEMS
			Row.SpawnerPOI,                                                 // SPAWNER_POI

			// Meta
			GetAssetName(Row.Appearance),                                   // APPEARANCE
			Row.Notes,                                                      // NOTES

			// Sync tracking
			TEXT("Synced"),                                                 // #STATE
			FString::Printf(TEXT("%lld"), RowHash),                         // #BASE_HASH
		};

		// Write all cell values
		for (int32 i = 0; i < Values.Num(); i++)
		{
			SheetData += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				*CellReference(i, RowNum), *EscapeXml(Values[i]));
		}

		SheetData += TEXT("</row>");
		RowNum++;
	}

	//-------------------------------------------------------------------------
	// Column width definitions
	//-------------------------------------------------------------------------
	FString ColsXml = TEXT("<cols>");
	for (int32 i = 0; i < Columns.Num(); i++)
	{
		ColsXml += FString::Printf(TEXT("<col min=\"%d\" max=\"%d\" width=\"%.1f\" customWidth=\"1\"/>"),
			i + 1, i + 1, Columns[i].Width);
	}
	ColsXml += TEXT("</cols>");

	// Assemble full worksheet XML
	return FString::Printf(TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  %s
  <sheetData>%s</sheetData>
</worksheet>)"), *ColsXml, *SheetData);
}

FString FNPCXLSXWriter::GenerateListsSheet(const TArray<FNPCTableRow>& Rows)
{
	// Collect unique values for potential dropdown validation
	TSet<FString> Blueprints;
	TSet<FString> AbilityConfigs;
	TSet<FString> Factions;
	TSet<FString> POIs;

	for (const FNPCTableRow& Row : Rows)
	{
		FString BPName = GetAssetName(Row.Blueprint);
		if (!BPName.IsEmpty()) Blueprints.Add(BPName);

		FString ACName = GetAssetName(Row.AbilityConfig);
		if (!ACName.IsEmpty()) AbilityConfigs.Add(ACName);

		// Parse faction tags (comma-separated)
		TArray<FString> FactionList;
		Row.Factions.ParseIntoArray(FactionList, TEXT(","));
		for (const FString& F : FactionList)
		{
			FString Trimmed = F.TrimStartAndEnd();
			if (!Trimmed.IsEmpty())
			{
				Factions.Add(FNPCTableRow::ToShortFactionName(Trimmed));
			}
		}

		if (!Row.SpawnerPOI.IsEmpty()) POIs.Add(Row.SpawnerPOI);
	}

	FString SheetData;

	// Header row (bold)
	SheetData += TEXT("<row r=\"1\">");
	SheetData += TEXT("<c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>BLUEPRINT</t></is></c>");
	SheetData += TEXT("<c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>ABILITY_CONFIG</t></is></c>");
	SheetData += TEXT("<c r=\"C1\" t=\"inlineStr\" s=\"1\"><is><t>FACTION</t></is></c>");
	SheetData += TEXT("<c r=\"D1\" t=\"inlineStr\" s=\"1\"><is><t>POI</t></is></c>");
	SheetData += TEXT("</row>");

	// Convert sets to arrays for indexed access
	TArray<FString> BlueprintArr = Blueprints.Array();
	TArray<FString> ACArr = AbilityConfigs.Array();
	TArray<FString> FactionArr = Factions.Array();
	TArray<FString> POIArr = POIs.Array();

	int32 MaxRows = FMath::Max(
		FMath::Max(BlueprintArr.Num(), ACArr.Num()),
		FMath::Max(FactionArr.Num(), POIArr.Num())
	);

	// Data rows
	for (int32 i = 0; i < MaxRows; i++)
	{
		int32 RowNum = i + 2;
		SheetData += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);

		if (i < BlueprintArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(BlueprintArr[i]));
		}
		if (i < ACArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(ACArr[i]));
		}
		if (i < FactionArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"C%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(FactionArr[i]));
		}
		if (i < POIArr.Num())
		{
			SheetData += FString::Printf(TEXT("<c r=\"D%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"),
				RowNum, *EscapeXml(POIArr[i]));
		}

		SheetData += TEXT("</row>");
	}

	return FString::Printf(TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <cols>
    <col min="1" max="1" width="25" customWidth="1"/>
    <col min="2" max="2" width="25" customWidth="1"/>
    <col min="3" max="3" width="20" customWidth="1"/>
    <col min="4" max="4" width="25" customWidth="1"/>
  </cols>
  <sheetData>%s</sheetData>
</worksheet>)"), *SheetData);
}

FString FNPCXLSXWriter::GenerateMetaSheet(const TArray<FNPCTableRow>& Rows)
{
	// Generate unique export identifier and timestamp
	FString ExportGuid = FGuid::NewGuid().ToString();
	FString ExportTime = FDateTime::Now().ToString();

	// Compute overall content hash (XOR of all row hashes)
	int64 ContentHash = 0;
	for (const FNPCTableRow& Row : Rows)
	{
		ContentHash ^= ComputeRowHash(Row);
	}

	FString SheetData;

	// Headers (bold)
	SheetData += TEXT("<row r=\"1\">");
	SheetData += TEXT("<c r=\"A1\" t=\"inlineStr\" s=\"1\"><is><t>Property</t></is></c>");
	SheetData += TEXT("<c r=\"B1\" t=\"inlineStr\" s=\"1\"><is><t>Value</t></is></c>");
	SheetData += TEXT("</row>");

	// Metadata key-value pairs
	TArray<TPair<FString, FString>> MetaValues = {
		{ TEXT("EXPORT_GUID"),    ExportGuid },
		{ TEXT("EXPORTED_AT"),    ExportTime },
		{ TEXT("FORMAT_VERSION"), FORMAT_VERSION },
		{ TEXT("ROW_COUNT"),      FString::Printf(TEXT("%d"), Rows.Num()) },
		{ TEXT("CONTENT_HASH"),   FString::Printf(TEXT("%lld"), ContentHash) },
		{ TEXT("TABLE_TYPE"),     TEXT("NPC") },
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

//=============================================================================
// Utility Functions
//=============================================================================

FString FNPCXLSXWriter::EscapeXml(const FString& Text)
{
	// Escape XML special characters to prevent malformed XML
	FString Result = Text;
	Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));   // Must be first!
	Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	Result.ReplaceInline(TEXT("\""), TEXT("&quot;"));
	Result.ReplaceInline(TEXT("'"), TEXT("&apos;"));
	return Result;
}

FString FNPCXLSXWriter::CellReference(int32 Col, int32 Row)
{
	// Combine column letter(s) with row number (e.g., "A1", "AA100")
	return ColumnLetter(Col) + FString::Printf(TEXT("%d"), Row);
}

FString FNPCXLSXWriter::ColumnLetter(int32 ColIndex)
{
	// Convert 0-based index to Excel column letters
	// 0 = "A", 25 = "Z", 26 = "AA", 27 = "AB", etc.
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

int64 FNPCXLSXWriter::ComputeRowHash(const FNPCTableRow& Row)
{
	// Hash all content fields (excludes RowId which is for identity tracking)
	// This hash is used for 3-way merge to detect changes
	int64 Hash = 0;

	// Core Identity
	Hash ^= GetTypeHash(Row.NPCName);
	Hash ^= GetTypeHash(Row.NPCId);
	Hash ^= GetTypeHash(Row.DisplayName);
	Hash ^= GetTypeHash(Row.Blueprint.ToString());

	// AI & Behavior
	Hash ^= GetTypeHash(Row.AbilityConfig.ToString());
	Hash ^= GetTypeHash(Row.ActivityConfig.ToString());
	Hash ^= GetTypeHash(Row.Schedule.ToString());
	Hash ^= GetTypeHash(Row.BehaviorTree.ToString());

	// Combat
	Hash ^= GetTypeHash(Row.MinLevel);
	Hash ^= GetTypeHash(Row.MaxLevel);
	Hash ^= GetTypeHash(Row.Factions);
	Hash ^= GetTypeHash(Row.AttackPriority);

	// Vendor
	Hash ^= GetTypeHash(Row.bIsVendor);
	Hash ^= GetTypeHash(Row.ShopName);

	// Items & Spawning
	Hash ^= GetTypeHash(Row.DefaultItems);
	Hash ^= GetTypeHash(Row.SpawnerPOI);

	// Meta
	Hash ^= GetTypeHash(Row.Appearance.ToString());
	Hash ^= GetTypeHash(Row.Notes);

	return Hash;
}

FString FNPCXLSXWriter::GetAssetName(const FSoftObjectPath& Path)
{
	// Extract just the asset name from a soft object path
	// Returns empty string if path is null/invalid
	if (Path.IsNull())
	{
		return FString();
	}
	return Path.GetAssetName();
}
