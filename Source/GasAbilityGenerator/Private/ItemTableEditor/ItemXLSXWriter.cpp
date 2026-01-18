// GasAbilityGenerator - Item XLSX Writer Implementation
// v4.12: Export Item table to Excel format

#include "ItemTableEditor/ItemXLSXWriter.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"

// Static constants
const FString FItemXLSXWriter::SHEET_SENTINEL = TEXT("#ITEM_SHEET_V1");
const FString FItemXLSXWriter::FORMAT_VERSION = TEXT("1.0");

TArray<FItemXLSXColumn> FItemXLSXWriter::GetColumnDefinitions()
{
	TArray<FItemXLSXColumn> Columns;

	// Sync columns (hidden from view but included for round-trip)
	Columns.Add(FItemXLSXColumn(TEXT("#ROW_GUID"), TEXT("Row GUID"), 36.0f));
	Columns.Add(FItemXLSXColumn(TEXT("#STATE"), TEXT("State"), 10.0f));
	Columns.Add(FItemXLSXColumn(TEXT("#BASE_HASH"), TEXT("Base Hash"), 15.0f));

	// Core Identity columns
	Columns.Add(FItemXLSXColumn(TEXT("ITEM_NAME"), TEXT("Item Name"), 20.0f));
	Columns.Add(FItemXLSXColumn(TEXT("DISPLAY_NAME"), TEXT("Display Name"), 25.0f));
	Columns.Add(FItemXLSXColumn(TEXT("ITEM_TYPE"), TEXT("Item Type"), 15.0f));
	Columns.Add(FItemXLSXColumn(TEXT("DESCRIPTION"), TEXT("Description"), 40.0f));

	// Equipment columns
	Columns.Add(FItemXLSXColumn(TEXT("EQUIPMENT_SLOT"), TEXT("Equipment Slot"), 30.0f));
	Columns.Add(FItemXLSXColumn(TEXT("BASE_VALUE"), TEXT("Base Value"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("WEIGHT"), TEXT("Weight"), 10.0f));
	Columns.Add(FItemXLSXColumn(TEXT("BASE_SCORE"), TEXT("Base Score"), 12.0f));

	// Combat stats
	Columns.Add(FItemXLSXColumn(TEXT("ATTACK_RATING"), TEXT("Attack Rating"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("ARMOR_RATING"), TEXT("Armor Rating"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("STEALTH_RATING"), TEXT("Stealth Rating"), 12.0f));

	// Weapon stats
	Columns.Add(FItemXLSXColumn(TEXT("ATTACK_DAMAGE"), TEXT("Attack Damage"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("HEAVY_ATTACK_MULT"), TEXT("Heavy Mult"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("WEAPON_HAND"), TEXT("Weapon Hand"), 15.0f));
	Columns.Add(FItemXLSXColumn(TEXT("CLIP_SIZE"), TEXT("Clip Size"), 10.0f));
	Columns.Add(FItemXLSXColumn(TEXT("REQUIRED_AMMO"), TEXT("Required Ammo"), 20.0f));
	Columns.Add(FItemXLSXColumn(TEXT("ALLOW_MANUAL_RELOAD"), TEXT("Manual Reload"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("BOT_ATTACK_RANGE"), TEXT("Bot Range"), 12.0f));

	// Ranged weapon stats
	Columns.Add(FItemXLSXColumn(TEXT("BASE_SPREAD"), TEXT("Base Spread"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("MAX_SPREAD"), TEXT("Max Spread"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("SPREAD_BUMP"), TEXT("Spread Bump"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("SPREAD_DECREASE"), TEXT("Spread Dec"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("AIM_FOV_PCT"), TEXT("Aim FOV %"), 12.0f));

	// Consumable stats
	Columns.Add(FItemXLSXColumn(TEXT("CONSUME_ON_USE"), TEXT("Consume"), 10.0f));
	Columns.Add(FItemXLSXColumn(TEXT("USE_RECHARGE"), TEXT("Recharge"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("CAN_ACTIVATE"), TEXT("Can Activate"), 12.0f));
	Columns.Add(FItemXLSXColumn(TEXT("GAMEPLAY_EFFECT"), TEXT("GE Class"), 25.0f));

	// References
	Columns.Add(FItemXLSXColumn(TEXT("MODIFIER_GE"), TEXT("Modifier GE"), 25.0f));
	Columns.Add(FItemXLSXColumn(TEXT("ABILITIES"), TEXT("Abilities"), 30.0f));
	Columns.Add(FItemXLSXColumn(TEXT("FRAGMENTS"), TEXT("Fragments"), 25.0f));

	// Tags & Stacking
	Columns.Add(FItemXLSXColumn(TEXT("ITEM_TAGS"), TEXT("Item Tags"), 30.0f));
	Columns.Add(FItemXLSXColumn(TEXT("STACKABLE"), TEXT("Stackable"), 10.0f));
	Columns.Add(FItemXLSXColumn(TEXT("MAX_STACK_SIZE"), TEXT("Max Stack"), 12.0f));

	// Meta
	Columns.Add(FItemXLSXColumn(TEXT("NOTES"), TEXT("Notes"), 40.0f));
	Columns.Add(FItemXLSXColumn(TEXT("DELETED"), TEXT("Deleted"), 10.0f));

	return Columns;
}

bool FItemXLSXWriter::ExportToXLSX(const TArray<FItemTableRow>& Rows, const FString& FilePath, FString& OutError)
{
	// Create temp directory for building XLSX structure
	FString TempDir = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("ItemXLSX_"));
	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*TempDir, true);

	// Create directory structure
	FM.MakeDirectory(*(TempDir / TEXT("_rels")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl/_rels")), true);
	FM.MakeDirectory(*(TempDir / TEXT("xl/worksheets")), true);

	// Write all XML files
	bool bSuccess = true;

	bSuccess &= FFileHelper::SaveStringToFile(GenerateContentTypesXml(), *(TempDir / TEXT("[Content_Types].xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateRelsXml(), *(TempDir / TEXT("_rels/.rels")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateWorkbookXml(), *(TempDir / TEXT("xl/workbook.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateWorkbookRelsXml(), *(TempDir / TEXT("xl/_rels/workbook.xml.rels")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateStylesXml(), *(TempDir / TEXT("xl/styles.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	// Write worksheets
	bSuccess &= FFileHelper::SaveStringToFile(GenerateItemsSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet1.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateListsSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet2.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bSuccess &= FFileHelper::SaveStringToFile(GenerateMetaSheet(Rows), *(TempDir / TEXT("xl/worksheets/sheet3.xml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	if (!bSuccess)
	{
		OutError = TEXT("Failed to write XLSX XML files");
		FM.DeleteDirectory(*TempDir, false, true);
		return false;
	}

	// Create ZIP archive using PowerShell
	FString ZipCommand;
#if PLATFORM_WINDOWS
	ZipCommand = FString::Printf(TEXT("powershell -ExecutionPolicy Bypass -Command \"Compress-Archive -Path '%s\\*' -DestinationPath '%s' -Force\""),
		*TempDir, *FilePath);
#else
	ZipCommand = FString::Printf(TEXT("cd '%s' && zip -r '%s' ."), *TempDir, *FilePath);
#endif

	int32 ReturnCode = 0;
	FString StdOut, StdErr;
	FPlatformProcess::ExecProcess(*ZipCommand, TEXT(""), &ReturnCode, &StdOut, &StdErr);

	// Clean up temp directory
	FM.DeleteDirectory(*TempDir, false, true);

	if (ReturnCode != 0)
	{
		OutError = FString::Printf(TEXT("Failed to create XLSX archive: %s"), *StdErr);
		return false;
	}

	return true;
}

FString FItemXLSXWriter::GenerateContentTypesXml()
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

FString FItemXLSXWriter::GenerateRelsXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)");
}

FString FItemXLSXWriter::GenerateWorkbookXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
<sheets>
<sheet name="Items" sheetId="1" r:id="rId1"/>
<sheet name="_Lists" sheetId="2" r:id="rId2"/>
<sheet name="_Meta" sheetId="3" r:id="rId3"/>
</sheets>
</workbook>)");
}

FString FItemXLSXWriter::GenerateWorkbookRelsXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet2.xml"/>
<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet3.xml"/>
<Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)");
}

FString FItemXLSXWriter::GenerateStylesXml()
{
	return TEXT(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
<fonts count="2">
<font><sz val="11"/><name val="Calibri"/></font>
<font><b/><sz val="11"/><name val="Calibri"/></font>
</fonts>
<fills count="3">
<fill><patternFill patternType="none"/></fill>
<fill><patternFill patternType="gray125"/></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FFE0E0E0"/></patternFill></fill>
</fills>
<borders count="1">
<border><left/><right/><top/><bottom/><diagonal/></border>
</borders>
<cellStyleXfs count="1">
<xf numFmtId="0" fontId="0" fillId="0" borderId="0"/>
</cellStyleXfs>
<cellXfs count="3">
<xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>
<xf numFmtId="0" fontId="1" fillId="2" borderId="0" xfId="0" applyFont="1" applyFill="1"/>
<xf numFmtId="0" fontId="1" fillId="0" borderId="0" xfId="0" applyFont="1"/>
</cellXfs>
</styleSheet>)");
}

FString FItemXLSXWriter::GenerateItemsSheet(const TArray<FItemTableRow>& Rows)
{
	TArray<FItemXLSXColumn> Columns = GetColumnDefinitions();
	FString SheetContent;

	SheetContent += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetContent += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetContent += TEXT("<sheetData>\n");

	int32 RowNum = 1;

	// Row 1: Sentinel + Column IDs
	SheetContent += FString::Printf(TEXT("<row r=\"%d\">\n"), RowNum);
	for (int32 ColIdx = 0; ColIdx < Columns.Num(); ColIdx++)
	{
		FString CellVal = (ColIdx == 0) ? SHEET_SENTINEL : Columns[ColIdx].ColumnId;
		SheetContent += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx, RowNum), *EscapeXml(CellVal));
	}
	SheetContent += TEXT("</row>\n");
	RowNum++;

	// Row 2: Human-readable headers
	SheetContent += FString::Printf(TEXT("<row r=\"%d\">\n"), RowNum);
	for (int32 ColIdx = 0; ColIdx < Columns.Num(); ColIdx++)
	{
		SheetContent += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\" s=\"1\"><is><t>%s</t></is></c>\n"),
			*CellReference(ColIdx, RowNum), *EscapeXml(Columns[ColIdx].DisplayName));
	}
	SheetContent += TEXT("</row>\n");
	RowNum++;

	// Data rows
	for (const FItemTableRow& Row : Rows)
	{
		SheetContent += FString::Printf(TEXT("<row r=\"%d\">\n"), RowNum);

		int32 ColIdx = 0;
		auto WriteCell = [&](const FString& Value)
		{
			SheetContent += FString::Printf(TEXT("<c r=\"%s\" t=\"inlineStr\"><is><t>%s</t></is></c>\n"),
				*CellReference(ColIdx++, RowNum), *EscapeXml(Value));
		};

		// Sync columns
		WriteCell(Row.RowId.ToString());
		WriteCell(Row.bDeleted ? TEXT("DELETED") : TEXT("ACTIVE"));
		WriteCell(FString::Printf(TEXT("%lld"), Row.LastSyncedHash));

		// Core Identity
		WriteCell(Row.ItemName);
		WriteCell(Row.DisplayName);
		WriteCell(Row.GetItemTypeString());
		WriteCell(Row.Description);

		// Equipment
		WriteCell(Row.EquipmentSlot);
		WriteCell(FString::FromInt(Row.BaseValue));
		WriteCell(FString::SanitizeFloat(Row.Weight));
		WriteCell(FString::SanitizeFloat(Row.BaseScore));

		// Combat stats
		WriteCell(FString::SanitizeFloat(Row.AttackRating));
		WriteCell(FString::SanitizeFloat(Row.ArmorRating));
		WriteCell(FString::SanitizeFloat(Row.StealthRating));

		// Weapon stats
		WriteCell(FString::SanitizeFloat(Row.AttackDamage));
		WriteCell(FString::SanitizeFloat(Row.HeavyAttackDamageMultiplier));
		WriteCell(Row.WeaponHand);
		WriteCell(FString::FromInt(Row.ClipSize));
		WriteCell(Row.RequiredAmmo);
		WriteCell(Row.bAllowManualReload ? TEXT("TRUE") : TEXT("FALSE"));
		WriteCell(FString::SanitizeFloat(Row.BotAttackRange));

		// Ranged weapon stats
		WriteCell(FString::SanitizeFloat(Row.BaseSpreadDegrees));
		WriteCell(FString::SanitizeFloat(Row.MaxSpreadDegrees));
		WriteCell(FString::SanitizeFloat(Row.SpreadFireBump));
		WriteCell(FString::SanitizeFloat(Row.SpreadDecreaseSpeed));
		WriteCell(FString::SanitizeFloat(Row.AimFOVPct));

		// Consumable stats
		WriteCell(Row.bConsumeOnUse ? TEXT("TRUE") : TEXT("FALSE"));
		WriteCell(FString::SanitizeFloat(Row.UseRechargeDuration));
		WriteCell(Row.bCanActivate ? TEXT("TRUE") : TEXT("FALSE"));
		WriteCell(Row.GameplayEffectClass);

		// References
		WriteCell(Row.ModifierGE.GetAssetName());
		WriteCell(Row.Abilities);
		WriteCell(Row.Fragments);

		// Tags & Stacking
		WriteCell(Row.ItemTags);
		WriteCell(Row.bStackable ? TEXT("TRUE") : TEXT("FALSE"));
		WriteCell(FString::FromInt(Row.MaxStackSize));

		// Meta
		WriteCell(Row.Notes);
		WriteCell(Row.bDeleted ? TEXT("TRUE") : TEXT("FALSE"));

		SheetContent += TEXT("</row>\n");
		RowNum++;
	}

	SheetContent += TEXT("</sheetData>\n");
	SheetContent += TEXT("</worksheet>");

	return SheetContent;
}

FString FItemXLSXWriter::GenerateListsSheet(const TArray<FItemTableRow>& Rows)
{
	FString SheetContent;

	SheetContent += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetContent += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetContent += TEXT("<sheetData>\n");

	// Item types dropdown
	SheetContent += TEXT("<row r=\"1\"><c r=\"A1\" t=\"inlineStr\"><is><t>ItemTypes</t></is></c></row>\n");
	TArray<FString> Types = { TEXT("Consumable"), TEXT("Ammo"), TEXT("WeaponAttachment"),
		TEXT("Equippable"), TEXT("Clothing"), TEXT("ThrowableWeapon"),
		TEXT("MeleeWeapon"), TEXT("RangedWeapon"), TEXT("MagicWeapon") };

	int32 Row = 2;
	for (const FString& Type : Types)
	{
		SheetContent += FString::Printf(TEXT("<row r=\"%d\"><c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"),
			Row, Row, *Type);
		Row++;
	}

	// Weapon hands dropdown
	SheetContent += FString::Printf(TEXT("<row r=\"%d\"><c r=\"B1\" t=\"inlineStr\"><is><t>WeaponHands</t></is></c></row>\n"), 1);
	TArray<FString> Hands = { TEXT("TwoHanded"), TEXT("MainHand"), TEXT("OffHand"), TEXT("DualWieldable") };
	Row = 2;
	for (const FString& Hand : Hands)
	{
		SheetContent += FString::Printf(TEXT("<row r=\"%d\"><c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"),
			Row, Row, *Hand);
		Row++;
	}

	// Collect unique equipment slots
	TSet<FString> UniqueSlots;
	for (const FItemTableRow& ItemRow : Rows)
	{
		if (!ItemRow.EquipmentSlot.IsEmpty())
		{
			UniqueSlots.Add(ItemRow.EquipmentSlot);
		}
	}

	SheetContent += FString::Printf(TEXT("<row r=\"%d\"><c r=\"C1\" t=\"inlineStr\"><is><t>EquipmentSlots</t></is></c></row>\n"), 1);
	Row = 2;
	for (const FString& Slot : UniqueSlots)
	{
		SheetContent += FString::Printf(TEXT("<row r=\"%d\"><c r=\"C%d\" t=\"inlineStr\"><is><t>%s</t></is></c></row>\n"),
			Row, Row, *EscapeXml(Slot));
		Row++;
	}

	SheetContent += TEXT("</sheetData>\n");
	SheetContent += TEXT("</worksheet>");

	return SheetContent;
}

FString FItemXLSXWriter::GenerateMetaSheet(const TArray<FItemTableRow>& Rows)
{
	FString SheetContent;

	SheetContent += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	SheetContent += TEXT("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n");
	SheetContent += TEXT("<sheetData>\n");

	// Compute content hash
	uint32 ContentHash = 0;
	for (const FItemTableRow& Row : Rows)
	{
		ContentHash = HashCombine(ContentHash, Row.ComputeEditableFieldsHash());
	}

	// Meta rows
	auto WriteMetaRow = [&](int32 RowNum, const FString& Key, const FString& Value)
	{
		SheetContent += FString::Printf(TEXT("<row r=\"%d\">"), RowNum);
		SheetContent += FString::Printf(TEXT("<c r=\"A%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"), RowNum, *Key);
		SheetContent += FString::Printf(TEXT("<c r=\"B%d\" t=\"inlineStr\"><is><t>%s</t></is></c>"), RowNum, *EscapeXml(Value));
		SheetContent += TEXT("</row>\n");
	};

	WriteMetaRow(1, TEXT("ExportGuid"), FGuid::NewGuid().ToString());
	WriteMetaRow(2, TEXT("ExportTimestamp"), FDateTime::Now().ToString());
	WriteMetaRow(3, TEXT("FormatVersion"), FORMAT_VERSION);
	WriteMetaRow(4, TEXT("ContentHash"), FString::FromInt(ContentHash));
	WriteMetaRow(5, TEXT("RowCount"), FString::FromInt(Rows.Num()));
	WriteMetaRow(6, TEXT("SheetSentinel"), SHEET_SENTINEL);

	SheetContent += TEXT("</sheetData>\n");
	SheetContent += TEXT("</worksheet>");

	return SheetContent;
}

FString FItemXLSXWriter::EscapeXml(const FString& Text)
{
	FString Result = Text;
	Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	Result.ReplaceInline(TEXT("\""), TEXT("&quot;"));
	Result.ReplaceInline(TEXT("'"), TEXT("&apos;"));
	return Result;
}

FString FItemXLSXWriter::CellReference(int32 Col, int32 Row)
{
	return FString::Printf(TEXT("%s%d"), *ColumnLetter(Col), Row);
}

FString FItemXLSXWriter::ColumnLetter(int32 ColIndex)
{
	FString Result;
	int32 Idx = ColIndex;
	while (Idx >= 0)
	{
		Result = FString::Chr(TEXT('A') + (Idx % 26)) + Result;
		Idx = Idx / 26 - 1;
	}
	return Result;
}

int64 FItemXLSXWriter::ComputeRowHash(const FItemTableRow& Row)
{
	return static_cast<int64>(Row.ComputeEditableFieldsHash());
}
