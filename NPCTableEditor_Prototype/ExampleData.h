// ExampleData.h
// PROTOTYPE - Example NPC data for testing
// This shows what the table would look like with real Narrative Pro NPCs
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "NPCTableEditorTypes.h"

/**
 * Populate table with example Narrative Pro NPCs
 * Call this to see what the table would look like with real data
 */
inline void PopulateExampleData(UNPCTableData* TableData)
{
	if (!TableData) return;

	TableData->Rows.Empty();

	//=========================================================================
	// Town NPCs
	//=========================================================================

	// Blacksmith
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Blacksmith");
		Row.NPCId = TEXT("blacksmith_01");
		Row.DisplayName = TEXT("Garrett the Blacksmith");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_Blacksmith.BP_Blacksmith"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Craftsman.AC_Craftsman"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Worker.ActConfig_Worker"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_DayWorker.Schedule_DayWorker"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_Blacksmith.DBP_Blacksmith"));
		Row.SpawnerPOI = TEXT("POI_Town_Forge");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = true;
		Row.ShopName = TEXT("Garrett's Forge");
		Row.TradingCurrency = 500;
		Row.DefaultItems = TEXT("EI_Hammer, EI_Apron");
		Row.ShopItems = TEXT("IC_Weapons_Common, IC_Armor_Light");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town");
		Row.MinLevel = 1;
		Row.MaxLevel = 1;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Main town blacksmith, sells weapons and repairs");
	}

	// Merchant
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Merchant");
		Row.NPCId = TEXT("merchant_01");
		Row.DisplayName = TEXT("Helena the Trader");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_Merchant.BP_Merchant"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Civilian.AC_Civilian"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Vendor.ActConfig_Vendor"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_DayWorker.Schedule_DayWorker"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_Merchant.DBP_Merchant"));
		Row.SpawnerPOI = TEXT("POI_Town_Market");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = true;
		Row.ShopName = TEXT("Helena's Goods");
		Row.TradingCurrency = 1000;
		Row.DefaultItems = TEXT("EI_Pouch");
		Row.ShopItems = TEXT("IC_Potions, IC_Food, IC_Materials");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town");
		Row.MinLevel = 1;
		Row.MaxLevel = 1;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("General goods merchant at the market");
	}

	// Innkeeper
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Innkeeper");
		Row.NPCId = TEXT("innkeeper_01");
		Row.DisplayName = TEXT("Marcus the Innkeeper");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_Innkeeper.BP_Innkeeper"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Civilian.AC_Civilian"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Innkeeper.ActConfig_Innkeeper"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_Innkeeper.Schedule_Innkeeper"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_Innkeeper.DBP_Innkeeper"));
		Row.SpawnerPOI = TEXT("POI_Town_Inn");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = true;
		Row.ShopName = TEXT("The Rusty Anchor");
		Row.TradingCurrency = 300;
		Row.DefaultItems = TEXT("EI_Mug, EI_Apron");
		Row.ShopItems = TEXT("IC_Food, IC_Drinks");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town");
		Row.MinLevel = 1;
		Row.MaxLevel = 1;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Sells food/drinks, provides rumors");
	}

	//=========================================================================
	// Guards
	//=========================================================================

	// Town Guard (Gate)
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Guard_Gate");
		Row.NPCId = TEXT("guard_gate_01");
		Row.DisplayName = TEXT("Town Guard");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_TownGuard.BP_TownGuard"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Guard.AC_Guard"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_GuardStationary.ActConfig_GuardStationary"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_Guard_Day.Schedule_Guard_Day"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_Guard_Generic.DBP_Guard_Generic"));
		Row.SpawnerPOI = TEXT("POI_Town_Gate");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_Sword, EI_Shield, EI_GuardArmor");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town, Narrative.Factions.Guard");
		Row.MinLevel = 3;
		Row.MaxLevel = 5;
		Row.AttackPriority = 0.7f;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Stationary guard at town gate");
	}

	// Town Guard (Patrol)
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Guard_Patrol");
		Row.NPCId = TEXT("guard_patrol_01");
		Row.DisplayName = TEXT("Town Guard");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_TownGuard.BP_TownGuard"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Guard.AC_Guard"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_GuardPatrol.ActConfig_GuardPatrol"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_Guard_Night.Schedule_Guard_Night"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_Guard_Generic.DBP_Guard_Generic"));
		Row.SpawnerPOI = TEXT("POI_Town_Square");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_Sword, EI_Torch, EI_GuardArmor");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town, Narrative.Factions.Guard");
		Row.MinLevel = 3;
		Row.MaxLevel = 5;
		Row.AttackPriority = 0.7f;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Patrolling guard, active at night");
	}

	//=========================================================================
	// Quest NPCs
	//=========================================================================

	// Quest Giver - Old Man
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("OldMan_Quest");
		Row.NPCId = TEXT("oldman_quest_01");
		Row.DisplayName = TEXT("Elder Thomas");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_OldMan.BP_OldMan"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_Civilian.AC_Civilian"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Idle.ActConfig_Idle"));
		Row.Schedule = FSoftObjectPath(TEXT("/Game/NPCs/Schedules/Schedule_Elder.Schedule_Elder"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_ElderThomas.DBP_ElderThomas"));
		Row.SpawnerPOI = TEXT("POI_Town_Church");
		Row.LevelName = TEXT("Town_Main");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_Staff, EI_Robe");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Town");
		Row.MinLevel = 1;
		Row.MaxLevel = 1;
		Row.AttackPriority = 0.1f;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Gives main story quest, important NPC");
	}

	//=========================================================================
	// Enemies
	//=========================================================================

	// Bandit
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Bandit_Melee");
		Row.NPCId = TEXT("bandit_melee_01");
		Row.DisplayName = TEXT("Bandit");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_Bandit.BP_Bandit"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_BanditMelee.AC_BanditMelee"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Aggressive.ActConfig_Aggressive"));
		Row.SpawnerPOI = TEXT("POI_Wilderness_Camp");
		Row.LevelName = TEXT("Wilderness_01");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_RustySword, EI_LeatherArmor, IC_LootCommon");
		Row.Factions = TEXT("Narrative.Factions.Hostile, Narrative.Factions.Bandits");
		Row.MinLevel = 2;
		Row.MaxLevel = 4;
		Row.AttackPriority = 0.8f;
		Row.Status = TEXT("New");
		Row.Notes = TEXT("Basic melee enemy");
	}

	// Bandit Archer
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Bandit_Archer");
		Row.NPCId = TEXT("bandit_archer_01");
		Row.DisplayName = TEXT("Bandit Archer");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_BanditArcher.BP_BanditArcher"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_BanditRanged.AC_BanditRanged"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Ranged.ActConfig_Ranged"));
		Row.SpawnerPOI = TEXT("POI_Wilderness_Camp");
		Row.LevelName = TEXT("Wilderness_01");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_Bow, EI_Arrows, EI_LeatherArmor, IC_LootCommon");
		Row.Factions = TEXT("Narrative.Factions.Hostile, Narrative.Factions.Bandits");
		Row.MinLevel = 2;
		Row.MaxLevel = 4;
		Row.AttackPriority = 0.6f;
		Row.Status = TEXT("New");
		Row.Notes = TEXT("Ranged enemy, keeps distance");
	}

	// Bandit Leader
	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Bandit_Leader");
		Row.NPCId = TEXT("bandit_leader_01");
		Row.DisplayName = TEXT("Bandit Chief");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/NPCs/Blueprints/BP_BanditLeader.BP_BanditLeader"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/AC_BanditElite.AC_BanditElite"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/NPCs/Configs/ActConfig_Boss.ActConfig_Boss"));
		Row.Dialogue = FSoftObjectPath(TEXT("/Game/NPCs/Dialogues/DBP_BanditLeader.DBP_BanditLeader"));
		Row.SpawnerPOI = TEXT("POI_Wilderness_Camp_Boss");
		Row.LevelName = TEXT("Wilderness_01");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("EI_Axe, EI_ChainArmor, IC_LootRare");
		Row.Factions = TEXT("Narrative.Factions.Hostile, Narrative.Factions.Bandits");
		Row.MinLevel = 5;
		Row.MaxLevel = 7;
		Row.AttackPriority = 0.9f;
		Row.Status = TEXT("Modified");
		Row.Notes = TEXT("Mini-boss, has dialogue before fight");
	}

	//=========================================================================
	// Father Companion (from your project)
	//=========================================================================

	{
		FNPCTableRow& Row = TableData->AddRow();
		Row.NPCName = TEXT("Father");
		Row.NPCId = TEXT("father_companion");
		Row.DisplayName = TEXT("Father");
		Row.NPCBlueprint = FSoftObjectPath(TEXT("/Game/FatherCompanion/Blueprints/BP_FatherCompanion.BP_FatherCompanion"));
		Row.AbilityConfig = FSoftObjectPath(TEXT("/Game/FatherCompanion/Configs/AC_FatherCrawler.AC_FatherCrawler"));
		Row.ActivityConfig = FSoftObjectPath(TEXT("/Game/FatherCompanion/Configs/ActConfig_Father.ActConfig_Father"));
		Row.SpawnerPOI = TEXT("Player_Companion");
		Row.LevelName = TEXT("*");
		Row.bIsVendor = false;
		Row.DefaultItems = TEXT("");
		Row.Factions = TEXT("Narrative.Factions.Friendly, Narrative.Factions.Player");
		Row.MinLevel = 1;
		Row.MaxLevel = 50;
		Row.AttackPriority = 0.3f;
		Row.Status = TEXT("Synced");
		Row.Notes = TEXT("Spider companion, 5 forms, follows player");
	}
}

/**
 * Example of what the CSV export would look like
 */
inline FString GetExampleCSV()
{
	return TEXT(
		"NPCName,NPCId,DisplayName,SpawnerPOI,LevelName,IsVendor,ShopName,DefaultItems,Factions,MinLevel,MaxLevel,Notes\n"
		"Blacksmith,blacksmith_01,Garrett the Blacksmith,POI_Town_Forge,Town_Main,TRUE,Garrett's Forge,\"EI_Hammer, EI_Apron\",\"Narrative.Factions.Friendly, Narrative.Factions.Town\",1,1,Main town blacksmith\n"
		"Merchant,merchant_01,Helena the Trader,POI_Town_Market,Town_Main,TRUE,Helena's Goods,EI_Pouch,\"Narrative.Factions.Friendly, Narrative.Factions.Town\",1,1,General goods merchant\n"
		"Guard_Gate,guard_gate_01,Town Guard,POI_Town_Gate,Town_Main,FALSE,,\"EI_Sword, EI_Shield, EI_GuardArmor\",\"Narrative.Factions.Friendly, Narrative.Factions.Guard\",3,5,Stationary guard\n"
		"Bandit_Melee,bandit_melee_01,Bandit,POI_Wilderness_Camp,Wilderness_01,FALSE,,\"EI_RustySword, EI_LeatherArmor\",Narrative.Factions.Hostile,2,4,Basic melee enemy\n"
		"Father,father_companion,Father,Player_Companion,*,FALSE,,,Narrative.Factions.Friendly,1,50,Spider companion\n"
	);
}
