// GasAbilityGenerator v2.8.2
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v2.8.2: Added nested parameters support for CallFunction nodes (parameters: section under properties:)
// v2.6.14: Prefix validation for all asset types (E_, IA_, IMC_, GE_, GA_, BP_, WBP_, BB_, BT_, M_, MF_, NS_, etc.)
// v2.5.6: Added NPC system extensions - suffix-based section names (_tags, _gameplay_effects, etc.)
// v2.3.0: Added 12 new asset type parsers with dependency-based generation order
// v2.2.0: Added event graph parsing support
// v2.1.9: Added manifest validation - incremental files must have assets in main manifest whitelist
// v2.1.8: Added enumeration parsing support
// v2.1.7: Fixed variable subsection exit - now properly detects new blueprint items by indent level

#include "GasAbilityGeneratorParser.h"

bool FGasAbilityGeneratorParser::ParseManifest(const FString& ManifestContent, FManifestData& OutData)
{
	// v2.1.3 FIX: Clear all arrays before populating to prevent duplicates on re-parse
	OutData.Tags.Empty();
	OutData.Enumerations.Empty();  // v2.1.8
	OutData.InputActions.Empty();
	OutData.InputMappingContexts.Empty();
	OutData.GameplayEffects.Empty();
	OutData.GameplayAbilities.Empty();
	OutData.ActorBlueprints.Empty();
	OutData.WidgetBlueprints.Empty();
	OutData.Blackboards.Empty();
	OutData.BehaviorTrees.Empty();
	OutData.Materials.Empty();
	OutData.EventGraphs.Empty();  // v2.2.0
	// v2.3.0: Clear new asset type arrays
	OutData.FloatCurves.Empty();
	OutData.AnimationMontages.Empty();
	OutData.AnimationNotifies.Empty();
	OutData.DialogueBlueprints.Empty();
	OutData.EquippableItems.Empty();
	OutData.Activities.Empty();
	OutData.AbilityConfigurations.Empty();
	OutData.ActivityConfigurations.Empty();
	OutData.ItemCollections.Empty();
	OutData.NarrativeEvents.Empty();
	OutData.NPCDefinitions.Empty();
	OutData.CharacterDefinitions.Empty();
	OutData.ProjectRoot.Empty();
	OutData.TagsIniPath.Empty();
	
	TArray<FString> Lines;
	ManifestContent.ParseIntoArrayLines(Lines);
	
	for (int32 i = 0; i < Lines.Num(); i++)
	{
		const FString& Line = Lines[i];
		FString TrimmedLine = Line.TrimStart();
		
		// Skip empty lines and comments
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			continue;
		}
		
		// Parse project_root
		if (TrimmedLine.StartsWith(TEXT("project_root:")))
		{
			OutData.ProjectRoot = GetLineValue(TrimmedLine);
		}
		// Parse tags_ini_path
		else if (TrimmedLine.StartsWith(TEXT("tags_ini_path:")))
		{
			OutData.TagsIniPath = GetLineValue(TrimmedLine);
		}
		// Parse tags section
		else if (IsSectionHeader(TrimmedLine, TEXT("tags:")))
		{
			ParseTags(Lines, i, OutData);
		}
		// v2.1.8: Parse enumerations section
		else if (IsSectionHeader(TrimmedLine, TEXT("enumerations:")))
		{
			ParseEnumerations(Lines, i, OutData);
		}
		// Parse input_actions section
		else if (IsSectionHeader(TrimmedLine, TEXT("input_actions:")))
		{
			ParseInputActions(Lines, i, OutData);
		}
		// Parse input_mapping_contexts section
		else if (IsSectionHeader(TrimmedLine, TEXT("input_mapping_contexts:")))
		{
			ParseInputMappingContexts(Lines, i, OutData);
		}
		// Parse gameplay_effects section
		else if (IsSectionHeader(TrimmedLine, TEXT("gameplay_effects:")))
		{
			ParseGameplayEffects(Lines, i, OutData);
		}
		// Parse gameplay_abilities section
		else if (IsSectionHeader(TrimmedLine, TEXT("gameplay_abilities:")))
		{
			ParseGameplayAbilities(Lines, i, OutData);
		}
		// Parse actor_blueprints or blueprints section
		else if (IsSectionHeader(TrimmedLine, TEXT("actor_blueprints:")) || 
		         IsSectionHeader(TrimmedLine, TEXT("blueprints:")))
		{
			ParseActorBlueprints(Lines, i, OutData);
		}
		// Parse widget_blueprints or widgets section
		else if (IsSectionHeader(TrimmedLine, TEXT("widget_blueprints:")) || 
		         IsSectionHeader(TrimmedLine, TEXT("widgets:")))
		{
			ParseWidgetBlueprints(Lines, i, OutData);
		}
		// Parse blackboards section
		else if (IsSectionHeader(TrimmedLine, TEXT("blackboards:")))
		{
			ParseBlackboards(Lines, i, OutData);
		}
		// Parse behavior_trees section
		else if (IsSectionHeader(TrimmedLine, TEXT("behavior_trees:")))
		{
			ParseBehaviorTrees(Lines, i, OutData);
		}
		// Parse materials section
		else if (IsSectionHeader(TrimmedLine, TEXT("materials:")))
		{
			ParseMaterials(Lines, i, OutData);
		}
		// v2.6.12: Parse material_functions section
		else if (IsSectionHeader(TrimmedLine, TEXT("material_functions:")))
		{
			ParseMaterialFunctions(Lines, i, OutData);
		}
		// v4.9: Parse material_instances section
		else if (IsSectionHeader(TrimmedLine, TEXT("material_instances:")))
		{
			ParseMaterialInstances(Lines, i, OutData);
		}
		// v2.2.0: Parse event_graphs section
		else if (IsSectionHeader(TrimmedLine, TEXT("event_graphs:")))
		{
			ParseEventGraphs(Lines, i, OutData);
		}
		// v2.3.0: Parse new asset type sections
		else if (IsSectionHeader(TrimmedLine, TEXT("float_curves:")))
		{
			ParseFloatCurves(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("animation_montages:")))
		{
			ParseAnimationMontages(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("animation_notifies:")))
		{
			ParseAnimationNotifies(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("dialogue_blueprints:")))
		{
			ParseDialogueBlueprints(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("equippable_items:")))
		{
			ParseEquippableItems(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("activities:")))
		{
			ParseActivities(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("ability_configurations:")))
		{
			ParseAbilityConfigurations(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("activity_configurations:")))
		{
			ParseActivityConfigurations(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("item_collections:")))
		{
			ParseItemCollections(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("narrative_events:")))
		{
			ParseNarrativeEvents(Lines, i, OutData);
		}
		// v4.0: Gameplay Cues
		else if (IsSectionHeader(TrimmedLine, TEXT("gameplay_cues:")))
		{
			ParseGameplayCues(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("npc_definitions:")))
		{
			ParseNPCDefinitions(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("character_definitions:")))
		{
			ParseCharacterDefinitions(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("tagged_dialogue_sets:")))
		{
			ParseTaggedDialogueSets(Lines, i, OutData);
		}
		// v2.6.5: Niagara System generation
		else if (IsSectionHeader(TrimmedLine, TEXT("niagara_systems:")))
		{
			ParseNiagaraSystems(Lines, i, OutData);
		}
		// v4.9: FX Presets (reusable Niagara parameter configurations)
		else if (IsSectionHeader(TrimmedLine, TEXT("fx_presets:")))
		{
			ParseFXPresets(Lines, i, OutData);
		}
		// v3.9: NPC Pipeline - Schedules, Goals, Quests
		else if (IsSectionHeader(TrimmedLine, TEXT("activity_schedules:")))
		{
			ParseActivitySchedules(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("goal_items:")) || IsSectionHeader(TrimmedLine, TEXT("goals:")))
		{
			ParseGoalItems(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("quests:")))
		{
			ParseQuests(Lines, i, OutData);
		}
		// v4.8.3: Character Appearances
		else if (IsSectionHeader(TrimmedLine, TEXT("character_appearances:")))
		{
			ParseCharacterAppearances(Lines, i, OutData);
		}
		// v4.9: TriggerSets (event-driven NPC behaviors)
		else if (IsSectionHeader(TrimmedLine, TEXT("trigger_sets:")))
		{
			ParseTriggerSets(Lines, i, OutData);
		}
		// v4.13: Category C - FormStateEffects (P1.1)
		else if (IsSectionHeader(TrimmedLine, TEXT("form_state_effects:")))
		{
			ParseFormStateEffects(Lines, i, OutData);
		}
		// v3.9.8: Mesh-to-Item Pipeline parsers
		else if (IsSectionHeader(TrimmedLine, TEXT("pipeline_config:")))
		{
			ParsePipelineConfig(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("pipeline_items:")))
		{
			ParsePipelineItems(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("pipeline_collections:")))
		{
			ParsePipelineCollections(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("pipeline_loadouts:")))
		{
			ParsePipelineLoadouts(Lines, i, OutData);
		}
		// v3.9.9: POI & NPC Spawner Placements
		else if (IsSectionHeader(TrimmedLine, TEXT("poi_placements:")))
		{
			ParsePOIPlacements(Lines, i, OutData);
		}
		else if (IsSectionHeader(TrimmedLine, TEXT("npc_spawner_placements:")) || IsSectionHeader(TrimmedLine, TEXT("spawner_placements:")))
		{
			ParseNPCSpawnerPlacements(Lines, i, OutData);
		}
		// v2.5.6: NPC System Extensions - Support for suffix-based section names
		// Any section ending with _tags: gets parsed as tags
		else if (TrimmedLine.EndsWith(TEXT("_tags:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseTags(Lines, i, OutData);
		}
		// Any section ending with _gameplay_effects: gets parsed as gameplay effects
		else if (TrimmedLine.EndsWith(TEXT("_gameplay_effects:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseGameplayEffects(Lines, i, OutData);
		}
		// Any section ending with _gameplay_abilities: gets parsed as gameplay abilities
		else if (TrimmedLine.EndsWith(TEXT("_gameplay_abilities:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseGameplayAbilities(Lines, i, OutData);
		}
		// Any section ending with _actor_blueprints: gets parsed as actor blueprints
		else if (TrimmedLine.EndsWith(TEXT("_actor_blueprints:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActorBlueprints(Lines, i, OutData);
		}
		// Any section ending with _blackboards: gets parsed as blackboards
		else if (TrimmedLine.EndsWith(TEXT("_blackboards:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseBlackboards(Lines, i, OutData);
		}
		// Any section ending with _behavior_trees: gets parsed as behavior trees
		else if (TrimmedLine.EndsWith(TEXT("_behavior_trees:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseBehaviorTrees(Lines, i, OutData);
		}
		// Any section ending with _activities: gets parsed as activities
		else if (TrimmedLine.EndsWith(TEXT("_activities:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActivities(Lines, i, OutData);
		}
		// Any section ending with _ability_configurations: gets parsed as ability configurations
		else if (TrimmedLine.EndsWith(TEXT("_ability_configurations:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseAbilityConfigurations(Lines, i, OutData);
		}
		// Any section ending with _activity_configurations: gets parsed as activity configurations
		else if (TrimmedLine.EndsWith(TEXT("_activity_configurations:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActivityConfigurations(Lines, i, OutData);
		}
		// Any section ending with _npc_definitions: gets parsed as NPC definitions
		else if (TrimmedLine.EndsWith(TEXT("_npc_definitions:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseNPCDefinitions(Lines, i, OutData);
		}
		// Any section ending with _materials: gets parsed as materials
		else if (TrimmedLine.EndsWith(TEXT("_materials:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseMaterials(Lines, i, OutData);
		}
		// Any section ending with _goals: gets parsed as actor blueprints (goals are blueprint classes)
		else if (TrimmedLine.EndsWith(TEXT("_goals:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActorBlueprints(Lines, i, OutData);
		}
		// Any section ending with _goal_generators: gets parsed as actor blueprints
		else if (TrimmedLine.EndsWith(TEXT("_goal_generators:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActorBlueprints(Lines, i, OutData);
		}
		// Any section ending with _bt_services: gets parsed as actor blueprints (BT services are blueprints)
		else if (TrimmedLine.EndsWith(TEXT("_bt_services:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseActorBlueprints(Lines, i, OutData);
		}
		// Any section ending with _tagged_dialogue_sets: gets parsed as tagged dialogue sets
		else if (TrimmedLine.EndsWith(TEXT("_tagged_dialogue_sets:")) && !TrimmedLine.StartsWith(TEXT("-")))
		{
			ParseTaggedDialogueSets(Lines, i, OutData);
		}
	}

	return true;
}

bool FGasAbilityGeneratorParser::ParseIncrementalFile(const FString& YamlContent, FString& OutAssetName, FManifestData& OutData)
{
	// Parse the incremental file to find asset_name and merge with manifest
	TArray<FString> Lines;
	YamlContent.ParseIntoArrayLines(Lines);

	for (int32 i = 0; i < Lines.Num(); i++)
	{
		const FString& Line = Lines[i];
		FString TrimmedLine = Line.TrimStart();

		if (TrimmedLine.StartsWith(TEXT("asset_name:")))
		{
			OutAssetName = GetLineValue(TrimmedLine);
			break;
		}
	}

	if (OutAssetName.IsEmpty())
	{
		return false;
	}

	// v2.1.9: Validate incremental asset against manifest whitelist
	// Assets from incremental files must also be in the main manifest
	if (!OutData.IsAssetInManifest(OutAssetName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GasAbilityGenerator] BLOCKED: Incremental asset '%s' not in manifest whitelist"), *OutAssetName);
		return false;
	}

	// Determine asset type from prefix and parse accordingly
	if (OutAssetName.StartsWith(TEXT("GA_")))
	{
		FManifestGameplayAbilityDefinition Def;
		Def.Name = OutAssetName;
		// Parse additional fields from the YAML
		for (const FString& Line : Lines)
		{
			FString TrimmedLine = Line.TrimStart();
			if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				Def.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				Def.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("instancing_policy:")))
			{
				Def.InstancingPolicy = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("net_execution_policy:")))
			{
				Def.NetExecutionPolicy = GetLineValue(TrimmedLine);
			}
		}
		OutData.GameplayAbilities.Add(Def);
	}
	else if (OutAssetName.StartsWith(TEXT("GE_")))
	{
		FManifestGameplayEffectDefinition Def;
		Def.Name = OutAssetName;
		for (const FString& Line : Lines)
		{
			FString TrimmedLine = Line.TrimStart();
			if (TrimmedLine.StartsWith(TEXT("duration_policy:")))
			{
				Def.DurationPolicy = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("duration_magnitude:")))
			{
				Def.DurationMagnitude = FCString::Atof(*GetLineValue(TrimmedLine));
			}
		}
		OutData.GameplayEffects.Add(Def);
	}
	// Add more asset type handling as needed
	
	return true;
}

void FGasAbilityGeneratorParser::ParseTags(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++; // Move past section header
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		
		// v2.0.9 FIX: Section exit logic
		if (ShouldExitSection(Line, SectionIndent))
		{
			LineIndex--; // Back up so main loop can process this line
			return;
		}
		
		// Skip empty lines and comments
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}
		
		// Parse array items
		if (IsArrayItem(Line))
		{
			FString RawValue = GetArrayItemValue(Line);
			
			// v2.1.2 FIX: Extract tag value from various manifest formats
			// Handles: "- tag: Ability.X", "- path: \"Ability.X\"", "- Ability.X"
			FString Tag = RawValue;
			
			// Strip "tag:" prefix if present
			if (Tag.StartsWith(TEXT("tag:")))
			{
				Tag = Tag.Mid(4).TrimStart();
			}
			// Strip "path:" prefix if present
			else if (Tag.StartsWith(TEXT("path:")))
			{
				Tag = Tag.Mid(5).TrimStart();
			}
			
			// Strip surrounding quotes if present
			if (Tag.Len() >= 2 && Tag.StartsWith(TEXT("\"")) && Tag.EndsWith(TEXT("\"")))
			{
				Tag = Tag.Mid(1, Tag.Len() - 2);
			}
			
			if (!Tag.IsEmpty())
			{
				OutData.Tags.AddUnique(Tag);
			}
		}
		
		LineIndex++;
	}
}

// v2.1.8: Parse enumerations section
void FGasAbilityGeneratorParser::ParseEnumerations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;
	
	FManifestEnumerationDefinition CurrentDef;
	bool bInItem = false;
	bool bInValues = false;
	int32 ItemIndent = -1;
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);
		
		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if E_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("E_")))
			{
				OutData.Enumerations.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}
		
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}
		
		// Check if this is a new enum item by comparing indent level
		bool bIsNewEnumItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewEnumItem = true;
		}
		
		if (bIsNewEnumItem)
		{
			// Save current enum and start new one - v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("E_")))
			{
				OutData.Enumerations.Add(CurrentDef);
			}
			CurrentDef = FManifestEnumerationDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInValues = false;
		}
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			ItemIndent = CurrentIndent;
			CurrentDef = FManifestEnumerationDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("values:")) || TrimmedLine.StartsWith(TEXT("values:")))
			{
				bInValues = true;
			}
			else if (bInValues && TrimmedLine.StartsWith(TEXT("-")))
			{
				// Parse enum value
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Value = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				// Remove quotes if present
				if (Value.Len() >= 2 && Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")))
				{
					Value = Value.Mid(1, Value.Len() - 2);
				}
				else if (Value.Len() >= 2 && Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))
				{
					Value = Value.Mid(1, Value.Len() - 2);
				}
				if (!Value.IsEmpty())
				{
					CurrentDef.Values.Add(Value);
				}
			}
		}
		
		LineIndex++;
	}
	
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("E_")))
	{
		OutData.Enumerations.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseInputActions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;
	
	FManifestInputActionDefinition CurrentDef;
	bool bInItem = false;
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		
		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if IA_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IA_")))
			{
				OutData.InputActions.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IA_")))
			{
				OutData.InputActions.Add(CurrentDef);
			}
			CurrentDef = FManifestInputActionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2)); // Skip "- "
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("value_type:")))
			{
				CurrentDef.ValueType = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("trigger_type:")))
			{
				CurrentDef.TriggerType = GetLineValue(TrimmedLine);
			}
		}
		
		LineIndex++;
	}
	
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IA_")))
	{
		OutData.InputActions.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseInputMappingContexts(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;
	
	FManifestInputMappingContextDefinition CurrentDef;
	bool bInItem = false;
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		
		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if IMC_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IMC_")))
			{
				OutData.InputMappingContexts.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IMC_")))
			{
				OutData.InputMappingContexts.Add(CurrentDef);
			}
			CurrentDef = FManifestInputMappingContextDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("IMC_")))
	{
		OutData.InputMappingContexts.Add(CurrentDef);
	}
}

// v2.3.0: Comprehensive GE parser with all fields
void FGasAbilityGeneratorParser::ParseGameplayEffects(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestGameplayEffectDefinition CurrentDef;
	FManifestModifierDefinition CurrentModifier;
	bool bInItem = false;
	bool bInModifiers = false;
	bool bInGrantedTags = false;
	bool bInRemoveTags = false;
	bool bInExecutions = false;
	bool bInSetByCallerTags = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending modifier
			if (bInModifiers && !CurrentModifier.Attribute.IsEmpty())
			{
				CurrentDef.Modifiers.Add(CurrentModifier);
			}
			// v2.6.14: Prefix validation - only add if GE_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GE_")))
			{
				OutData.GameplayEffects.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New GE item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save pending modifier
			if (bInModifiers && !CurrentModifier.Attribute.IsEmpty())
			{
				CurrentDef.Modifiers.Add(CurrentModifier);
				CurrentModifier = FManifestModifierDefinition();
			}
			// Save current GE - v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GE_")))
			{
				OutData.GameplayEffects.Add(CurrentDef);
			}
			CurrentDef = FManifestGameplayEffectDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInModifiers = false;
			bInGrantedTags = false;
			bInRemoveTags = false;
			bInExecutions = false;
			bInSetByCallerTags = false;
		}
		else if (bInItem)
		{
			// Check for subsection headers
			if (TrimmedLine.Equals(TEXT("modifiers:")) || TrimmedLine.StartsWith(TEXT("modifiers:")))
			{
				bInModifiers = true;
				bInGrantedTags = false;
				bInRemoveTags = false;
				bInExecutions = false;
				bInSetByCallerTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("granted_tags:")) || TrimmedLine.StartsWith(TEXT("granted_tags:")))
			{
				bInModifiers = false;
				bInGrantedTags = true;
				bInRemoveTags = false;
				bInExecutions = false;
				bInSetByCallerTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("remove_gameplay_effects_with_tags:")) || TrimmedLine.StartsWith(TEXT("remove_gameplay_effects_with_tags:")))
			{
				bInModifiers = false;
				bInGrantedTags = false;
				bInRemoveTags = true;
				bInExecutions = false;
				bInSetByCallerTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("executions:")) || TrimmedLine.StartsWith(TEXT("executions:")))
			{
				bInModifiers = false;
				bInGrantedTags = false;
				bInRemoveTags = false;
				bInExecutions = true;
				bInSetByCallerTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("setbycaller_tags:")) || TrimmedLine.StartsWith(TEXT("setbycaller_tags:")))
			{
				bInModifiers = false;
				bInGrantedTags = false;
				bInRemoveTags = false;
				bInExecutions = false;
				bInSetByCallerTags = true;
			}
			// Parse simple fields
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("duration_policy:")))
			{
				CurrentDef.DurationPolicy = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("duration_magnitude:")))
			{
				CurrentDef.DurationMagnitude = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("period:")))
			{
				CurrentDef.Period = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("execute_periodic_on_application:")))
			{
				FString Val = GetLineValue(TrimmedLine);
				CurrentDef.bExecutePeriodicOnApplication = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
			}
			else if (TrimmedLine.StartsWith(TEXT("stack_limit_count:")))
			{
				CurrentDef.StackLimitCount = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("stacking_type:")))
			{
				CurrentDef.StackingType = GetLineValue(TrimmedLine);
			}
			// Parse modifier items
			else if (bInModifiers)
			{
				if (TrimmedLine.StartsWith(TEXT("- attribute:")))
				{
					// Save previous modifier
					if (!CurrentModifier.Attribute.IsEmpty())
					{
						CurrentDef.Modifiers.Add(CurrentModifier);
					}
					CurrentModifier = FManifestModifierDefinition();
					CurrentModifier.Attribute = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("modifier_op:")) || TrimmedLine.StartsWith(TEXT("op:")) || TrimmedLine.StartsWith(TEXT("operation:")))
				{
					CurrentModifier.Operation = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("magnitude_type:")))
				{
					// v2.4.0: Parse magnitude type (ScalableFloat or SetByCaller)
					CurrentModifier.MagnitudeType = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("magnitude_value:")) || TrimmedLine.StartsWith(TEXT("magnitude:")))
				{
					// v2.4.0: Support both magnitude_value: and magnitude: for backward compatibility
					CurrentModifier.ScalableFloatValue = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("setbycaller_tag:")))
				{
					// v2.4.0: Parse SetByCaller tag for this modifier
					CurrentModifier.SetByCallerTag = GetLineValue(TrimmedLine);
				}
			}
			// Parse granted tags array items
			else if (bInGrantedTags)
			{
				if (TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString TagName = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!TagName.IsEmpty())
					{
						CurrentDef.GrantedTags.Add(TagName);
					}
				}
			}
			// Parse remove tags array items
			else if (bInRemoveTags)
			{
				if (TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString TagName = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!TagName.IsEmpty())
					{
						CurrentDef.RemoveGameplayEffectsWithTags.Add(TagName);
					}
				}
			}
			// Parse execution classes
			else if (bInExecutions)
			{
				if (TrimmedLine.StartsWith(TEXT("- class:")))
				{
					FString ClassName = GetLineValue(TrimmedLine.Mid(2));
					if (!ClassName.IsEmpty())
					{
						CurrentDef.ExecutionClasses.Add(ClassName);
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("setbycaller_tag:")))
				{
					FString TagName = GetLineValue(TrimmedLine);
					if (!TagName.IsEmpty())
					{
						CurrentDef.SetByCallerTags.Add(TagName);
					}
				}
			}
			// Parse setbycaller tags array
			else if (bInSetByCallerTags)
			{
				if (TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString TagName = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!TagName.IsEmpty())
					{
						CurrentDef.SetByCallerTags.Add(TagName);
					}
				}
			}
		}

		LineIndex++;
	}

	// Save pending modifier
	if (bInModifiers && !CurrentModifier.Attribute.IsEmpty())
	{
		CurrentDef.Modifiers.Add(CurrentModifier);
	}
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GE_")))
	{
		OutData.GameplayEffects.Add(CurrentDef);
	}
}

// v2.3.0: Updated to parse tags subsection with all tag arrays
// v2.4.0: Updated to support inline event_graph and variables subsections
void FGasAbilityGeneratorParser::ParseGameplayAbilities(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestGameplayAbilityDefinition CurrentDef;
	bool bInItem = false;
	bool bInTags = false;
	bool bInVariables = false;
	bool bInEventGraph = false;  // v2.7.6: For inline event graph parsing
	bool bInCueTriggers = false;  // v4.13: Category C - P3.2
	bool bInVFXSpawns = false;    // v4.13: Category C - P3.1
	bool bInDelegateBindings = false;  // v4.13: Category C - P2.1
	int32 ItemIndent = -1;
	FManifestActorVariableDefinition CurrentVar;
	FManifestCueTriggerDefinition CurrentCueTrigger;   // v4.13: Category C
	FManifestVFXSpawnDefinition CurrentVFXSpawn;       // v4.13: Category C
	FManifestDelegateBindingDefinition CurrentDelegateBinding;  // v4.13: P2.1

	// Track which tag array we're currently parsing
	enum class ECurrentTagArray { None, AbilityTags, CancelAbilitiesWithTag, ActivationOwnedTags, ActivationRequiredTags, ActivationBlockedTags };
	ECurrentTagArray CurrentTagArray = ECurrentTagArray::None;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending variable
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
			}
			// v4.13: Save pending cue trigger
			if (bInCueTriggers && !CurrentCueTrigger.CueTag.IsEmpty())
			{
				CurrentDef.CueTriggers.Add(CurrentCueTrigger);
			}
			// v4.13: Save pending VFX spawn
			if (bInVFXSpawns && !CurrentVFXSpawn.NiagaraSystem.IsEmpty())
			{
				CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
			}
			// v4.13: Save pending delegate binding
			if (bInDelegateBindings && !CurrentDelegateBinding.Delegate.IsEmpty())
			{
				CurrentDef.DelegateBindings.Add(CurrentDelegateBinding);
			}
			// v2.6.14: Prefix validation - only add if GA_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GA_")))
			{
				OutData.GameplayAbilities.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// v2.4.0: Check if this is a new ability item by comparing indent level
		bool bIsNewAbilityItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewAbilityItem = true;
		}

		if (bIsNewAbilityItem)
		{
			// Save pending variable before switching to new ability
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
				CurrentVar = FManifestActorVariableDefinition();
			}
			// v4.13: Save pending cue trigger
			if (bInCueTriggers && !CurrentCueTrigger.CueTag.IsEmpty())
			{
				CurrentDef.CueTriggers.Add(CurrentCueTrigger);
				CurrentCueTrigger = FManifestCueTriggerDefinition();
			}
			// v4.13: Save pending VFX spawn
			if (bInVFXSpawns && !CurrentVFXSpawn.NiagaraSystem.IsEmpty())
			{
				CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
				CurrentVFXSpawn = FManifestVFXSpawnDefinition();
			}
			// v4.13: Save pending delegate binding
			if (bInDelegateBindings && !CurrentDelegateBinding.Delegate.IsEmpty())
			{
				CurrentDef.DelegateBindings.Add(CurrentDelegateBinding);
				CurrentDelegateBinding = FManifestDelegateBindingDefinition();
			}
			bInVariables = false;
			bInTags = false;
			bInEventGraph = false;
			bInCueTriggers = false;
			bInVFXSpawns = false;
			bInDelegateBindings = false;
			CurrentTagArray = ECurrentTagArray::None;

			// Save current ability and start new one - v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GA_")))
			{
				OutData.GameplayAbilities.Add(CurrentDef);
			}
			CurrentDef = FManifestGameplayAbilityDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			// First ability item - establish the item indent level
			ItemIndent = CurrentIndent;
			CurrentDef = FManifestGameplayAbilityDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInTags = false;
			bInVariables = false;
			bInEventGraph = false;
			bInCueTriggers = false;
			bInVFXSpawns = false;
			bInDelegateBindings = false;
			CurrentTagArray = ECurrentTagArray::None;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.StartsWith(TEXT("instancing_policy:")))
			{
				CurrentDef.InstancingPolicy = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.StartsWith(TEXT("net_execution_policy:")))
			{
				CurrentDef.NetExecutionPolicy = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.StartsWith(TEXT("cooldown_gameplay_effect_class:")))
			{
				CurrentDef.CooldownGameplayEffectClass = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			// v4.8: Parse NarrativeGameplayAbility properties
			else if (TrimmedLine.StartsWith(TEXT("input_tag:")))
			{
				CurrentDef.InputTag = GetLineValue(TrimmedLine);
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.StartsWith(TEXT("activate_on_granted:")))
			{
				CurrentDef.bActivateAbilityOnGranted = GetLineValue(TrimmedLine).ToBool();
				bInTags = false;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (TrimmedLine.Equals(TEXT("tags:")) || TrimmedLine.StartsWith(TEXT("tags:")))
			{
				// Save pending variable before switching sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				bInTags = true;
				bInVariables = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
				UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Entered tags section"), *CurrentDef.Name);
			}
			// v2.4.0: Parse variables subsection
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				bInVariables = true;
				bInTags = false;
				bInEventGraph = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			// v2.4.0: Parse inline event_graph subsection
			else if (TrimmedLine.Equals(TEXT("event_graph:")) || TrimmedLine.StartsWith(TEXT("event_graph:")))
			{
				// Save pending variable before switching sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				bInEventGraph = true;
				bInVariables = false;
				bInTags = false;
				CurrentTagArray = ECurrentTagArray::None;
				CurrentDef.bHasInlineEventGraph = true;
				CurrentDef.EventGraphName = CurrentDef.Name + TEXT("_EventGraph");
			}
			else if (bInEventGraph)
			{
				// Parse nodes and connections subsections within event_graph
				// Use a temporary graph definition for parsing, then copy to arrays
				if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
				{
					int32 NodesIndent = CurrentIndent;
					LineIndex++;
					FManifestEventGraphDefinition TempGraph;
					ParseGraphNodes(Lines, LineIndex, NodesIndent, TempGraph);
					CurrentDef.EventGraphNodes = MoveTemp(TempGraph.Nodes);
					continue; // ParseGraphNodes already advanced LineIndex
				}
				else if (TrimmedLine.Equals(TEXT("connections:")) || TrimmedLine.StartsWith(TEXT("connections:")))
				{
					int32 ConnectionsIndent = CurrentIndent;
					LineIndex++;
					FManifestEventGraphDefinition TempGraph;
					ParseGraphConnections(Lines, LineIndex, ConnectionsIndent, TempGraph);
					CurrentDef.EventGraphConnections = MoveTemp(TempGraph.Connections);
					continue; // ParseGraphConnections already advanced LineIndex
				}
			}
			// v4.13: Category C - P3.2 cue_triggers section
			else if (TrimmedLine.Equals(TEXT("cue_triggers:")) || TrimmedLine.StartsWith(TEXT("cue_triggers:")))
			{
				// Save pending data from other sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				if (bInVFXSpawns && !CurrentVFXSpawn.NiagaraSystem.IsEmpty())
				{
					CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
					CurrentVFXSpawn = FManifestVFXSpawnDefinition();
				}
				bInCueTriggers = true;
				bInVariables = false;
				bInTags = false;
				bInEventGraph = false;
				bInVFXSpawns = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			// v4.13: Category C - P3.1 vfx_spawns section
			else if (TrimmedLine.Equals(TEXT("vfx_spawns:")) || TrimmedLine.StartsWith(TEXT("vfx_spawns:")))
			{
				// Save pending data from other sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				if (bInCueTriggers && !CurrentCueTrigger.CueTag.IsEmpty())
				{
					CurrentDef.CueTriggers.Add(CurrentCueTrigger);
					CurrentCueTrigger = FManifestCueTriggerDefinition();
				}
				bInVFXSpawns = true;
				bInVariables = false;
				bInTags = false;
				bInEventGraph = false;
				bInCueTriggers = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (bInCueTriggers)
			{
				// Parse cue trigger items
				if (TrimmedLine.StartsWith(TEXT("- trigger:")) || TrimmedLine.StartsWith(TEXT("- cue_tag:")))
				{
					// Save previous trigger
					if (!CurrentCueTrigger.CueTag.IsEmpty())
					{
						CurrentDef.CueTriggers.Add(CurrentCueTrigger);
					}
					CurrentCueTrigger = FManifestCueTriggerDefinition();
					if (TrimmedLine.StartsWith(TEXT("- trigger:")))
					{
						CurrentCueTrigger.Trigger = GetLineValue(TrimmedLine.Mid(2));
					}
					else
					{
						CurrentCueTrigger.CueTag = GetLineValue(TrimmedLine.Mid(2));
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("trigger:")))
				{
					CurrentCueTrigger.Trigger = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("cue_tag:")))
				{
					CurrentCueTrigger.CueTag = GetLineValue(TrimmedLine);
				}
			}
			else if (bInVFXSpawns)
			{
				// Parse VFX spawn items
				if (TrimmedLine.StartsWith(TEXT("- id:")) || TrimmedLine.StartsWith(TEXT("- niagara_system:")))
				{
					// Save previous spawn
					if (!CurrentVFXSpawn.NiagaraSystem.IsEmpty())
					{
						CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
					}
					CurrentVFXSpawn = FManifestVFXSpawnDefinition();
					if (TrimmedLine.StartsWith(TEXT("- id:")))
					{
						CurrentVFXSpawn.Id = GetLineValue(TrimmedLine.Mid(2));
					}
					else
					{
						CurrentVFXSpawn.NiagaraSystem = GetLineValue(TrimmedLine.Mid(2));
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("id:")))
				{
					CurrentVFXSpawn.Id = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("niagara_system:")))
				{
					CurrentVFXSpawn.NiagaraSystem = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_to:")))
				{
					CurrentVFXSpawn.AttachTo = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("socket:")))
				{
					CurrentVFXSpawn.Socket = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("destroy_on_end_ability:")))
				{
					CurrentVFXSpawn.bDestroyOnEndAbility = GetLineValue(TrimmedLine).ToBool();
				}
			}
			// v4.13: Category C - P2.1 delegate_bindings section
			else if (TrimmedLine.Equals(TEXT("delegate_bindings:")) || TrimmedLine.StartsWith(TEXT("delegate_bindings:")))
			{
				// Save pending data from other sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				if (bInCueTriggers && !CurrentCueTrigger.CueTag.IsEmpty())
				{
					CurrentDef.CueTriggers.Add(CurrentCueTrigger);
					CurrentCueTrigger = FManifestCueTriggerDefinition();
				}
				if (bInVFXSpawns && !CurrentVFXSpawn.NiagaraSystem.IsEmpty())
				{
					CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
					CurrentVFXSpawn = FManifestVFXSpawnDefinition();
				}
				bInDelegateBindings = true;
				bInVariables = false;
				bInTags = false;
				bInEventGraph = false;
				bInCueTriggers = false;
				bInVFXSpawns = false;
				CurrentTagArray = ECurrentTagArray::None;
			}
			else if (bInDelegateBindings)
			{
				// Parse delegate binding items
				if (TrimmedLine.StartsWith(TEXT("- delegate:")) || TrimmedLine.StartsWith(TEXT("- handler:")))
				{
					// Save previous binding
					if (!CurrentDelegateBinding.Delegate.IsEmpty())
					{
						CurrentDef.DelegateBindings.Add(CurrentDelegateBinding);
					}
					CurrentDelegateBinding = FManifestDelegateBindingDefinition();
					if (TrimmedLine.StartsWith(TEXT("- delegate:")))
					{
						CurrentDelegateBinding.Delegate = GetLineValue(TrimmedLine.Mid(2));
					}
					else
					{
						CurrentDelegateBinding.Handler = GetLineValue(TrimmedLine.Mid(2));
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("delegate:")))
				{
					CurrentDelegateBinding.Delegate = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("source:")))
				{
					CurrentDelegateBinding.Source = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attribute:")))
				{
					CurrentDelegateBinding.Attribute = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("handler:")))
				{
					CurrentDelegateBinding.Handler = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("unbind_on_end:")))
				{
					CurrentDelegateBinding.bUnbindOnEnd = GetLineValue(TrimmedLine).ToBool();
				}
			}
			else if (bInVariables)
			{
				// Parse variable items
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
					}
					CurrentVar = FManifestActorVariableDefinition();
					CurrentVar.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentVar.Type = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("class:")))
				{
					// For Object/Class types, store class in Type field with prefix
					FString ClassValue = GetLineValue(TrimmedLine);
					if (!CurrentVar.Type.IsEmpty())
					{
						CurrentVar.Type = CurrentVar.Type + TEXT(":") + ClassValue;
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("default_value:")) || TrimmedLine.StartsWith(TEXT("default:")))
				{
					CurrentVar.DefaultValue = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("instance_editable:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bInstanceEditable = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
				else if (TrimmedLine.StartsWith(TEXT("replicated:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bReplicated = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
			}
			else if (bInTags)
			{
				// Check for tag array headers
				if (TrimmedLine.Equals(TEXT("ability_tags:")) || TrimmedLine.StartsWith(TEXT("ability_tags:")))
				{
					CurrentTagArray = ECurrentTagArray::AbilityTags;
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Set CurrentTagArray to AbilityTags"), *CurrentDef.Name);
				}
				else if (TrimmedLine.Equals(TEXT("cancel_abilities_with_tag:")) || TrimmedLine.StartsWith(TEXT("cancel_abilities_with_tag:")))
				{
					CurrentTagArray = ECurrentTagArray::CancelAbilitiesWithTag;
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Set CurrentTagArray to CancelAbilitiesWithTag"), *CurrentDef.Name);
				}
				else if (TrimmedLine.Equals(TEXT("activation_owned_tags:")) || TrimmedLine.StartsWith(TEXT("activation_owned_tags:")))
				{
					CurrentTagArray = ECurrentTagArray::ActivationOwnedTags;
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Set CurrentTagArray to ActivationOwnedTags"), *CurrentDef.Name);
				}
				else if (TrimmedLine.Equals(TEXT("activation_required_tags:")) || TrimmedLine.StartsWith(TEXT("activation_required_tags:")))
				{
					CurrentTagArray = ECurrentTagArray::ActivationRequiredTags;
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Set CurrentTagArray to ActivationRequiredTags"), *CurrentDef.Name);
				}
				else if (TrimmedLine.Equals(TEXT("activation_blocked_tags:")) || TrimmedLine.StartsWith(TEXT("activation_blocked_tags:")))
				{
					CurrentTagArray = ECurrentTagArray::ActivationBlockedTags;
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Set CurrentTagArray to ActivationBlockedTags"), *CurrentDef.Name);
				}
				else if (TrimmedLine.StartsWith(TEXT("-")) && CurrentTagArray != ECurrentTagArray::None)
				{
					// Parse tag value
					// v3.9.10: Use StripYamlComment to remove inline comments
					FString TagValue = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
					// Remove quotes if present
					if (TagValue.Len() >= 2)
					{
						if ((TagValue.StartsWith(TEXT("\"")) && TagValue.EndsWith(TEXT("\""))) ||
							(TagValue.StartsWith(TEXT("'")) && TagValue.EndsWith(TEXT("'"))))
						{
							TagValue = TagValue.Mid(1, TagValue.Len() - 2);
						}
					}

					if (!TagValue.IsEmpty())
					{
						UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: Adding tag '%s' to array %d"), *CurrentDef.Name, *TagValue, (int)CurrentTagArray);
						switch (CurrentTagArray)
						{
						case ECurrentTagArray::AbilityTags:
							CurrentDef.Tags.AbilityTags.Add(TagValue);
							break;
						case ECurrentTagArray::CancelAbilitiesWithTag:
							CurrentDef.Tags.CancelAbilitiesWithTag.Add(TagValue);
							break;
						case ECurrentTagArray::ActivationOwnedTags:
							CurrentDef.Tags.ActivationOwnedTags.Add(TagValue);
							break;
						case ECurrentTagArray::ActivationRequiredTags:
							CurrentDef.Tags.ActivationRequiredTags.Add(TagValue);
							break;
						case ECurrentTagArray::ActivationBlockedTags:
							CurrentDef.Tags.ActivationBlockedTags.Add(TagValue);
							break;
						default:
							break;
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Parser] GA %s: bInTags=true but line not matched: '%s' (CurrentTagArray=%d)"), *CurrentDef.Name, *TrimmedLine, (int)CurrentTagArray);
				}
			}
		}

		LineIndex++;
	}

	// Save any pending data
	if (bInVariables && !CurrentVar.Name.IsEmpty())
	{
		CurrentDef.Variables.Add(CurrentVar);
	}
	// v4.13: Save pending cue trigger, VFX spawn, and delegate binding
	if (bInCueTriggers && !CurrentCueTrigger.CueTag.IsEmpty())
	{
		CurrentDef.CueTriggers.Add(CurrentCueTrigger);
	}
	if (bInVFXSpawns && !CurrentVFXSpawn.NiagaraSystem.IsEmpty())
	{
		CurrentDef.VFXSpawns.Add(CurrentVFXSpawn);
	}
	if (bInDelegateBindings && !CurrentDelegateBinding.Delegate.IsEmpty())
	{
		CurrentDef.DelegateBindings.Add(CurrentDelegateBinding);
	}
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("GA_")))
	{
		OutData.GameplayAbilities.Add(CurrentDef);
	}
}

// v2.1.7 FIX: Proper indent-based subsection exit logic
void FGasAbilityGeneratorParser::ParseActorBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;
	
	FManifestActorBlueprintDefinition CurrentDef;
	bool bInItem = false;
	bool bInVariables = false;
	bool bInEventGraph = false;  // v2.7.6: For inline event graph parsing
	bool bInComponents = false;
	int32 ItemIndent = -1;      // v2.1.7: Track blueprint item indent level
	int32 VariablesIndent = 0;
	int32 ComponentsIndent = 0;
	FManifestActorVariableDefinition CurrentVar;
	FManifestActorComponentDefinition CurrentComp;
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);
		
		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
			}
			if (bInComponents && !CurrentComp.Name.IsEmpty())
			{
				CurrentDef.Components.Add(CurrentComp);
			}
			// v2.6.14: Prefix validation - valid prefixes: BP_, BTS_, BPA_, Goal_, GoalGenerator_
			if (bInItem && !CurrentDef.Name.IsEmpty() &&
				(CurrentDef.Name.StartsWith(TEXT("BP_")) || CurrentDef.Name.StartsWith(TEXT("BTS_")) ||
				 CurrentDef.Name.StartsWith(TEXT("BPA_")) || CurrentDef.Name.StartsWith(TEXT("Goal_")) ||
				 CurrentDef.Name.StartsWith(TEXT("GoalGenerator_"))))
			{
				OutData.ActorBlueprints.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}
		
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}
		
		// v2.1.7 FIX: Check if this is a new blueprint item by comparing indent level
		// Blueprint items are at ItemIndent (e.g., indent 2)
		// Variable items are at higher indent (e.g., indent 6)
		bool bIsNewBlueprintItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewBlueprintItem = true;
		}
		
		// If we detect a new blueprint item, close current subsections
		if (bIsNewBlueprintItem)
		{
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
				CurrentVar = FManifestActorVariableDefinition();
			}
			if (bInComponents && !CurrentComp.Name.IsEmpty())
			{
				CurrentDef.Components.Add(CurrentComp);
				CurrentComp = FManifestActorComponentDefinition();
			}
			bInVariables = false;
			bInComponents = false;

			// Save current blueprint and start new one - v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() &&
				(CurrentDef.Name.StartsWith(TEXT("BP_")) || CurrentDef.Name.StartsWith(TEXT("BTS_")) ||
				 CurrentDef.Name.StartsWith(TEXT("BPA_")) || CurrentDef.Name.StartsWith(TEXT("Goal_")) ||
				 CurrentDef.Name.StartsWith(TEXT("GoalGenerator_"))))
			{
				OutData.ActorBlueprints.Add(CurrentDef);
			}
			CurrentDef = FManifestActorBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			// ItemIndent already set from first item
		}
		// First blueprint item - establish the item indent level
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			ItemIndent = CurrentIndent;
			CurrentDef = FManifestActorBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			// v2.7.6: Parse inline event_graph subsection (like GA abilities)
			else if (TrimmedLine.Equals(TEXT("event_graph:")) || TrimmedLine.StartsWith(TEXT("event_graph:")))
			{
				// Save pending variable/component before switching sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				if (bInComponents && !CurrentComp.Name.IsEmpty())
				{
					CurrentDef.Components.Add(CurrentComp);
					CurrentComp = FManifestActorComponentDefinition();
				}
				bInEventGraph = true;
				bInVariables = false;
				bInComponents = false;
				CurrentDef.bHasInlineEventGraph = true;
				CurrentDef.EventGraphName = CurrentDef.Name + TEXT("_EventGraph");
			}
			else if (bInEventGraph)
			{
				// Parse nodes and connections subsections within event_graph
				if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
				{
					int32 NodesIndent = CurrentIndent;
					LineIndex++;
					FManifestEventGraphDefinition TempGraph;
					ParseGraphNodes(Lines, LineIndex, NodesIndent, TempGraph);
					CurrentDef.EventGraphNodes = MoveTemp(TempGraph.Nodes);
					continue;
				}
				else if (TrimmedLine.Equals(TEXT("connections:")) || TrimmedLine.StartsWith(TEXT("connections:")))
				{
					int32 ConnectionsIndent = CurrentIndent;
					LineIndex++;
					FManifestEventGraphDefinition TempGraph;
					ParseGraphConnections(Lines, LineIndex, ConnectionsIndent, TempGraph);
					CurrentDef.EventGraphConnections = MoveTemp(TempGraph.Connections);
					continue;
				}
				// v2.8.3: If we see another section header while in event_graph, exit event_graph mode
				// This handles function_overrides, variables, components etc. following event_graph
				else if (TrimmedLine.Contains(TEXT(":")) && !TrimmedLine.StartsWith(TEXT("-")) && !TrimmedLine.StartsWith(TEXT("#")))
				{
					bInEventGraph = false;
					// Don't continue - fall through to process this section header below
				}
			}
			// v2.8.3: Parse function_overrides section
			if (!bInEventGraph && (TrimmedLine.Equals(TEXT("function_overrides:")) || TrimmedLine.StartsWith(TEXT("function_overrides:"))))
			{
				// Save pending variable/component before switching sections
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				if (bInComponents && !CurrentComp.Name.IsEmpty())
				{
					CurrentDef.Components.Add(CurrentComp);
					CurrentComp = FManifestActorComponentDefinition();
				}
				bInEventGraph = false;
				bInVariables = false;
				bInComponents = false;

				// Parse function overrides subsection
				int32 FuncOverridesIndent = CurrentIndent;
				LineIndex++;
				ParseFunctionOverrides(Lines, LineIndex, FuncOverridesIndent, CurrentDef.FunctionOverrides);
				continue;
			}
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				bInEventGraph = false;
				bInVariables = true;
				bInComponents = false;
				VariablesIndent = CurrentIndent;
			}
			else if (TrimmedLine.Equals(TEXT("components:")) || TrimmedLine.StartsWith(TEXT("components:")))
			{
				bInEventGraph = false;
				// Save pending variable before switching to components
				if (bInVariables && !CurrentVar.Name.IsEmpty())
				{
					CurrentDef.Variables.Add(CurrentVar);
					CurrentVar = FManifestActorVariableDefinition();
				}
				bInComponents = true;
				bInVariables = false;
				ComponentsIndent = CurrentIndent;
			}
			else if (bInVariables)
			{
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
					}
					CurrentVar = FManifestActorVariableDefinition();
					CurrentVar.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentVar.Type = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("default:")))
				{
					CurrentVar.DefaultValue = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("replicated:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bReplicated = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
				else if (TrimmedLine.StartsWith(TEXT("instance_editable:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bInstanceEditable = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
			}
			else if (bInComponents)
			{
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					if (!CurrentComp.Name.IsEmpty())
					{
						CurrentDef.Components.Add(CurrentComp);
					}
					CurrentComp = FManifestActorComponentDefinition();
					CurrentComp.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentComp.Type = GetLineValue(TrimmedLine);
				}
			}
		}
		
		LineIndex++;
	}
	
	if (bInVariables && !CurrentVar.Name.IsEmpty())
	{
		CurrentDef.Variables.Add(CurrentVar);
	}
	if (bInComponents && !CurrentComp.Name.IsEmpty())
	{
		CurrentDef.Components.Add(CurrentComp);
	}
	// v2.6.14: Prefix validation - valid prefixes: BP_, BTS_, BPA_, Goal_, GoalGenerator_
	if (bInItem && !CurrentDef.Name.IsEmpty() &&
		(CurrentDef.Name.StartsWith(TEXT("BP_")) || CurrentDef.Name.StartsWith(TEXT("BTS_")) ||
		 CurrentDef.Name.StartsWith(TEXT("BPA_")) || CurrentDef.Name.StartsWith(TEXT("Goal_")) ||
		 CurrentDef.Name.StartsWith(TEXT("GoalGenerator_"))))
	{
		OutData.ActorBlueprints.Add(CurrentDef);
	}
}

// v2.1.7 FIX: Proper indent-based subsection exit logic
// v4.3: Added widget_tree parsing for full visual layout automation
void FGasAbilityGeneratorParser::ParseWidgetBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestWidgetBlueprintDefinition CurrentDef;
	bool bInItem = false;
	bool bInVariables = false;
	bool bInEventGraph = false;  // v2.7.6: For inline event graph parsing
	int32 ItemIndent = -1;      // v2.1.7: Track widget item indent level
	int32 VariablesIndent = 0;
	FManifestWidgetVariableDefinition CurrentVar;

	// v4.3: Widget tree parsing state
	bool bInWidgetTree = false;
	bool bInWidgets = false;
	bool bInWidgetDef = false;
	bool bInSlot = false;
	bool bInChildren = false;
	bool bInProperties = false;
	FManifestWidgetNodeDefinition CurrentWidget;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v4.3: Save pending widget
			if (bInWidgetDef && !CurrentWidget.Id.IsEmpty())
			{
				CurrentDef.WidgetTree.Widgets.Add(CurrentWidget);
			}
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
			}
			// v2.6.14: Prefix validation - only add if WBP_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("WBP_")))
			{
				OutData.WidgetBlueprints.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// v2.1.7 FIX: Check if this is a new widget item by comparing indent level
		bool bIsNewWidgetItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewWidgetItem = true;
		}

		// If we detect a new widget item, close current subsections
		if (bIsNewWidgetItem)
		{
			// v4.3: Save pending widget
			if (bInWidgetDef && !CurrentWidget.Id.IsEmpty())
			{
				CurrentDef.WidgetTree.Widgets.Add(CurrentWidget);
				CurrentWidget = FManifestWidgetNodeDefinition();
			}
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
				CurrentVar = FManifestWidgetVariableDefinition();
			}
			bInVariables = false;
			bInWidgetTree = false;
			bInWidgets = false;
			bInWidgetDef = false;
			bInSlot = false;
			bInChildren = false;
			bInProperties = false;

			// Save current widget and start new one - v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("WBP_")))
			{
				OutData.WidgetBlueprints.Add(CurrentDef);
			}
			CurrentDef = FManifestWidgetBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		// First widget item - establish the item indent level
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			ItemIndent = CurrentIndent;
			CurrentDef = FManifestWidgetBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
				bInVariables = false;
				bInWidgetTree = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInVariables = false;
				bInWidgetTree = false;
			}
			// v2.2.0: Parse event_graph reference
			else if (TrimmedLine.StartsWith(TEXT("event_graph:")))
			{
				CurrentDef.EventGraphName = GetLineValue(TrimmedLine);
				bInVariables = false;
				bInWidgetTree = false;
			}
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				bInVariables = true;
				bInWidgetTree = false;
				VariablesIndent = CurrentIndent;
			}
			// v4.3: Widget tree section
			else if (TrimmedLine.Equals(TEXT("widget_tree:")) || TrimmedLine.StartsWith(TEXT("widget_tree:")))
			{
				bInWidgetTree = true;
				bInVariables = false;
				bInWidgets = false;
			}
			// v4.3: Widget tree properties
			else if (bInWidgetTree && !bInWidgets)
			{
				if (TrimmedLine.StartsWith(TEXT("root:")))
				{
					CurrentDef.WidgetTree.RootWidget = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.Equals(TEXT("widgets:")) || TrimmedLine.StartsWith(TEXT("widgets:")))
				{
					bInWidgets = true;
					bInWidgetDef = false;
				}
			}
			// v4.3: Widget definitions
			else if (bInWidgets)
			{
				// New widget definition
				if (TrimmedLine.StartsWith(TEXT("- id:")))
				{
					// Save previous widget
					if (bInWidgetDef && !CurrentWidget.Id.IsEmpty())
					{
						CurrentDef.WidgetTree.Widgets.Add(CurrentWidget);
					}
					CurrentWidget = FManifestWidgetNodeDefinition();
					CurrentWidget.Id = GetLineValue(TrimmedLine.Mid(2));
					bInWidgetDef = true;
					bInSlot = false;
					bInChildren = false;
					bInProperties = false;
				}
				else if (bInWidgetDef)
				{
					// Widget properties
					if (TrimmedLine.StartsWith(TEXT("type:")))
					{
						CurrentWidget.Type = GetLineValue(TrimmedLine);
						bInSlot = false;
						bInChildren = false;
						bInProperties = false;
					}
					else if (TrimmedLine.StartsWith(TEXT("name:")))
					{
						CurrentWidget.Name = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("text:")))
					{
						CurrentWidget.Text = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("image_path:")) || TrimmedLine.StartsWith(TEXT("image:")))
					{
						CurrentWidget.ImagePath = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("style_class:")) || TrimmedLine.StartsWith(TEXT("style:")))
					{
						CurrentWidget.StyleClass = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("is_variable:")))
					{
						FString Val = GetLineValue(TrimmedLine);
						CurrentWidget.bIsVariable = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
					}
					// Slot section
					else if (TrimmedLine.Equals(TEXT("slot:")) || TrimmedLine.StartsWith(TEXT("slot:")))
					{
						bInSlot = true;
						bInChildren = false;
						bInProperties = false;
					}
					// Children section
					else if (TrimmedLine.Equals(TEXT("children:")) || TrimmedLine.StartsWith(TEXT("children:")))
					{
						bInChildren = true;
						bInSlot = false;
						bInProperties = false;
						// Check for inline array
						FString InlineValue = GetLineValue(TrimmedLine);
						if (!InlineValue.IsEmpty() && InlineValue.StartsWith(TEXT("[")))
						{
							// Parse inline array [id1, id2, id3]
							FString ArrayContent = InlineValue.Mid(1);
							ArrayContent = ArrayContent.LeftChop(1); // Remove ]
							TArray<FString> Parts;
							ArrayContent.ParseIntoArray(Parts, TEXT(","));
							for (const FString& Part : Parts)
							{
								FString CleanPart = Part.TrimStartAndEnd();
								if (!CleanPart.IsEmpty())
								{
									CurrentWidget.Children.Add(CleanPart);
								}
							}
							bInChildren = false;
						}
					}
					// Properties section
					else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
					{
						bInProperties = true;
						bInSlot = false;
						bInChildren = false;
					}
					// Slot properties
					else if (bInSlot)
					{
						if (TrimmedLine.StartsWith(TEXT("anchors:")))
						{
							CurrentWidget.Slot.Anchors = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("position:")))
						{
							CurrentWidget.Slot.Position = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("size:")))
						{
							CurrentWidget.Slot.Size = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("alignment:")))
						{
							CurrentWidget.Slot.Alignment = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("auto_size:")))
						{
							FString Val = GetLineValue(TrimmedLine);
							CurrentWidget.Slot.bAutoSize = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
						}
						else if (TrimmedLine.StartsWith(TEXT("horizontal_alignment:")) || TrimmedLine.StartsWith(TEXT("h_align:")))
						{
							CurrentWidget.Slot.HorizontalAlignment = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("vertical_alignment:")) || TrimmedLine.StartsWith(TEXT("v_align:")))
						{
							CurrentWidget.Slot.VerticalAlignment = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("size_rule:")))
						{
							CurrentWidget.Slot.SizeRule = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("fill_weight:")))
						{
							CurrentWidget.Slot.FillWeight = FCString::Atof(*GetLineValue(TrimmedLine));
						}
						else if (TrimmedLine.StartsWith(TEXT("padding:")))
						{
							CurrentWidget.Slot.Padding = GetLineValue(TrimmedLine);
						}
					}
					// Children list items
					else if (bInChildren && TrimmedLine.StartsWith(TEXT("-")))
					{
						FString ChildId = TrimmedLine.Mid(1).TrimStartAndEnd();
						if (!ChildId.IsEmpty())
						{
							CurrentWidget.Children.Add(ChildId);
						}
					}
					// Custom properties
					else if (bInProperties)
					{
						int32 ColonPos;
						if (TrimmedLine.FindChar(':', ColonPos))
						{
							FString PropName = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
							FString PropValue = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
							if (!PropName.IsEmpty())
							{
								CurrentWidget.Properties.Add(PropName, PropValue);
							}
						}
					}
				}
			}
			else if (bInVariables)
			{
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
					}
					CurrentVar = FManifestWidgetVariableDefinition();
					CurrentVar.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentVar.Type = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("default:")))
				{
					CurrentVar.DefaultValue = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("instance_editable:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bInstanceEditable = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
				else if (TrimmedLine.StartsWith(TEXT("expose_on_spawn:")))
				{
					FString Val = GetLineValue(TrimmedLine);
					CurrentVar.bExposeOnSpawn = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
			}
		}

		LineIndex++;
	}

	// v4.3: Save pending widget
	if (bInWidgetDef && !CurrentWidget.Id.IsEmpty())
	{
		CurrentDef.WidgetTree.Widgets.Add(CurrentWidget);
	}
	if (bInVariables && !CurrentVar.Name.IsEmpty())
	{
		CurrentDef.Variables.Add(CurrentVar);
	}
	// v2.6.14: Prefix validation - only add if WBP_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("WBP_")))
	{
		OutData.WidgetBlueprints.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseBlackboards(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestBlackboardDefinition CurrentDef;
	bool bInItem = false;
	bool bInKeys = false;
	// v4.0: Track current key being parsed
	FManifestBlackboardKeyDefinition CurrentKey;
	bool bInKeyDef = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v4.0: Save any pending key
			if (bInKeyDef && !CurrentKey.Name.IsEmpty())
			{
				CurrentDef.Keys.Add(CurrentKey);
			}
			// v2.6.14: Prefix validation - only add if BB_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BB_")))
			{
				OutData.Blackboards.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v4.0: Save any pending key
			if (bInKeyDef && !CurrentKey.Name.IsEmpty())
			{
				CurrentDef.Keys.Add(CurrentKey);
			}
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BB_")))
			{
				OutData.Blackboards.Add(CurrentDef);
			}
			CurrentDef = FManifestBlackboardDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInKeys = false;
			bInKeyDef = false;
		}
		else if (bInItem)
		{
			// v4.0: Parent blackboard
			if (TrimmedLine.StartsWith(TEXT("parent:")))
			{
				CurrentDef.Parent = GetLineValue(TrimmedLine);
				bInKeys = false;
				bInKeyDef = false;
			}
			// Keys section start
			else if (TrimmedLine.Equals(TEXT("keys:")) || TrimmedLine.StartsWith(TEXT("keys:")))
			{
				bInKeys = true;
				bInKeyDef = false;
			}
			else if (bInKeys)
			{
				// New key starts with "- name:" or "- type:"
				if (TrimmedLine.StartsWith(TEXT("- name:")) || TrimmedLine.StartsWith(TEXT("- type:")))
				{
					// Save previous key if exists
					if (bInKeyDef && !CurrentKey.Name.IsEmpty())
					{
						CurrentDef.Keys.Add(CurrentKey);
					}
					CurrentKey = FManifestBlackboardKeyDefinition();
					bInKeyDef = true;

					if (TrimmedLine.StartsWith(TEXT("- name:")))
					{
						CurrentKey.Name = GetLineValue(TrimmedLine.Mid(2));
					}
					else if (TrimmedLine.StartsWith(TEXT("- type:")))
					{
						CurrentKey.Type = GetLineValue(TrimmedLine.Mid(2));
					}
				}
				else if (bInKeyDef)
				{
					// Parse key properties
					if (TrimmedLine.StartsWith(TEXT("name:")))
					{
						CurrentKey.Name = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("type:")))
					{
						CurrentKey.Type = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("instance_synced:")))
					{
						CurrentKey.bInstanceSynced = GetLineValue(TrimmedLine).ToBool();
					}
					// v4.0: Base class for Object/Class types
					else if (TrimmedLine.StartsWith(TEXT("base_class:")))
					{
						CurrentKey.BaseClass = GetLineValue(TrimmedLine);
					}
				}
			}
		}

		LineIndex++;
	}

	// v4.0: Save any pending key
	if (bInKeyDef && !CurrentKey.Name.IsEmpty())
	{
		CurrentDef.Keys.Add(CurrentKey);
	}
	// v2.6.14: Prefix validation - only add if BB_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BB_")))
	{
		OutData.Blackboards.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseBehaviorTrees(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestBehaviorTreeDefinition CurrentDef;
	bool bInItem = false;
	bool bInNodes = false;
	bool bInChildren = false;
	bool bInDecorators = false;
	bool bInServices = false;
	bool bInNodeProperties = false;  // v4.0: Node properties flag
	FManifestBTNodeDefinition CurrentNode;
	FManifestBTDecoratorDefinition CurrentDecorator;
	FManifestBTServiceDefinition CurrentService;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save current node if valid
			if (bInNodes && !CurrentNode.Id.IsEmpty())
			{
				CurrentDef.Nodes.Add(CurrentNode);
			}
			// v2.6.14: Prefix validation - only add if BT_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BT_")))
			{
				OutData.BehaviorTrees.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save current node if valid
			if (bInNodes && !CurrentNode.Id.IsEmpty())
			{
				CurrentDef.Nodes.Add(CurrentNode);
			}
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BT_")))
			{
				OutData.BehaviorTrees.Add(CurrentDef);
			}
			CurrentDef = FManifestBehaviorTreeDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInNodes = false;
			bInChildren = false;
			bInDecorators = false;
			bInServices = false;
		}
		else if (bInItem)
		{
			// v4.13.1: Support both blackboard_asset: and blackboard: aliases
			if (TrimmedLine.StartsWith(TEXT("blackboard_asset:")) || TrimmedLine.StartsWith(TEXT("blackboard:")))
			{
				CurrentDef.BlackboardAsset = GetLineValue(TrimmedLine);
				bInNodes = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInNodes = false;
			}
			// v3.1: root_type for composite (Selector or Sequence)
			else if (TrimmedLine.StartsWith(TEXT("root_type:")))
			{
				CurrentDef.RootType = GetLineValue(TrimmedLine);
				bInNodes = false;
			}
			// v4.13.1: description field (ignored but handled to prevent fall-through)
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				// Description is for documentation only, not stored
				bInNodes = false;
			}
			// v3.1: nodes array
			else if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
			{
				bInNodes = true;
				bInChildren = false;
				bInDecorators = false;
				bInServices = false;
			}
			else if (bInNodes)
			{
				// Start new node
				if (TrimmedLine.StartsWith(TEXT("- id:")))
				{
					// Save previous node
					if (!CurrentNode.Id.IsEmpty())
					{
						CurrentDef.Nodes.Add(CurrentNode);
					}
					CurrentNode = FManifestBTNodeDefinition();
					CurrentNode.Id = GetLineValue(TrimmedLine.Mid(2));
					bInChildren = false;
					bInDecorators = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentNode.Type = GetLineValue(TrimmedLine);
					bInChildren = false;
					bInDecorators = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.StartsWith(TEXT("task_class:")))
				{
					CurrentNode.TaskClass = GetLineValue(TrimmedLine);
					bInChildren = false;
					bInDecorators = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.StartsWith(TEXT("blackboard_key:")))
				{
					CurrentNode.BlackboardKey = GetLineValue(TrimmedLine);
					bInChildren = false;
					bInDecorators = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.Equals(TEXT("children:")) || TrimmedLine.StartsWith(TEXT("children:")))
				{
					bInChildren = true;
					bInDecorators = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.Equals(TEXT("decorators:")) || TrimmedLine.StartsWith(TEXT("decorators:")))
				{
					bInDecorators = true;
					bInChildren = false;
					bInServices = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.Equals(TEXT("services:")) || TrimmedLine.StartsWith(TEXT("services:")))
				{
					bInServices = true;
					bInChildren = false;
					bInDecorators = false;
					bInNodeProperties = false;
				}
				// v4.0: Node properties section
				else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
				{
					bInNodeProperties = true;
					bInChildren = false;
					bInDecorators = false;
					bInServices = false;
				}
				// v4.0: Parse node properties (key: value format)
				else if (bInNodeProperties && TrimmedLine.Contains(TEXT(":")))
				{
					FString Key, Value;
					if (TrimmedLine.Split(TEXT(":"), &Key, &Value))
					{
						Key = Key.TrimStartAndEnd();
						Value = Value.TrimStartAndEnd();
						// Remove quotes from value
						if (Value.Len() >= 2 && ((Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\""))) ||
							(Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))))
						{
							Value = Value.Mid(1, Value.Len() - 2);
						}
						if (!Key.IsEmpty())
						{
							CurrentNode.Properties.Add(Key, Value);
						}
					}
				}
				else if (bInChildren && TrimmedLine.StartsWith(TEXT("-")))
				{
					// v3.9.10: Use StripYamlComment to remove inline comments
					FString ChildId = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
					if (ChildId.Len() >= 2 && ((ChildId.StartsWith(TEXT("\"")) && ChildId.EndsWith(TEXT("\""))) ||
						(ChildId.StartsWith(TEXT("'")) && ChildId.EndsWith(TEXT("'")))))
					{
						ChildId = ChildId.Mid(1, ChildId.Len() - 2);
					}
					if (!ChildId.IsEmpty())
					{
						CurrentNode.Children.Add(ChildId);
					}
				}
				else if (bInDecorators && TrimmedLine.StartsWith(TEXT("- class:")))
				{
					// Save previous decorator if any
					if (!CurrentDecorator.Class.IsEmpty())
					{
						CurrentNode.Decorators.Add(CurrentDecorator);
					}
					CurrentDecorator = FManifestBTDecoratorDefinition();
					CurrentDecorator.Class = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (bInDecorators && TrimmedLine.StartsWith(TEXT("key:")))
				{
					CurrentDecorator.BlackboardKey = GetLineValue(TrimmedLine);
				}
				else if (bInDecorators && TrimmedLine.StartsWith(TEXT("blackboard_key:")))
				{
					CurrentDecorator.BlackboardKey = GetLineValue(TrimmedLine);
				}
				// v4.13.2: Support both operation: and key_query: aliases
				else if (bInDecorators && (TrimmedLine.StartsWith(TEXT("operation:")) || TrimmedLine.StartsWith(TEXT("key_query:"))))
				{
					CurrentDecorator.Operation = GetLineValue(TrimmedLine);
				}
				// v4.13.2: NotifyObserver for BTDecorator_Blackboard (OnResultChange, OnValueChange)
				else if (bInDecorators && (TrimmedLine.StartsWith(TEXT("notify_observer:")) || TrimmedLine.StartsWith(TEXT("notifyobserver:"))))
				{
					CurrentDecorator.NotifyObserver = GetLineValue(TrimmedLine);
				}
				// v4.0: Enhanced decorator properties
				else if (bInDecorators && (TrimmedLine.StartsWith(TEXT("inverse_condition:")) || TrimmedLine.StartsWith(TEXT("binversecondition:"))))
				{
					FString Value = GetLineValue(TrimmedLine);
					CurrentDecorator.bInverseCondition = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
				else if (bInDecorators && (TrimmedLine.StartsWith(TEXT("flow_abort_mode:")) || TrimmedLine.StartsWith(TEXT("flowabortmode:"))))
				{
					CurrentDecorator.FlowAbortMode = GetLineValue(TrimmedLine);
				}
				else if (bInServices && TrimmedLine.StartsWith(TEXT("- class:")))
				{
					// Save previous service if any
					if (!CurrentService.Class.IsEmpty())
					{
						CurrentNode.Services.Add(CurrentService);
					}
					CurrentService = FManifestBTServiceDefinition();
					CurrentService.Class = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (bInServices && TrimmedLine.StartsWith(TEXT("interval:")))
				{
					CurrentService.Interval = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// v4.0: Enhanced service properties
				else if (bInServices && (TrimmedLine.StartsWith(TEXT("random_deviation:")) || TrimmedLine.StartsWith(TEXT("randomdeviation:"))))
				{
					CurrentService.RandomDeviation = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (bInServices && (TrimmedLine.StartsWith(TEXT("call_tick_on_search_start:")) || TrimmedLine.StartsWith(TEXT("bcalltickonssearchstart:"))))
				{
					FString Value = GetLineValue(TrimmedLine);
					CurrentService.bCallTickOnSearchStart = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
				else if (bInServices && (TrimmedLine.StartsWith(TEXT("restart_timer_on_activation:")) || TrimmedLine.StartsWith(TEXT("brestarttimeronactivation:"))))
				{
					FString Value = GetLineValue(TrimmedLine);
					CurrentService.bRestartTimerOnActivation = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				}
			}
		}

		LineIndex++;
	}

	// Save last decorator/service
	if (!CurrentDecorator.Class.IsEmpty())
	{
		CurrentNode.Decorators.Add(CurrentDecorator);
	}
	if (!CurrentService.Class.IsEmpty())
	{
		CurrentNode.Services.Add(CurrentService);
	}
	// Save current node if valid
	if (bInNodes && !CurrentNode.Id.IsEmpty())
	{
		CurrentDef.Nodes.Add(CurrentNode);
	}
	// v2.6.14: Prefix validation - only add if BT_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BT_")))
	{
		OutData.BehaviorTrees.Add(CurrentDef);
	}
}

// v2.6.12: Enhanced material parser with expression graph support
void FGasAbilityGeneratorParser::ParseMaterials(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestMaterialDefinition CurrentDef;
	FManifestMaterialExpression CurrentExpr;
	FManifestMaterialConnection CurrentConn;
	bool bInItem = false;
	bool bInExpressions = false;
	bool bInConnections = false;
	bool bInExpression = false;
	bool bInConnection = false;
	bool bInProperties = false;
	bool bInInputs = false;          // v4.10: Parsing switch inputs section
	bool bInFunctionInputs = false;  // v4.10: Parsing function_inputs section

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending expression/connection
			if (bInExpression && !CurrentExpr.Id.IsEmpty())
			{
				CurrentDef.Expressions.Add(CurrentExpr);
			}
			if (bInConnection && !CurrentConn.FromId.IsEmpty())
			{
				CurrentDef.Connections.Add(CurrentConn);
			}
			// v2.6.14: Prefix validation - only add if M_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("M_")))
			{
				OutData.Materials.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New material item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save pending expression/connection
			if (bInExpression && !CurrentExpr.Id.IsEmpty())
			{
				CurrentDef.Expressions.Add(CurrentExpr);
			}
			if (bInConnection && !CurrentConn.FromId.IsEmpty())
			{
				CurrentDef.Connections.Add(CurrentConn);
			}
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("M_")))
			{
				OutData.Materials.Add(CurrentDef);
			}
			CurrentDef = FManifestMaterialDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInExpressions = false;
			bInConnections = false;
			bInExpression = false;
			bInConnection = false;
			bInProperties = false;
		}
		else if (bInItem)
		{
			// Material properties
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("blend_mode:")))
			{
				CurrentDef.BlendMode = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("shading_model:")))
			{
				CurrentDef.ShadingModel = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("two_sided:")))
			{
				CurrentDef.bTwoSided = GetLineValue(TrimmedLine).ToBool();
				bInExpressions = false;
				bInConnections = false;
			}
			// v4.0: Extended material properties
			else if (TrimmedLine.StartsWith(TEXT("material_domain:")))
			{
				CurrentDef.MaterialDomain = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_mode:")))
			{
				CurrentDef.CullMode = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("opacity_mask_clip_value:")))
			{
				CurrentDef.OpacityMaskClipValue = FCString::Atof(*GetLineValue(TrimmedLine));
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("translucency_pass:")))
			{
				CurrentDef.TranslucencyPass = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("enable_separate_translucency:")) || TrimmedLine.StartsWith(TEXT("separate_translucency:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				CurrentDef.bEnableSeparateTranslucency = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("enable_responsive_aa:")) || TrimmedLine.StartsWith(TEXT("responsive_aa:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				CurrentDef.bEnableResponsiveAA = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("decal_response:")))
			{
				CurrentDef.DecalResponse = GetLineValue(TrimmedLine);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cast_dynamic_shadow:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				CurrentDef.bCastDynamicShadow = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("affect_dynamic_indirect_lighting:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				CurrentDef.bAffectDynamicIndirectLighting = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				bInExpressions = false;
				bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("block_gi:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				CurrentDef.bBlockGI = Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
				bInExpressions = false;
				bInConnections = false;
			}
			// v2.6.12: Expressions section
			else if (TrimmedLine.StartsWith(TEXT("expressions:")))
			{
				bInExpressions = true;
				bInConnections = false;
				bInExpression = false;
				bInProperties = false;
				CurrentExpr = FManifestMaterialExpression();
			}
			// v2.6.12: Connections section
			else if (TrimmedLine.StartsWith(TEXT("connections:")))
			{
				// Save pending expression
				if (bInExpression && !CurrentExpr.Id.IsEmpty())
				{
					CurrentDef.Expressions.Add(CurrentExpr);
				}
				bInExpressions = false;
				bInConnections = true;
				bInExpression = false;
				bInConnection = false;
				bInProperties = false;
				CurrentConn = FManifestMaterialConnection();
			}
			// Expression item
			else if (bInExpressions && TrimmedLine.StartsWith(TEXT("- id:")))
			{
				// Save previous expression
				if (bInExpression && !CurrentExpr.Id.IsEmpty())
				{
					CurrentDef.Expressions.Add(CurrentExpr);
				}
				CurrentExpr = FManifestMaterialExpression();
				CurrentExpr.Id = GetLineValue(TrimmedLine.Mid(2));
				bInExpression = true;
				bInProperties = false;
				bInInputs = false;          // v4.10
				bInFunctionInputs = false;  // v4.10
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentExpr.Type = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("name:")))
			{
				CurrentExpr.Name = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && (TrimmedLine.StartsWith(TEXT("default:")) || TrimmedLine.StartsWith(TEXT("default_value:"))))
			{
				CurrentExpr.DefaultValue = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("pos_x:")))
			{
				CurrentExpr.PosX = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("pos_y:")))
			{
				CurrentExpr.PosY = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInProperties = false;
			}
			// v4.0: Texture-specific expression properties
			else if (bInExpression && (TrimmedLine.StartsWith(TEXT("texture_path:")) || TrimmedLine.StartsWith(TEXT("texture:"))))
			{
				CurrentExpr.TexturePath = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("sampler_type:")))
			{
				CurrentExpr.SamplerType = GetLineValue(TrimmedLine);
				bInProperties = false;
				bInInputs = false;
				bInFunctionInputs = false;
			}
			// v4.10: MaterialFunctionCall function path
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("function:")))
			{
				CurrentExpr.Function = GetLineValue(TrimmedLine);
				bInProperties = false;
				bInInputs = false;
				bInFunctionInputs = false;
			}
			// v4.10: Switch expression inputs section (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("inputs:")))
			{
				bInInputs = true;
				bInProperties = false;
				bInFunctionInputs = false;
			}
			// v4.10: MaterialFunctionCall function_inputs section
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("function_inputs:")))
			{
				bInFunctionInputs = true;
				bInProperties = false;
				bInInputs = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("properties:")))
			{
				bInProperties = true;
				bInInputs = false;
				bInFunctionInputs = false;
			}
			// v4.10: Parse switch inputs (e.g., "default: ExpressionId", "low: LowQualityExpr")
			else if (bInExpression && bInInputs && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonIdx;
				TrimmedLine.FindChar(TEXT(':'), ColonIdx);
				FString Key = TrimmedLine.Left(ColonIdx).TrimStartAndEnd();
				FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStartAndEnd();
				CurrentExpr.Inputs.Add(Key, Value);
			}
			// v4.10: Parse function inputs (e.g., "Intensity: IntensityParam")
			else if (bInExpression && bInFunctionInputs && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonIdx;
				TrimmedLine.FindChar(TEXT(':'), ColonIdx);
				FString Key = TrimmedLine.Left(ColonIdx).TrimStartAndEnd();
				FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStartAndEnd();
				CurrentExpr.FunctionInputs.Add(Key, Value);
			}
			else if (bInExpression && bInProperties && TrimmedLine.Contains(TEXT(":")))
			{
				// Parse property key:value
				int32 ColonIdx;
				TrimmedLine.FindChar(TEXT(':'), ColonIdx);
				FString Key = TrimmedLine.Left(ColonIdx).TrimStartAndEnd();
				FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStartAndEnd();
				CurrentExpr.Properties.Add(Key, Value);
			}
			// Connection item
			else if (bInConnections && TrimmedLine.StartsWith(TEXT("- from:")))
			{
				// Save previous connection
				if (bInConnection && !CurrentConn.FromId.IsEmpty())
				{
					CurrentDef.Connections.Add(CurrentConn);
				}
				CurrentConn = FManifestMaterialConnection();
				CurrentConn.FromId = GetLineValue(TrimmedLine.Mid(2));
				bInConnection = true;
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("from_output:")))
			{
				CurrentConn.FromOutput = GetLineValue(TrimmedLine);
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("to:")))
			{
				CurrentConn.ToId = GetLineValue(TrimmedLine);
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("to_input:")))
			{
				CurrentConn.ToInput = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
	}

	// Save any pending items
	if (bInExpression && !CurrentExpr.Id.IsEmpty())
	{
		CurrentDef.Expressions.Add(CurrentExpr);
	}
	if (bInConnection && !CurrentConn.FromId.IsEmpty())
	{
		CurrentDef.Connections.Add(CurrentConn);
	}
	// v2.6.14: Prefix validation - only add if M_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("M_")))
	{
		OutData.Materials.Add(CurrentDef);
	}
}

// v2.6.12: Parse material_functions section
// v3.9.7: Fixed multi-item parsing with indent-based subsection detection
void FGasAbilityGeneratorParser::ParseMaterialFunctions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestMaterialFunctionDefinition CurrentDef;
	FManifestMaterialFunctionInput CurrentInput;
	FManifestMaterialFunctionOutput CurrentOutput;
	FManifestMaterialExpression CurrentExpr;
	FManifestMaterialConnection CurrentConn;
	bool bInItem = false;
	bool bInInputs = false;
	bool bInOutputs = false;
	bool bInExpressions = false;
	bool bInConnections = false;
	bool bInInput = false;
	bool bInOutput = false;
	bool bInExpression = false;
	bool bInConnection = false;
	bool bInProperties = false;
	int32 ItemIndent = -1;  // v3.9.7: Track item indent level

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);  // v3.9.7: Track indent level

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save all pending items
			if (bInInput && !CurrentInput.Name.IsEmpty()) CurrentDef.Inputs.Add(CurrentInput);
			if (bInOutput && !CurrentOutput.Name.IsEmpty()) CurrentDef.Outputs.Add(CurrentOutput);
			if (bInExpression && !CurrentExpr.Id.IsEmpty()) CurrentDef.Expressions.Add(CurrentExpr);
			if (bInConnection && !CurrentConn.FromId.IsEmpty()) CurrentDef.Connections.Add(CurrentConn);
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("MF_")))
			{
				OutData.MaterialFunctions.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// v3.9.7: Use indent level to detect new items - if we see "- name:" at item level,
		// it's a new item regardless of subsection flags (fixes multi-item parsing)
		bool bIsNewItem = TrimmedLine.StartsWith(TEXT("- name:")) &&
			(ItemIndent < 0 || CurrentIndent <= ItemIndent);

		if (bIsNewItem)
		{
			// Save all pending from previous item
			if (bInInput && !CurrentInput.Name.IsEmpty()) CurrentDef.Inputs.Add(CurrentInput);
			if (bInOutput && !CurrentOutput.Name.IsEmpty()) CurrentDef.Outputs.Add(CurrentOutput);
			if (bInExpression && !CurrentExpr.Id.IsEmpty()) CurrentDef.Expressions.Add(CurrentExpr);
			if (bInConnection && !CurrentConn.FromId.IsEmpty()) CurrentDef.Connections.Add(CurrentConn);
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("MF_")))
			{
				OutData.MaterialFunctions.Add(CurrentDef);
			}
			CurrentDef = FManifestMaterialFunctionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			if (ItemIndent < 0) ItemIndent = CurrentIndent;  // v3.9.7: Store item indent
			bInInputs = false;
			bInOutputs = false;
			bInExpressions = false;
			bInConnections = false;
			bInInput = false;
			bInOutput = false;
			bInExpression = false;
			bInConnection = false;
			bInProperties = false;
		}
		else if (bInItem)
		{
			// Function properties
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInInputs = false; bInOutputs = false; bInExpressions = false; bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentDef.Description = GetLineValue(TrimmedLine);
				bInInputs = false; bInOutputs = false; bInExpressions = false; bInConnections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("expose_to_library:")))
			{
				CurrentDef.bExposeToLibrary = GetLineValue(TrimmedLine).ToBool();
				bInInputs = false; bInOutputs = false; bInExpressions = false; bInConnections = false;
			}
			// Inputs section
			else if (TrimmedLine.StartsWith(TEXT("inputs:")))
			{
				bInInputs = true;
				bInOutputs = false; bInExpressions = false; bInConnections = false;
				bInInput = false;
				CurrentInput = FManifestMaterialFunctionInput();
			}
			// Outputs section
			else if (TrimmedLine.StartsWith(TEXT("outputs:")))
			{
				if (bInInput && !CurrentInput.Name.IsEmpty()) CurrentDef.Inputs.Add(CurrentInput);
				bInInputs = false;
				bInOutputs = true;
				bInExpressions = false; bInConnections = false;
				bInInput = false; bInOutput = false;
				CurrentOutput = FManifestMaterialFunctionOutput();
			}
			// Expressions section
			else if (TrimmedLine.StartsWith(TEXT("expressions:")))
			{
				if (bInOutput && !CurrentOutput.Name.IsEmpty()) CurrentDef.Outputs.Add(CurrentOutput);
				bInInputs = false; bInOutputs = false;
				bInExpressions = true;
				bInConnections = false;
				bInOutput = false; bInExpression = false;
				CurrentExpr = FManifestMaterialExpression();
			}
			// Connections section
			else if (TrimmedLine.StartsWith(TEXT("connections:")))
			{
				if (bInExpression && !CurrentExpr.Id.IsEmpty()) CurrentDef.Expressions.Add(CurrentExpr);
				bInInputs = false; bInOutputs = false; bInExpressions = false;
				bInConnections = true;
				bInExpression = false; bInConnection = false;
				CurrentConn = FManifestMaterialConnection();
			}
			// Input item
			else if (bInInputs && TrimmedLine.StartsWith(TEXT("- name:")))
			{
				if (bInInput && !CurrentInput.Name.IsEmpty()) CurrentDef.Inputs.Add(CurrentInput);
				CurrentInput = FManifestMaterialFunctionInput();
				CurrentInput.Name = GetLineValue(TrimmedLine.Mid(2));
				bInInput = true;
			}
			else if (bInInput && TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentInput.Type = GetLineValue(TrimmedLine);
			}
			else if (bInInput && (TrimmedLine.StartsWith(TEXT("default:")) || TrimmedLine.StartsWith(TEXT("default_value:"))))
			{
				CurrentInput.DefaultValue = GetLineValue(TrimmedLine);
			}
			else if (bInInput && TrimmedLine.StartsWith(TEXT("sort_priority:")))
			{
				CurrentInput.SortPriority = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			// Output item
			else if (bInOutputs && TrimmedLine.StartsWith(TEXT("- name:")))
			{
				if (bInOutput && !CurrentOutput.Name.IsEmpty()) CurrentDef.Outputs.Add(CurrentOutput);
				CurrentOutput = FManifestMaterialFunctionOutput();
				CurrentOutput.Name = GetLineValue(TrimmedLine.Mid(2));
				bInOutput = true;
			}
			else if (bInOutput && TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentOutput.Type = GetLineValue(TrimmedLine);
			}
			else if (bInOutput && TrimmedLine.StartsWith(TEXT("sort_priority:")))
			{
				CurrentOutput.SortPriority = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			// Expression item (same as materials)
			else if (bInExpressions && TrimmedLine.StartsWith(TEXT("- id:")))
			{
				if (bInExpression && !CurrentExpr.Id.IsEmpty()) CurrentDef.Expressions.Add(CurrentExpr);
				CurrentExpr = FManifestMaterialExpression();
				CurrentExpr.Id = GetLineValue(TrimmedLine.Mid(2));
				bInExpression = true;
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentExpr.Type = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("name:")))
			{
				CurrentExpr.Name = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && (TrimmedLine.StartsWith(TEXT("default:")) || TrimmedLine.StartsWith(TEXT("default_value:"))))
			{
				CurrentExpr.DefaultValue = GetLineValue(TrimmedLine);
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("pos_x:")))
			{
				CurrentExpr.PosX = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("pos_y:")))
			{
				CurrentExpr.PosY = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInProperties = false;
			}
			else if (bInExpression && TrimmedLine.StartsWith(TEXT("properties:")))
			{
				bInProperties = true;
			}
			else if (bInExpression && bInProperties && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonIdx;
				TrimmedLine.FindChar(TEXT(':'), ColonIdx);
				FString Key = TrimmedLine.Left(ColonIdx).TrimStartAndEnd();
				FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStartAndEnd();
				CurrentExpr.Properties.Add(Key, Value);
			}
			// Connection item (same as materials)
			else if (bInConnections && TrimmedLine.StartsWith(TEXT("- from:")))
			{
				if (bInConnection && !CurrentConn.FromId.IsEmpty()) CurrentDef.Connections.Add(CurrentConn);
				CurrentConn = FManifestMaterialConnection();
				CurrentConn.FromId = GetLineValue(TrimmedLine.Mid(2));
				bInConnection = true;
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("from_output:")))
			{
				CurrentConn.FromOutput = GetLineValue(TrimmedLine);
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("to:")))
			{
				CurrentConn.ToId = GetLineValue(TrimmedLine);
			}
			else if (bInConnection && TrimmedLine.StartsWith(TEXT("to_input:")))
			{
				CurrentConn.ToInput = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
	}

	// Save any pending items
	if (bInInput && !CurrentInput.Name.IsEmpty()) CurrentDef.Inputs.Add(CurrentInput);
	if (bInOutput && !CurrentOutput.Name.IsEmpty()) CurrentDef.Outputs.Add(CurrentOutput);
	if (bInExpression && !CurrentExpr.Id.IsEmpty()) CurrentDef.Expressions.Add(CurrentExpr);
	if (bInConnection && !CurrentConn.FromId.IsEmpty()) CurrentDef.Connections.Add(CurrentConn);
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("MF_")))
	{
		OutData.MaterialFunctions.Add(CurrentDef);
	}
}

// v4.9: Parse material_instances section
void FGasAbilityGeneratorParser::ParseMaterialInstances(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestMaterialInstanceDefinition CurrentDef;
	FManifestMaterialInstanceScalarParam CurrentScalar;
	FManifestMaterialInstanceVectorParam CurrentVector;
	FManifestMaterialInstanceTextureParam CurrentTexture;
	bool bInItem = false;
	bool bInScalarParams = false;
	bool bInVectorParams = false;
	bool bInTextureParams = false;
	bool bInScalar = false;
	bool bInVector = false;
	bool bInTexture = false;
	int32 ItemIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending items
			if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
			if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
			if (bInTexture && !CurrentTexture.Name.IsEmpty()) CurrentDef.TextureParams.Add(CurrentTexture);
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.MaterialInstances.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Detect new item
		if (TrimmedLine.StartsWith(TEXT("- name:")) && CurrentIndent <= ItemIndent && bInItem)
		{
			// Save current item
			if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
			if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
			if (bInTexture && !CurrentTexture.Name.IsEmpty()) CurrentDef.TextureParams.Add(CurrentTexture);
			if (!CurrentDef.Name.IsEmpty())
			{
				OutData.MaterialInstances.Add(CurrentDef);
			}
			CurrentDef = FManifestMaterialInstanceDefinition();
			bInScalarParams = false;
			bInVectorParams = false;
			bInTextureParams = false;
			bInScalar = false;
			bInVector = false;
			bInTexture = false;
		}

		// New item start
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(7));
			bInItem = true;
			ItemIndent = CurrentIndent;
		}
		else if (bInItem)
		{
			// Item properties
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine.Mid(7));
			}
			else if (TrimmedLine.StartsWith(TEXT("parent_material:")) || TrimmedLine.StartsWith(TEXT("parent:")))
			{
				FString Value = TrimmedLine.Contains(TEXT("parent_material:")) ?
					GetLineValue(TrimmedLine.Mid(16)) : GetLineValue(TrimmedLine.Mid(7));
				CurrentDef.ParentMaterial = Value;
			}
			// Scalar parameters section
			else if (TrimmedLine.StartsWith(TEXT("scalar_parameters:")) || TrimmedLine.StartsWith(TEXT("scalar_params:")))
			{
				if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
				bInScalarParams = true;
				bInVectorParams = false;
				bInTextureParams = false;
				bInScalar = false;
				CurrentScalar = FManifestMaterialInstanceScalarParam();
			}
			// Vector parameters section
			else if (TrimmedLine.StartsWith(TEXT("vector_parameters:")) || TrimmedLine.StartsWith(TEXT("vector_params:")))
			{
				if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
				if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
				bInScalarParams = false;
				bInVectorParams = true;
				bInTextureParams = false;
				bInScalar = false;
				bInVector = false;
				CurrentVector = FManifestMaterialInstanceVectorParam();
			}
			// Texture parameters section
			else if (TrimmedLine.StartsWith(TEXT("texture_parameters:")) || TrimmedLine.StartsWith(TEXT("texture_params:")))
			{
				if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
				if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
				if (bInTexture && !CurrentTexture.Name.IsEmpty()) CurrentDef.TextureParams.Add(CurrentTexture);
				bInScalarParams = false;
				bInVectorParams = false;
				bInTextureParams = true;
				bInScalar = false;
				bInVector = false;
				bInTexture = false;
				CurrentTexture = FManifestMaterialInstanceTextureParam();
			}
			// Parameter items - list format (- name: / value:)
			else if (TrimmedLine.StartsWith(TEXT("- name:")) && (bInScalarParams || bInVectorParams || bInTextureParams))
			{
				// Save previous param
				if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
				if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
				if (bInTexture && !CurrentTexture.Name.IsEmpty()) CurrentDef.TextureParams.Add(CurrentTexture);

				FString ParamName = GetLineValue(TrimmedLine.Mid(7));
				if (bInScalarParams)
				{
					CurrentScalar = FManifestMaterialInstanceScalarParam();
					CurrentScalar.Name = ParamName;
					bInScalar = true;
				}
				else if (bInVectorParams)
				{
					CurrentVector = FManifestMaterialInstanceVectorParam();
					CurrentVector.Name = ParamName;
					bInVector = true;
				}
				else if (bInTextureParams)
				{
					CurrentTexture = FManifestMaterialInstanceTextureParam();
					CurrentTexture.Name = ParamName;
					bInTexture = true;
				}
			}
			// v4.9: Simple key:value format (e.g., "Intensity: 10.0")
			else if (bInScalarParams && !TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonPos;
				if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
				{
					FString ParamName = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
					FString ParamValue = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
					FManifestMaterialInstanceScalarParam Param;
					Param.Name = ParamName;
					Param.Value = FCString::Atof(*ParamValue);
					CurrentDef.ScalarParams.Add(Param);
				}
			}
			else if (bInVectorParams && !TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonPos;
				if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
				{
					FString ParamName = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
					FString ValueStr = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
					// Remove brackets/parens if present
					ValueStr.ReplaceInline(TEXT("("), TEXT(""));
					ValueStr.ReplaceInline(TEXT(")"), TEXT(""));
					ValueStr.ReplaceInline(TEXT("["), TEXT(""));
					ValueStr.ReplaceInline(TEXT("]"), TEXT(""));
					ValueStr.ReplaceInline(TEXT("\""), TEXT(""));
					TArray<FString> Parts;
					ValueStr.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 3)
					{
						FManifestMaterialInstanceVectorParam Param;
						Param.Name = ParamName;
						Param.Value.R = FCString::Atof(*Parts[0].TrimStartAndEnd());
						Param.Value.G = FCString::Atof(*Parts[1].TrimStartAndEnd());
						Param.Value.B = FCString::Atof(*Parts[2].TrimStartAndEnd());
						Param.Value.A = Parts.Num() >= 4 ? FCString::Atof(*Parts[3].TrimStartAndEnd()) : 1.0f;
						CurrentDef.VectorParams.Add(Param);
					}
				}
			}
			else if (bInTextureParams && !TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":")))
			{
				int32 ColonPos;
				if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
				{
					FString ParamName = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
					FString ParamValue = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
					FManifestMaterialInstanceTextureParam Param;
					Param.Name = ParamName;
					Param.TexturePath = ParamValue;
					CurrentDef.TextureParams.Add(Param);
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("value:")) && bInScalar)
			{
				CurrentScalar.Value = FCString::Atof(*GetLineValue(TrimmedLine.Mid(6)));
			}
			else if (TrimmedLine.StartsWith(TEXT("value:")) && bInVector)
			{
				FString ValueStr = GetLineValue(TrimmedLine.Mid(6));
				// Remove brackets if present
				ValueStr.ReplaceInline(TEXT("["), TEXT(""));
				ValueStr.ReplaceInline(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				ValueStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentVector.Value.R = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentVector.Value.G = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentVector.Value.B = FCString::Atof(*Parts[2].TrimStartAndEnd());
					CurrentVector.Value.A = Parts.Num() >= 4 ? FCString::Atof(*Parts[3].TrimStartAndEnd()) : 1.0f;
				}
			}
			else if ((TrimmedLine.StartsWith(TEXT("texture:")) || TrimmedLine.StartsWith(TEXT("texture_path:"))) && bInTexture)
			{
				FString Value = TrimmedLine.Contains(TEXT("texture_path:")) ?
					GetLineValue(TrimmedLine.Mid(13)) : GetLineValue(TrimmedLine.Mid(8));
				CurrentTexture.TexturePath = Value;
			}
		}

		LineIndex++;
	}

	// Save final pending items
	if (bInScalar && !CurrentScalar.Name.IsEmpty()) CurrentDef.ScalarParams.Add(CurrentScalar);
	if (bInVector && !CurrentVector.Name.IsEmpty()) CurrentDef.VectorParams.Add(CurrentVector);
	if (bInTexture && !CurrentTexture.Name.IsEmpty()) CurrentDef.TextureParams.Add(CurrentTexture);
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.MaterialInstances.Add(CurrentDef);
	}
}

// v2.2.0: Parse event_graphs section
void FGasAbilityGeneratorParser::ParseEventGraphs(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestEventGraphDefinition CurrentGraph;
	bool bInItem = false;
	int32 ItemIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentGraph.Name.IsEmpty())
			{
				OutData.EventGraphs.Add(CurrentGraph);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check if this is a new event graph item
		bool bIsNewGraphItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewGraphItem = true;
		}

		if (bIsNewGraphItem)
		{
			// Save current graph and start new one
			if (bInItem && !CurrentGraph.Name.IsEmpty())
			{
				OutData.EventGraphs.Add(CurrentGraph);
			}
			CurrentGraph = FManifestEventGraphDefinition();
			CurrentGraph.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			// First graph item
			ItemIndent = CurrentIndent;
			CurrentGraph = FManifestEventGraphDefinition();
			CurrentGraph.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentGraph.Description = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
			{
				// Parse nodes subsection
				int32 NodesIndent = CurrentIndent;
				LineIndex++;
				ParseGraphNodes(Lines, LineIndex, NodesIndent, CurrentGraph);
				continue; // ParseGraphNodes already advanced LineIndex
			}
			else if (TrimmedLine.Equals(TEXT("connections:")) || TrimmedLine.StartsWith(TEXT("connections:")))
			{
				// Parse connections subsection
				int32 ConnectionsIndent = CurrentIndent;
				LineIndex++;
				ParseGraphConnections(Lines, LineIndex, ConnectionsIndent, CurrentGraph);
				continue; // ParseGraphConnections already advanced LineIndex
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentGraph.Name.IsEmpty())
	{
		OutData.EventGraphs.Add(CurrentGraph);
	}
}

// v2.2.0: Parse graph nodes subsection
// v2.8.2: Added nested parameters support for CallFunction nodes
void FGasAbilityGeneratorParser::ParseGraphNodes(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, FManifestEventGraphDefinition& OutGraph)
{
	FManifestGraphNodeDefinition CurrentNode;
	bool bInNode = false;
	bool bInProperties = false;
	bool bInParameters = false;  // v2.8.2: Track nested parameters section
	int32 NodeIndent = -1;
	int32 PropertiesIndent = -1;  // v2.8.2: Track properties indentation

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		// Exit if we're back at or before subsection level with a new section
		if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("#")))
		{
			if (CurrentIndent <= SubsectionIndent && TrimmedLine.Contains(TEXT(":")))
			{
				// Save pending node
				if (bInNode && !CurrentNode.Id.IsEmpty())
				{
					OutGraph.Nodes.Add(CurrentNode);
				}
				return;
			}
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for new node item
		bool bIsNewNode = false;
		if (TrimmedLine.StartsWith(TEXT("- id:")) && NodeIndent >= 0 && CurrentIndent <= NodeIndent)
		{
			bIsNewNode = true;
		}

		if (bIsNewNode)
		{
			// Save previous node
			if (bInNode && !CurrentNode.Id.IsEmpty())
			{
				OutGraph.Nodes.Add(CurrentNode);
			}
			CurrentNode = FManifestGraphNodeDefinition();
			CurrentNode.Id = GetLineValue(TrimmedLine.Mid(2));
			bInNode = true;
			bInProperties = false;
			bInParameters = false;
			PropertiesIndent = -1;
		}
		else if (TrimmedLine.StartsWith(TEXT("- id:")) && NodeIndent < 0)
		{
			// First node
			NodeIndent = CurrentIndent;
			CurrentNode = FManifestGraphNodeDefinition();
			CurrentNode.Id = GetLineValue(TrimmedLine.Mid(2));
			bInNode = true;
			bInProperties = false;
			bInParameters = false;
			PropertiesIndent = -1;
		}
		else if (bInNode)
		{
			if (TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentNode.Type = GetLineValue(TrimmedLine);
				bInParameters = false;  // v2.8.2: Exit parameters when hitting new property
			}
			else if (TrimmedLine.StartsWith(TEXT("position:")))
			{
				// Parse position as [X, Y]
				FString PosValue = GetLineValue(TrimmedLine);
				PosValue = PosValue.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Coords;
				PosValue.ParseIntoArray(Coords, TEXT(","));
				if (Coords.Num() >= 2)
				{
					CurrentNode.PositionX = FCString::Atof(*Coords[0].TrimStartAndEnd());
					CurrentNode.PositionY = FCString::Atof(*Coords[1].TrimStartAndEnd());
					CurrentNode.bHasPosition = true;
				}
				bInParameters = false;  // v2.8.2: Exit parameters when hitting new property
			}
			else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
			{
				bInProperties = true;
				bInParameters = false;
				PropertiesIndent = CurrentIndent;
			}
			else if (bInProperties && !TrimmedLine.StartsWith(TEXT("-")))
			{
				// Parse property key: value
				int32 ColonPos;
				if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
				{
					FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
					FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();

					// v2.8.2: Check if this is the start of a nested parameters section
					if (Key == TEXT("parameters") && Value.IsEmpty())
					{
						bInParameters = true;
						// Don't add "parameters" as a property - we'll add individual params with "param." prefix
					}
					else if (bInParameters && CurrentIndent > PropertiesIndent + 2)
					{
						// v2.8.2: We're inside nested parameters - prefix with "param."
						// Remove quotes from value
						if (Value.Len() >= 2)
						{
							if ((Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\""))) ||
								(Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'"))))
							{
								Value = Value.Mid(1, Value.Len() - 2);
							}
						}
						CurrentNode.Properties.Add(TEXT("param.") + Key, Value);
					}
					else
					{
						// v2.8.2: Regular property - exit parameters mode if we were in it
						if (bInParameters && CurrentIndent <= PropertiesIndent + 2)
						{
							bInParameters = false;
						}
						// Remove quotes
						if (Value.Len() >= 2)
						{
							if ((Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\""))) ||
								(Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'"))))
							{
								Value = Value.Mid(1, Value.Len() - 2);
							}
						}
						CurrentNode.Properties.Add(Key, Value);
					}
				}
			}
		}

		LineIndex++;
	}

	// Save last node
	if (bInNode && !CurrentNode.Id.IsEmpty())
	{
		OutGraph.Nodes.Add(CurrentNode);
	}
}

// v2.2.0: Parse graph connections subsection
void FGasAbilityGeneratorParser::ParseGraphConnections(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, FManifestEventGraphDefinition& OutGraph)
{
	FManifestGraphConnectionDefinition CurrentConn;
	bool bInConnection = false;
	int32 ConnIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		// Exit if we're back at or before subsection level with a new section
		if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("#")))
		{
			if (CurrentIndent <= SubsectionIndent && TrimmedLine.Contains(TEXT(":")))
			{
				// Save pending connection
				if (bInConnection && !CurrentConn.From.NodeId.IsEmpty())
				{
					OutGraph.Connections.Add(CurrentConn);
				}
				return;
			}
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for new connection item
		bool bIsNewConn = false;
		if (TrimmedLine.StartsWith(TEXT("- from:")) && ConnIndent >= 0 && CurrentIndent <= ConnIndent)
		{
			bIsNewConn = true;
		}

		if (bIsNewConn)
		{
			// Save previous connection
			if (bInConnection && !CurrentConn.From.NodeId.IsEmpty())
			{
				OutGraph.Connections.Add(CurrentConn);
			}
			CurrentConn = FManifestGraphConnectionDefinition();
			CurrentConn.From = ParsePinReference(GetLineValue(TrimmedLine.Mid(2)));
			bInConnection = true;
		}
		else if (TrimmedLine.StartsWith(TEXT("- from:")) && ConnIndent < 0)
		{
			// First connection
			ConnIndent = CurrentIndent;
			CurrentConn = FManifestGraphConnectionDefinition();
			CurrentConn.From = ParsePinReference(GetLineValue(TrimmedLine.Mid(2)));
			bInConnection = true;
		}
		else if (bInConnection)
		{
			if (TrimmedLine.StartsWith(TEXT("to:")))
			{
				CurrentConn.To = ParsePinReference(GetLineValue(TrimmedLine));
			}
		}

		LineIndex++;
	}

	// Save last connection
	if (bInConnection && !CurrentConn.From.NodeId.IsEmpty())
	{
		OutGraph.Connections.Add(CurrentConn);
	}
}

// v2.2.0: Parse pin reference from [NodeId, PinName] format
FManifestGraphPinReference FGasAbilityGeneratorParser::ParsePinReference(const FString& Value)
{
	FManifestGraphPinReference Ref;

	// Expected format: [NodeId, PinName]
	FString CleanValue = Value;
	CleanValue = CleanValue.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));

	TArray<FString> Parts;
	CleanValue.ParseIntoArray(Parts, TEXT(","));

	if (Parts.Num() >= 2)
	{
		Ref.NodeId = Parts[0].TrimStartAndEnd();
		Ref.PinName = Parts[1].TrimStartAndEnd();

		// Remove quotes if present
		if (Ref.NodeId.Len() >= 2)
		{
			if ((Ref.NodeId.StartsWith(TEXT("\"")) && Ref.NodeId.EndsWith(TEXT("\""))) ||
				(Ref.NodeId.StartsWith(TEXT("'")) && Ref.NodeId.EndsWith(TEXT("'"))))
			{
				Ref.NodeId = Ref.NodeId.Mid(1, Ref.NodeId.Len() - 2);
			}
		}
		if (Ref.PinName.Len() >= 2)
		{
			if ((Ref.PinName.StartsWith(TEXT("\"")) && Ref.PinName.EndsWith(TEXT("\""))) ||
				(Ref.PinName.StartsWith(TEXT("'")) && Ref.PinName.EndsWith(TEXT("'"))))
			{
				Ref.PinName = Ref.PinName.Mid(1, Ref.PinName.Len() - 2);
			}
		}
	}

	return Ref;
}

// ============================================================================
// v2.8.3: Function Override Parser
// ============================================================================

void FGasAbilityGeneratorParser::ParseFunctionOverrides(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, TArray<FManifestFunctionOverrideDefinition>& OutOverrides)
{
	FManifestFunctionOverrideDefinition CurrentOverride;
	bool bInOverride = false;
	bool bInNodes = false;
	bool bInConnections = false;
	int32 OverrideIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		// Exit if we're back at or before subsection level with a new section
		if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("#")))
		{
			if (CurrentIndent <= SubsectionIndent && TrimmedLine.Contains(TEXT(":")))
			{
				// Save pending override
				if (bInOverride && !CurrentOverride.FunctionName.IsEmpty())
				{
					OutOverrides.Add(CurrentOverride);
				}
				return;
			}
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for new override item (- function: name)
		if (TrimmedLine.StartsWith(TEXT("- function:")))
		{
			// Save previous override
			if (bInOverride && !CurrentOverride.FunctionName.IsEmpty())
			{
				OutOverrides.Add(CurrentOverride);
			}
			CurrentOverride = FManifestFunctionOverrideDefinition();
			CurrentOverride.FunctionName = GetLineValue(TrimmedLine.Mid(2));  // Skip "- "
			bInOverride = true;
			bInNodes = false;
			bInConnections = false;
			OverrideIndent = CurrentIndent;
		}
		else if (bInOverride)
		{
			if (TrimmedLine.StartsWith(TEXT("call_parent:")))
			{
				FString Val = GetLineValue(TrimmedLine);
				CurrentOverride.bCallParent = Val.Equals(TEXT("true"), ESearchCase::IgnoreCase);
			}
			else if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
			{
				bInNodes = true;
				bInConnections = false;
				int32 NodesIndent = CurrentIndent;
				LineIndex++;
				FManifestEventGraphDefinition TempGraph;
				ParseGraphNodes(Lines, LineIndex, NodesIndent, TempGraph);
				CurrentOverride.Nodes = MoveTemp(TempGraph.Nodes);
				continue;
			}
			else if (TrimmedLine.Equals(TEXT("connections:")) || TrimmedLine.StartsWith(TEXT("connections:")))
			{
				bInNodes = false;
				bInConnections = true;
				int32 ConnectionsIndent = CurrentIndent;
				LineIndex++;
				FManifestEventGraphDefinition TempGraph;
				ParseGraphConnections(Lines, LineIndex, ConnectionsIndent, TempGraph);
				CurrentOverride.Connections = MoveTemp(TempGraph.Connections);
				continue;
			}
		}

		LineIndex++;
	}

	// Save last override
	if (bInOverride && !CurrentOverride.FunctionName.IsEmpty())
	{
		OutOverrides.Add(CurrentOverride);
	}
}

// ============================================================================
// v2.3.0: New Asset Type Parsers
// ============================================================================

void FGasAbilityGeneratorParser::ParseFloatCurves(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestFloatCurveDefinition CurrentDef;
	bool bInItem = false;
	bool bInKeys = false;
	// v4.0: Track current key being parsed (for multi-line key format)
	FManifestFloatCurveKeyDefinition CurrentKey;
	bool bInKeyDef = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v4.0: Save any pending key
			if (bInKeyDef && (CurrentKey.Time != 0.0f || CurrentKey.Value != 0.0f))
			{
				CurrentDef.Keys.Add(CurrentKey);
			}
			// v2.6.14: Prefix validation - only add if FC_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("FC_")))
			{
				OutData.FloatCurves.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v4.0: Save any pending key
			if (bInKeyDef && (CurrentKey.Time != 0.0f || CurrentKey.Value != 0.0f))
			{
				CurrentDef.Keys.Add(CurrentKey);
			}
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("FC_")))
			{
				OutData.FloatCurves.Add(CurrentDef);
			}
			CurrentDef = FManifestFloatCurveDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInKeys = false;
			bInKeyDef = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInKeys = false;
				bInKeyDef = false;
			}
			// v4.0: Extrapolation modes
			else if (TrimmedLine.StartsWith(TEXT("extrapolation_before:")))
			{
				CurrentDef.ExtrapolationBefore = GetLineValue(TrimmedLine);
				bInKeys = false;
				bInKeyDef = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("extrapolation_after:")))
			{
				CurrentDef.ExtrapolationAfter = GetLineValue(TrimmedLine);
				bInKeys = false;
				bInKeyDef = false;
			}
			else if (TrimmedLine.Equals(TEXT("keys:")) || TrimmedLine.StartsWith(TEXT("keys:")))
			{
				bInKeys = true;
				bInKeyDef = false;
			}
			else if (bInKeys)
			{
				// v4.0: Handle both simple [time, value] and expanded key format
				if (TrimmedLine.StartsWith(TEXT("- time:")) || TrimmedLine.StartsWith(TEXT("- [")))
				{
					// Save previous key if exists
					if (bInKeyDef && (CurrentKey.Time != 0.0f || CurrentKey.Value != 0.0f))
					{
						CurrentDef.Keys.Add(CurrentKey);
					}
					CurrentKey = FManifestFloatCurveKeyDefinition();
					bInKeyDef = true;

					// Check for simple format: - [time, value]
					if (TrimmedLine.StartsWith(TEXT("- [")))
					{
						FString KeyValue = TrimmedLine.Mid(2).TrimStart();
						KeyValue = KeyValue.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
						TArray<FString> Parts;
						KeyValue.ParseIntoArray(Parts, TEXT(","));
						if (Parts.Num() >= 2)
						{
							CurrentKey.Time = FCString::Atof(*Parts[0].TrimStartAndEnd());
							CurrentKey.Value = FCString::Atof(*Parts[1].TrimStartAndEnd());
						}
						CurrentDef.Keys.Add(CurrentKey);
						CurrentKey = FManifestFloatCurveKeyDefinition();
						bInKeyDef = false;
					}
					else
					{
						// Expanded format: - time: value
						CurrentKey.Time = FCString::Atof(*GetLineValue(TrimmedLine.Mid(2)));
					}
				}
				else if (bInKeyDef)
				{
					// Parse key properties
					if (TrimmedLine.StartsWith(TEXT("value:")))
					{
						CurrentKey.Value = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("interp_mode:")))
					{
						CurrentKey.InterpMode = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("tangent_mode:")))
					{
						CurrentKey.TangentMode = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("arrive_tangent:")))
					{
						CurrentKey.ArriveTangent = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("leave_tangent:")))
					{
						CurrentKey.LeaveTangent = FCString::Atof(*GetLineValue(TrimmedLine));
					}
				}
			}
		}

		LineIndex++;
	}

	// v4.0: Save any pending key
	if (bInKeyDef && (CurrentKey.Time != 0.0f || CurrentKey.Value != 0.0f))
	{
		CurrentDef.Keys.Add(CurrentKey);
	}
	// v2.6.14: Prefix validation - only add if FC_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("FC_")))
	{
		OutData.FloatCurves.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseAnimationMontages(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestAnimationMontageDefinition CurrentDef;
	bool bInItem = false;
	bool bInSections = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if AM_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("AM_")))
			{
				OutData.AnimationMontages.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("AM_")))
			{
				OutData.AnimationMontages.Add(CurrentDef);
			}
			CurrentDef = FManifestAnimationMontageDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInSections = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("skeleton:")))
			{
				CurrentDef.Skeleton = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("sections:")) || TrimmedLine.StartsWith(TEXT("sections:")))
			{
				bInSections = true;
			}
			else if (bInSections && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Section = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Section.Len() >= 2 && ((Section.StartsWith(TEXT("\"")) && Section.EndsWith(TEXT("\""))) ||
					(Section.StartsWith(TEXT("'")) && Section.EndsWith(TEXT("'")))))
				{
					Section = Section.Mid(1, Section.Len() - 2);
				}
				if (!Section.IsEmpty())
				{
					CurrentDef.Sections.Add(Section);
				}
			}
		}

		LineIndex++;
	}

	// v2.6.14: Prefix validation - only add if AM_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("AM_")))
	{
		OutData.AnimationMontages.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseAnimationNotifies(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestAnimationNotifyDefinition CurrentDef;
	bool bInItem = false;
	// v4.0: Enhanced parsing states
	bool bInVariables = false;
	bool bInEventGraph = false;
	bool bInEventNodes = false;
	bool bInEventConnections = false;
	bool bInNodeProperties = false;
	FManifestActorVariableDefinition CurrentVar;
	FManifestGraphNodeDefinition CurrentNode;
	FManifestGraphConnectionDefinition CurrentConn;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending node before exit
			if (bInEventNodes && !CurrentNode.Id.IsEmpty())
			{
				CurrentDef.InlineEventGraph.Nodes.Add(CurrentNode);
			}
			// v2.6.14: Prefix validation - valid prefixes: AN_, NAS_
			if (bInItem && !CurrentDef.Name.IsEmpty() &&
				(CurrentDef.Name.StartsWith(TEXT("AN_")) || CurrentDef.Name.StartsWith(TEXT("NAS_"))))
			{
				OutData.AnimationNotifies.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save pending node before new item
			if (bInEventNodes && !CurrentNode.Id.IsEmpty())
			{
				CurrentDef.InlineEventGraph.Nodes.Add(CurrentNode);
				CurrentNode = FManifestGraphNodeDefinition();
			}
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() &&
				(CurrentDef.Name.StartsWith(TEXT("AN_")) || CurrentDef.Name.StartsWith(TEXT("NAS_"))))
			{
				OutData.AnimationNotifies.Add(CurrentDef);
			}
			CurrentDef = FManifestAnimationNotifyDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInVariables = false;
			bInEventGraph = false;
			bInEventNodes = false;
			bInEventConnections = false;
			bInNodeProperties = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInVariables = false;
				bInEventGraph = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("notify_class:")))
			{
				CurrentDef.NotifyClass = GetLineValue(TrimmedLine);
				bInVariables = false;
				bInEventGraph = false;
			}
			// v4.0: Variables section
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				bInVariables = true;
				bInEventGraph = false;
				bInEventNodes = false;
				bInEventConnections = false;
			}
			// v4.0: Event graph reference (single line)
			else if (TrimmedLine.StartsWith(TEXT("event_graph:")) && TrimmedLine.Len() > 12 && !TrimmedLine.Contains(TEXT("event_graph:\n")))
			{
				FString GraphValue = GetLineValue(TrimmedLine);
				if (!GraphValue.IsEmpty())
				{
					CurrentDef.EventGraph = GraphValue;
					bInVariables = false;
					bInEventGraph = false;
				}
				else
				{
					// Inline event graph starting
					bInEventGraph = true;
					bInVariables = false;
				}
			}
			// v4.0: Inline event graph section
			else if (TrimmedLine.Equals(TEXT("event_graph:")))
			{
				bInEventGraph = true;
				bInVariables = false;
				bInEventNodes = false;
				bInEventConnections = false;
			}
			// v4.0: Parse variables
			else if (bInVariables)
			{
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					// Save previous variable
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
					}
					CurrentVar = FManifestActorVariableDefinition();
					CurrentVar.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentVar.Type = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("default_value:")))
				{
					CurrentVar.DefaultValue = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("object_class:")) || TrimmedLine.StartsWith(TEXT("class:")))
				{
					CurrentVar.Class = GetLineValue(TrimmedLine);
				}
				// Check for section exit
				else if (!TrimmedLine.StartsWith(TEXT("-")) && GetIndentLevel(Line) <= SectionIndent + 4)
				{
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
						CurrentVar = FManifestActorVariableDefinition();
					}
					bInVariables = false;
					// Don't consume line - let other handlers process it
					continue;
				}
			}
			// v4.0: Parse inline event graph
			else if (bInEventGraph)
			{
				if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
				{
					bInEventNodes = true;
					bInEventConnections = false;
					bInNodeProperties = false;
				}
				else if (TrimmedLine.Equals(TEXT("connections:")) || TrimmedLine.StartsWith(TEXT("connections:")))
				{
					// Save pending node
					if (!CurrentNode.Id.IsEmpty())
					{
						CurrentDef.InlineEventGraph.Nodes.Add(CurrentNode);
						CurrentNode = FManifestGraphNodeDefinition();
					}
					bInEventConnections = true;
					bInEventNodes = false;
					bInNodeProperties = false;
				}
				else if (bInEventNodes)
				{
					if (TrimmedLine.StartsWith(TEXT("- id:")))
					{
						// Save previous node
						if (!CurrentNode.Id.IsEmpty())
						{
							CurrentDef.InlineEventGraph.Nodes.Add(CurrentNode);
						}
						CurrentNode = FManifestGraphNodeDefinition();
						CurrentNode.Id = GetLineValue(TrimmedLine.Mid(2));
						bInNodeProperties = false;
					}
					else if (TrimmedLine.StartsWith(TEXT("type:")))
					{
						CurrentNode.Type = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
					{
						bInNodeProperties = true;
					}
					else if (bInNodeProperties)
					{
						// Parse key: value properties
						int32 ColonPos = TrimmedLine.Find(TEXT(":"));
						if (ColonPos > 0)
						{
							FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
							FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
							CurrentNode.Properties.Add(Key, Value);
						}
					}
				}
				else if (bInEventConnections)
				{
					// Parse connections: - from: [NodeId, PinName] to: [NodeId, PinName]
					if (TrimmedLine.StartsWith(TEXT("- from:")))
					{
						CurrentConn = FManifestGraphConnectionDefinition();
						// Parse [NodeId, PinName] format
						FString FromValue = GetLineValue(TrimmedLine.Mid(2));
						FromValue.ReplaceInline(TEXT("["), TEXT(""));
						FromValue.ReplaceInline(TEXT("]"), TEXT(""));
						TArray<FString> Parts;
						FromValue.ParseIntoArray(Parts, TEXT(","), true);
						if (Parts.Num() >= 2)
						{
							CurrentConn.From.NodeId = Parts[0].TrimStartAndEnd();
							CurrentConn.From.PinName = Parts[1].TrimStartAndEnd();
						}
					}
					else if (TrimmedLine.StartsWith(TEXT("to:")))
					{
						FString ToValue = GetLineValue(TrimmedLine);
						ToValue.ReplaceInline(TEXT("["), TEXT(""));
						ToValue.ReplaceInline(TEXT("]"), TEXT(""));
						TArray<FString> Parts;
						ToValue.ParseIntoArray(Parts, TEXT(","), true);
						if (Parts.Num() >= 2)
						{
							CurrentConn.To.NodeId = Parts[0].TrimStartAndEnd();
							CurrentConn.To.PinName = Parts[1].TrimStartAndEnd();
						}
						// Add connection when we have both from and to
						if (!CurrentConn.From.NodeId.IsEmpty() && !CurrentConn.To.NodeId.IsEmpty())
						{
							CurrentDef.InlineEventGraph.Connections.Add(CurrentConn);
							CurrentConn = FManifestGraphConnectionDefinition();
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// Save pending variable
	if (bInVariables && !CurrentVar.Name.IsEmpty())
	{
		CurrentDef.Variables.Add(CurrentVar);
	}
	// Save pending node
	if (bInEventNodes && !CurrentNode.Id.IsEmpty())
	{
		CurrentDef.InlineEventGraph.Nodes.Add(CurrentNode);
	}
	// v2.6.14: Prefix validation - valid prefixes: AN_, NAS_
	if (bInItem && !CurrentDef.Name.IsEmpty() &&
		(CurrentDef.Name.StartsWith(TEXT("AN_")) || CurrentDef.Name.StartsWith(TEXT("NAS_"))))
	{
		OutData.AnimationNotifies.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseDialogueBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestDialogueBlueprintDefinition CurrentDef;
	bool bInItem = false;
	bool bInVariables = false;
	bool bInEventGraph = false;  // v2.7.6: For inline event graph parsing
	bool bInSpeakers = false;  // v3.2: Speaker array parsing
	bool bInOwnedTags = false;  // v3.2: Speaker owned tags array
	bool bInPlayerSpeaker = false;  // v4.0: Player speaker parsing
	// v4.8: Party speaker info parsing
	bool bInPartySpeakerInfo = false;
	FManifestPlayerSpeakerDefinition CurrentPartySpeaker;
	int32 ItemIndent = -1;
	FManifestActorVariableDefinition CurrentVar;
	FManifestDialogueSpeakerDefinition CurrentSpeaker;  // v3.2: Current speaker being parsed

	// v4.8.3: Player auto-adjust transform and default dialogue shot parsing state
	bool bInPlayerAutoAdjustTransform = false;
	bool bInDefaultDialogueShot = false;

	// v3.7: Dialogue tree parsing state
	bool bInDialogueTree = false;
	bool bInDialogueNodes = false;
	FManifestDialogueNodeDefinition CurrentDialogueNode;
	bool bInNodeNPCReplies = false;
	bool bInNodePlayerReplies = false;
	bool bInNodeAlternativeLines = false;
	bool bInNodeEvents = false;
	bool bInNodeConditions = false;
	bool bInEventProperties = false;
	bool bInConditionProperties = false;
	FManifestDialogueLineDefinition CurrentAltLine;
	FManifestDialogueEventDefinition CurrentEvent;
	FManifestDialogueConditionDefinition CurrentCondition;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
			}
			// v3.2: Save any pending speaker
			if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
			{
				CurrentDef.Speakers.Add(CurrentSpeaker);
			}
			// v4.8: Save any pending party speaker
			if (bInPartySpeakerInfo && !CurrentPartySpeaker.SpeakerID.IsEmpty() && CurrentPartySpeaker.SpeakerID != TEXT("Player"))
			{
				CurrentDef.PartySpeakerInfo.Add(CurrentPartySpeaker);
			}
			// v3.7: Save pending dialogue tree node
			if (bInDialogueTree && bInDialogueNodes && !CurrentDialogueNode.Id.IsEmpty())
			{
				if (bInNodeEvents && !CurrentEvent.Type.IsEmpty())
				{
					CurrentDialogueNode.Events.Add(CurrentEvent);
				}
				if (bInNodeConditions && !CurrentCondition.Type.IsEmpty())
				{
					CurrentDialogueNode.Conditions.Add(CurrentCondition);
				}
				if (bInNodeAlternativeLines && !CurrentAltLine.Text.IsEmpty())
				{
					CurrentDialogueNode.AlternativeLines.Add(CurrentAltLine);
				}
				CurrentDef.DialogueTree.Nodes.Add(CurrentDialogueNode);
			}
			// v2.6.14: Prefix validation - only add if DBP_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("DBP_")))
			{
				OutData.DialogueBlueprints.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		bool bIsNewItem = false;
		if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent >= 0 && CurrentIndent <= ItemIndent)
		{
			bIsNewItem = true;
		}

		if (bIsNewItem)
		{
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
				CurrentVar = FManifestActorVariableDefinition();
			}
			bInVariables = false;
			// v3.2: Save pending speaker and reset
			if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
			{
				CurrentDef.Speakers.Add(CurrentSpeaker);
				CurrentSpeaker = FManifestDialogueSpeakerDefinition();
			}
			bInSpeakers = false;
			bInOwnedTags = false;

			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("DBP_")))
			{
				OutData.DialogueBlueprints.Add(CurrentDef);
			}
			CurrentDef = FManifestDialogueBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (TrimmedLine.StartsWith(TEXT("- name:")) && ItemIndent < 0)
		{
			ItemIndent = CurrentIndent;
			CurrentDef = FManifestDialogueBlueprintDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("event_graph:")))
			{
				CurrentDef.EventGraphName = GetLineValue(TrimmedLine);
			}
			// v3.2: Dialogue configuration properties
			else if (TrimmedLine.StartsWith(TEXT("free_movement:")))
			{
				CurrentDef.bFreeMovement = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("unskippable:")))
			{
				CurrentDef.bUnskippable = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("can_be_exited:")))
			{
				CurrentDef.bCanBeExited = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("cinematic_bars:")))
			{
				CurrentDef.bShowCinematicBars = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("auto_rotate_speakers:")))
			{
				CurrentDef.bAutoRotateSpeakers = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("auto_stop_movement:")))
			{
				CurrentDef.bAutoStopMovement = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("priority:")))
			{
				CurrentDef.Priority = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("end_dialogue_distance:")))
			{
				CurrentDef.EndDialogueDist = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			// v3.5: Additional UDialogue properties
			else if (TrimmedLine.StartsWith(TEXT("default_head_bone_name:")))
			{
				CurrentDef.DefaultHeadBoneName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("dialogue_blend_out_time:")))
			{
				CurrentDef.DialogueBlendOutTime = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("adjust_player_transform:")))
			{
				CurrentDef.bAdjustPlayerTransform = GetLineValue(TrimmedLine).ToBool();
			}
			// v4.1: Camera shake for dialogue
			else if (TrimmedLine.StartsWith(TEXT("camera_shake:")))
			{
				CurrentDef.CameraShake = GetLineValue(TrimmedLine);
			}
			// v4.8.3: Sound attenuation for dialogue audio
			else if (TrimmedLine.StartsWith(TEXT("dialogue_sound_attenuation:")) || TrimmedLine.StartsWith(TEXT("sound_attenuation:")))
			{
				CurrentDef.DialogueSoundAttenuation = GetLineValue(TrimmedLine);
			}
			// v4.8.3: Player auto-adjust transform
			else if (TrimmedLine.Equals(TEXT("player_auto_adjust_transform:")) || TrimmedLine.StartsWith(TEXT("player_auto_adjust_transform:")))
			{
				bInPlayerAutoAdjustTransform = true;
			}
			else if (bInPlayerAutoAdjustTransform)
			{
				if (TrimmedLine.StartsWith(TEXT("location_x:")) || TrimmedLine.StartsWith(TEXT("x:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.LocationX = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("location_y:")) || TrimmedLine.StartsWith(TEXT("y:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.LocationY = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("location_z:")) || TrimmedLine.StartsWith(TEXT("z:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.LocationZ = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("pitch:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.RotationPitch = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("yaw:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.RotationYaw = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("roll:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.RotationRoll = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("scale_x:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.ScaleX = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("scale_y:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.ScaleY = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("scale_z:")))
				{
					CurrentDef.PlayerAutoAdjustTransform.ScaleZ = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (!TrimmedLine.StartsWith(TEXT("-")) && !TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("#")))
				{
					// End of transform section - let other parser handle this
					bInPlayerAutoAdjustTransform = false;
					LineIndex--;  // Re-process this line
					continue;
				}
			}
			// v4.8.3: Default dialogue shot (instanced sequence)
			else if (TrimmedLine.Equals(TEXT("default_dialogue_shot:")) || TrimmedLine.StartsWith(TEXT("default_dialogue_shot:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				if (!Value.IsEmpty())
				{
					// Inline class reference
					CurrentDef.DefaultDialogueShot.SequenceClass = Value;
				}
				else
				{
					bInDefaultDialogueShot = true;
				}
			}
			else if (bInDefaultDialogueShot)
			{
				if (TrimmedLine.StartsWith(TEXT("sequence_class:")))
				{
					CurrentDef.DefaultDialogueShot.SequenceClass = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("anchor_origin_rule:")))
				{
					CurrentDef.DefaultDialogueShot.AnchorOriginRule = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("anchor_rotation_rule:")))
				{
					CurrentDef.DefaultDialogueShot.AnchorRotationRule = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("use_180_degree_rule:")))
				{
					CurrentDef.DefaultDialogueShot.bUse180DegreeRule = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("- ")) && bInDefaultDialogueShot)
				{
					// Sequence asset list item
					CurrentDef.DefaultDialogueShot.SequenceAssets.Add(GetLineValue(TrimmedLine.Mid(2)));
				}
				else if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("#")))
				{
					// End of shot section
					bInDefaultDialogueShot = false;
					LineIndex--;
					continue;
				}
			}
			// v3.7: Dialogue tree parsing
			else if (TrimmedLine.Equals(TEXT("dialogue_tree:")) || TrimmedLine.StartsWith(TEXT("dialogue_tree:")))
			{
				// Save pending states from other sections
				if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
				{
					CurrentDef.Speakers.Add(CurrentSpeaker);
					CurrentSpeaker = FManifestDialogueSpeakerDefinition();
				}
				bInSpeakers = false;
				bInVariables = false;
				bInOwnedTags = false;
				bInPlayerSpeaker = false;  // v4.0: Reset player speaker parsing
				bInDialogueTree = true;
				bInDialogueNodes = false;
			}
			else if (bInDialogueTree)
			{
				// Handle dialogue tree properties
				if (TrimmedLine.StartsWith(TEXT("root:")))
				{
					CurrentDef.DialogueTree.RootNodeId = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.Equals(TEXT("nodes:")) || TrimmedLine.StartsWith(TEXT("nodes:")))
				{
					bInDialogueNodes = true;
					// Reset node parsing state
					bInNodeNPCReplies = false;
					bInNodePlayerReplies = false;
					bInNodeAlternativeLines = false;
					bInNodeEvents = false;
					bInNodeConditions = false;
					bInEventProperties = false;
					bInConditionProperties = false;
				}
				else if (bInDialogueNodes)
				{
					// New node: "- id:"
					if (TrimmedLine.StartsWith(TEXT("- id:")))
					{
						// Save previous node if any
						if (!CurrentDialogueNode.Id.IsEmpty())
						{
							// Save pending event/condition
							if (bInNodeEvents && !CurrentEvent.Type.IsEmpty())
							{
								CurrentDialogueNode.Events.Add(CurrentEvent);
							}
							if (bInNodeConditions && !CurrentCondition.Type.IsEmpty())
							{
								CurrentDialogueNode.Conditions.Add(CurrentCondition);
							}
							if (bInNodeAlternativeLines && !CurrentAltLine.Text.IsEmpty())
							{
								CurrentDialogueNode.AlternativeLines.Add(CurrentAltLine);
							}
							CurrentDef.DialogueTree.Nodes.Add(CurrentDialogueNode);
						}
						CurrentDialogueNode = FManifestDialogueNodeDefinition();
						CurrentDialogueNode.Id = GetLineValue(TrimmedLine.Mid(2));
						bInNodeNPCReplies = false;
						bInNodePlayerReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeEvents = false;
						bInNodeConditions = false;
						bInEventProperties = false;
						bInConditionProperties = false;
						CurrentEvent = FManifestDialogueEventDefinition();
						CurrentCondition = FManifestDialogueConditionDefinition();
						CurrentAltLine = FManifestDialogueLineDefinition();
					}
					else if (TrimmedLine.StartsWith(TEXT("type:")))
					{
						CurrentDialogueNode.Type = GetLineValue(TrimmedLine);
						bInNodeNPCReplies = false;
						bInNodePlayerReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeEvents = false;
						bInNodeConditions = false;
					}
					else if (TrimmedLine.StartsWith(TEXT("speaker:")))
					{
						CurrentDialogueNode.Speaker = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("text:")))
					{
						CurrentDialogueNode.Text = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("option_text:")))
					{
						CurrentDialogueNode.OptionText = GetLineValue(TrimmedLine);
					}
					// v4.1: Hint text for player dialogue options
					else if (TrimmedLine.StartsWith(TEXT("hint_text:")))
					{
						CurrentDialogueNode.HintText = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("audio:")))
					{
						CurrentDialogueNode.Audio = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("montage:")))
					{
						CurrentDialogueNode.Montage = GetLineValue(TrimmedLine);
					}
					// v4.1: Facial animation montage
					else if (TrimmedLine.StartsWith(TEXT("facial_animation:")))
					{
						CurrentDialogueNode.FacialAnimation = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("duration:")))
					{
						CurrentDialogueNode.Duration = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("duration_seconds:")))
					{
						CurrentDialogueNode.DurationSeconds = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("auto_select:")))
					{
						CurrentDialogueNode.bAutoSelect = GetLineValue(TrimmedLine).ToBool();
					}
					else if (TrimmedLine.StartsWith(TEXT("auto_select_if_only:")))
					{
						CurrentDialogueNode.bAutoSelectIfOnly = GetLineValue(TrimmedLine).ToBool();
					}
					else if (TrimmedLine.StartsWith(TEXT("skippable:")))
					{
						CurrentDialogueNode.bIsSkippable = GetLineValue(TrimmedLine).ToBool();
					}
					else if (TrimmedLine.StartsWith(TEXT("directed_at:")))
					{
						CurrentDialogueNode.DirectedAt = GetLineValue(TrimmedLine);
					}
					// v3.9.6: Quest shortcuts
					else if (TrimmedLine.StartsWith(TEXT("start_quest:")))
					{
						CurrentDialogueNode.StartQuest = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("complete_quest_branch:")))
					{
						CurrentDialogueNode.CompleteQuestBranch = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("fail_quest:")))
					{
						CurrentDialogueNode.FailQuest = GetLineValue(TrimmedLine);
					}
					// v4.2: Custom event callback function name
					else if (TrimmedLine.StartsWith(TEXT("on_play_node_func_name:")) || TrimmedLine.StartsWith(TEXT("on_play_func:")))
					{
						CurrentDialogueNode.OnPlayNodeFuncName = GetLineValue(TrimmedLine);
					}
					// NPC replies array
					else if (TrimmedLine.Equals(TEXT("npc_replies:")) || TrimmedLine.StartsWith(TEXT("npc_replies:")))
					{
						bInNodeNPCReplies = true;
						bInNodePlayerReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeEvents = false;
						bInNodeConditions = false;
						// Handle inline array format [reply1, reply2]
						FString Value = GetLineValue(TrimmedLine);
						if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
						{
							FString ArrayContent = Value.Mid(1, Value.Len() - 2);
							TArray<FString> Replies;
							ArrayContent.ParseIntoArray(Replies, TEXT(","), true);
							for (const FString& Reply : Replies)
							{
								CurrentDialogueNode.NPCReplies.Add(Reply.TrimStartAndEnd());
							}
							bInNodeNPCReplies = false;
						}
					}
					else if (bInNodeNPCReplies && TrimmedLine.StartsWith(TEXT("-")))
					{
						FString Reply = TrimmedLine.Mid(1).TrimStartAndEnd();
						if (!Reply.IsEmpty())
						{
							CurrentDialogueNode.NPCReplies.Add(Reply);
						}
					}
					// Player replies array
					else if (TrimmedLine.Equals(TEXT("player_replies:")) || TrimmedLine.StartsWith(TEXT("player_replies:")))
					{
						bInNodePlayerReplies = true;
						bInNodeNPCReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeEvents = false;
						bInNodeConditions = false;
						// Handle inline array format
						FString Value = GetLineValue(TrimmedLine);
						if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
						{
							FString ArrayContent = Value.Mid(1, Value.Len() - 2);
							TArray<FString> Replies;
							ArrayContent.ParseIntoArray(Replies, TEXT(","), true);
							for (const FString& Reply : Replies)
							{
								CurrentDialogueNode.PlayerReplies.Add(Reply.TrimStartAndEnd());
							}
							bInNodePlayerReplies = false;
						}
					}
					else if (bInNodePlayerReplies && TrimmedLine.StartsWith(TEXT("-")))
					{
						FString Reply = TrimmedLine.Mid(1).TrimStartAndEnd();
						if (!Reply.IsEmpty())
						{
							CurrentDialogueNode.PlayerReplies.Add(Reply);
						}
					}
					// Alternative lines array
					else if (TrimmedLine.Equals(TEXT("alternative_lines:")) || TrimmedLine.StartsWith(TEXT("alternative_lines:")))
					{
						bInNodeAlternativeLines = true;
						bInNodeNPCReplies = false;
						bInNodePlayerReplies = false;
						bInNodeEvents = false;
						bInNodeConditions = false;
						CurrentAltLine = FManifestDialogueLineDefinition();
					}
					else if (bInNodeAlternativeLines)
					{
						if (TrimmedLine.StartsWith(TEXT("- text:")))
						{
							// Save previous alt line
							if (!CurrentAltLine.Text.IsEmpty())
							{
								CurrentDialogueNode.AlternativeLines.Add(CurrentAltLine);
							}
							CurrentAltLine = FManifestDialogueLineDefinition();
							CurrentAltLine.Text = GetLineValue(TrimmedLine.Mid(2));
						}
						else if (TrimmedLine.StartsWith(TEXT("audio:")) && !CurrentAltLine.Text.IsEmpty())
						{
							CurrentAltLine.Audio = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("montage:")) && !CurrentAltLine.Text.IsEmpty())
						{
							CurrentAltLine.Montage = GetLineValue(TrimmedLine);
						}
					}
					// Events array
					else if (TrimmedLine.Equals(TEXT("events:")) || TrimmedLine.StartsWith(TEXT("events:")))
					{
						// Save pending alt line
						if (bInNodeAlternativeLines && !CurrentAltLine.Text.IsEmpty())
						{
							CurrentDialogueNode.AlternativeLines.Add(CurrentAltLine);
							CurrentAltLine = FManifestDialogueLineDefinition();
						}
						bInNodeEvents = true;
						bInNodeNPCReplies = false;
						bInNodePlayerReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeConditions = false;
						bInEventProperties = false;
						CurrentEvent = FManifestDialogueEventDefinition();
					}
					else if (bInNodeEvents)
					{
						if (TrimmedLine.StartsWith(TEXT("- type:")))
						{
							// Save previous event
							if (!CurrentEvent.Type.IsEmpty())
							{
								CurrentDialogueNode.Events.Add(CurrentEvent);
							}
							CurrentEvent = FManifestDialogueEventDefinition();
							CurrentEvent.Type = GetLineValue(TrimmedLine.Mid(2));
							bInEventProperties = false;
						}
						else if (TrimmedLine.StartsWith(TEXT("runtime:")) && !CurrentEvent.Type.IsEmpty())
						{
							CurrentEvent.Runtime = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
						{
							bInEventProperties = true;
						}
						else if (bInEventProperties && TrimmedLine.Contains(TEXT(":")))
						{
							int32 ColonPos = TrimmedLine.Find(TEXT(":"));
							FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
							FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
							if (!Key.IsEmpty())
							{
								CurrentEvent.Properties.Add(Key, Value);
							}
						}
					}
					// Conditions array
					else if (TrimmedLine.Equals(TEXT("conditions:")) || TrimmedLine.StartsWith(TEXT("conditions:")))
					{
						// Save pending event
						if (bInNodeEvents && !CurrentEvent.Type.IsEmpty())
						{
							CurrentDialogueNode.Events.Add(CurrentEvent);
							CurrentEvent = FManifestDialogueEventDefinition();
						}
						bInNodeConditions = true;
						bInNodeNPCReplies = false;
						bInNodePlayerReplies = false;
						bInNodeAlternativeLines = false;
						bInNodeEvents = false;
						bInConditionProperties = false;
						CurrentCondition = FManifestDialogueConditionDefinition();
					}
					else if (bInNodeConditions)
					{
						if (TrimmedLine.StartsWith(TEXT("- type:")))
						{
							// Save previous condition
							if (!CurrentCondition.Type.IsEmpty())
							{
								CurrentDialogueNode.Conditions.Add(CurrentCondition);
							}
							CurrentCondition = FManifestDialogueConditionDefinition();
							CurrentCondition.Type = GetLineValue(TrimmedLine.Mid(2));
							bInConditionProperties = false;
						}
						else if (TrimmedLine.StartsWith(TEXT("not:")) && !CurrentCondition.Type.IsEmpty())
						{
							CurrentCondition.bNot = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
						{
							bInConditionProperties = true;
						}
						else if (bInConditionProperties && TrimmedLine.Contains(TEXT(":")))
						{
							int32 ColonPos = TrimmedLine.Find(TEXT(":"));
							FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
							FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
							if (!Key.IsEmpty())
							{
								CurrentCondition.Properties.Add(Key, Value);
							}
						}
					}
				}
			}
			// v3.2: Speakers array parsing
			else if (TrimmedLine.Equals(TEXT("speakers:")) || TrimmedLine.StartsWith(TEXT("speakers:")))
			{
				// Save any pending speaker from previous section
				if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
				{
					CurrentDef.Speakers.Add(CurrentSpeaker);
				}
				bInSpeakers = true;
				bInVariables = false;
				bInOwnedTags = false;
				CurrentSpeaker = FManifestDialogueSpeakerDefinition();
			}
			else if (bInSpeakers)
			{
				// New speaker item: "- npc_definition:"
				if (TrimmedLine.StartsWith(TEXT("- npc_definition:")))
				{
					// Save previous speaker if any
					if (!CurrentSpeaker.NPCDefinition.IsEmpty())
					{
						CurrentDef.Speakers.Add(CurrentSpeaker);
					}
					CurrentSpeaker = FManifestDialogueSpeakerDefinition();
					CurrentSpeaker.NPCDefinition = GetLineValue(TrimmedLine.Mid(2));
					bInOwnedTags = false;
				}
				else if (TrimmedLine.StartsWith(TEXT("npc_definition:")))
				{
					CurrentSpeaker.NPCDefinition = GetLineValue(TrimmedLine);
				}
				// v4.0: Speaker ID override
				else if (TrimmedLine.StartsWith(TEXT("speaker_id:")))
				{
					CurrentSpeaker.SpeakerID = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("node_color:")))
				{
					CurrentSpeaker.NodeColor = GetLineValue(TrimmedLine);
				}
				// v4.0: Is player flag
				else if (TrimmedLine.StartsWith(TEXT("is_player:")))
				{
					CurrentSpeaker.bIsPlayer = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.Equals(TEXT("owned_tags:")) || TrimmedLine.StartsWith(TEXT("owned_tags:")))
				{
					bInOwnedTags = true;
					// Check for inline array format [tag1, tag2]
					FString Value = GetLineValue(TrimmedLine);
					if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
					{
						FString ArrayContent = Value.Mid(1, Value.Len() - 2);
						TArray<FString> Tags;
						ArrayContent.ParseIntoArray(Tags, TEXT(","), true);
						for (const FString& Tag : Tags)
						{
							CurrentSpeaker.OwnedTags.Add(Tag.TrimStartAndEnd());
						}
						bInOwnedTags = false;
					}
				}
				else if (bInOwnedTags && TrimmedLine.StartsWith(TEXT("-")))
				{
					FString Tag = TrimmedLine.Mid(1).TrimStartAndEnd();
					if (!Tag.IsEmpty())
					{
						CurrentSpeaker.OwnedTags.Add(Tag);
					}
				}
			}
			// v4.0: Player speaker configuration
			else if (TrimmedLine.Equals(TEXT("player_speaker:")) || TrimmedLine.StartsWith(TEXT("player_speaker:")))
			{
				// Save any pending speaker
				if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
				{
					CurrentDef.Speakers.Add(CurrentSpeaker);
					CurrentSpeaker = FManifestDialogueSpeakerDefinition();
				}
				bInSpeakers = false;
				bInOwnedTags = false;
				bInPlayerSpeaker = true;
			}
			else if (bInPlayerSpeaker)
			{
				if (TrimmedLine.StartsWith(TEXT("speaker_id:")))
				{
					CurrentDef.PlayerSpeaker.SpeakerID = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("node_color:")))
				{
					CurrentDef.PlayerSpeaker.NodeColor = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("selecting_reply_shot:")))
				{
					CurrentDef.PlayerSpeaker.SelectingReplyShot = GetLineValue(TrimmedLine);
				}
			}
			// v4.8: Party speaker info array (for multiplayer dialogues)
			else if (TrimmedLine.Equals(TEXT("party_speaker_info:")) || TrimmedLine.StartsWith(TEXT("party_speaker_info:")))
			{
				bInSpeakers = false;
				bInOwnedTags = false;
				bInPlayerSpeaker = false;
				bInPartySpeakerInfo = true;
				CurrentPartySpeaker = FManifestPlayerSpeakerDefinition();
			}
			else if (bInPartySpeakerInfo)
			{
				if (TrimmedLine.StartsWith(TEXT("- speaker_id:")) || TrimmedLine.StartsWith(TEXT("- ")))
				{
					// Save previous party speaker if valid
					if (!CurrentPartySpeaker.SpeakerID.IsEmpty() && CurrentPartySpeaker.SpeakerID != TEXT("Player"))
					{
						CurrentDef.PartySpeakerInfo.Add(CurrentPartySpeaker);
					}
					CurrentPartySpeaker = FManifestPlayerSpeakerDefinition();
					if (TrimmedLine.StartsWith(TEXT("- speaker_id:")))
					{
						CurrentPartySpeaker.SpeakerID = GetLineValue(TrimmedLine.Mid(2));
					}
					else
					{
						CurrentPartySpeaker.SpeakerID = GetLineValue(TrimmedLine.Mid(2));
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("speaker_id:")))
				{
					CurrentPartySpeaker.SpeakerID = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("node_color:")))
				{
					CurrentPartySpeaker.NodeColor = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("selecting_reply_shot:")))
				{
					CurrentPartySpeaker.SelectingReplyShot = GetLineValue(TrimmedLine);
				}
			}
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				// Save pending speaker when entering variables
				if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
				{
					CurrentDef.Speakers.Add(CurrentSpeaker);
					CurrentSpeaker = FManifestDialogueSpeakerDefinition();
				}
				bInSpeakers = false;
				bInOwnedTags = false;
				bInPlayerSpeaker = false;  // v4.0: Reset player speaker parsing
				bInVariables = true;
			}
			else if (bInVariables)
			{
				if (TrimmedLine.StartsWith(TEXT("- name:")))
				{
					if (!CurrentVar.Name.IsEmpty())
					{
						CurrentDef.Variables.Add(CurrentVar);
					}
					CurrentVar = FManifestActorVariableDefinition();
					CurrentVar.Name = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("type:")))
				{
					CurrentVar.Type = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("default:")))
				{
					CurrentVar.DefaultValue = GetLineValue(TrimmedLine);
				}
			}
		}

		LineIndex++;
	}

	if (bInVariables && !CurrentVar.Name.IsEmpty())
	{
		CurrentDef.Variables.Add(CurrentVar);
	}
	// v3.2: Save any pending speaker
	if (bInSpeakers && !CurrentSpeaker.NPCDefinition.IsEmpty())
	{
		CurrentDef.Speakers.Add(CurrentSpeaker);
	}
	// v3.7: Save pending dialogue tree node
	if (bInDialogueTree && bInDialogueNodes && !CurrentDialogueNode.Id.IsEmpty())
	{
		if (bInNodeEvents && !CurrentEvent.Type.IsEmpty())
		{
			CurrentDialogueNode.Events.Add(CurrentEvent);
		}
		if (bInNodeConditions && !CurrentCondition.Type.IsEmpty())
		{
			CurrentDialogueNode.Conditions.Add(CurrentCondition);
		}
		if (bInNodeAlternativeLines && !CurrentAltLine.Text.IsEmpty())
		{
			CurrentDialogueNode.AlternativeLines.Add(CurrentAltLine);
		}
		CurrentDef.DialogueTree.Nodes.Add(CurrentDialogueNode);
	}
	// v2.6.14: Prefix validation - only add if DBP_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("DBP_")))
	{
		OutData.DialogueBlueprints.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseEquippableItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestEquippableItemDefinition CurrentDef;
	bool bInItem = false;
	bool bInAbilities = false;
	bool bInItemTags = false;
	// v3.4: Weapon ability array parsing states
	bool bInWeaponAbilities = false;
	bool bInMainhandAbilities = false;
	bool bInOffhandAbilities = false;
	// v3.9.6: Weapon attachment parsing states
	bool bInHolsterAttachments = false;
	bool bInWieldAttachments = false;
	// v3.10: Weapon attachment slots TMap parsing
	bool bInWeaponAttachmentSlots = false;
	FManifestWeaponAttachmentSlot CurrentAttachmentSlot;
	// v3.9.8: Clothing mesh parsing states
	bool bInClothingMesh = false;
	bool bInClothingMaterials = false;
	bool bInCurrentMaterial = false;
	bool bInVectorParams = false;
	bool bInScalarParams = false;
	bool bInMorphs = false;
	bool bInCurrentMorph = false;
	bool bInMorphNames = false;
	bool bInEquipmentEffectValues = false;
	bool bInEquipmentAbilities = false;  // v4.2
	// v4.8: Stats and ActivitiesToGrant parsing states
	bool bInStats = false;
	FManifestItemStatDefinition CurrentStat;
	bool bInActivitiesToGrant = false;
	// v4.8.2: PickupMeshData and TraceData parsing states
	bool bInPickupMeshData = false;
	bool bInPickupMeshMaterials = false;
	bool bInTraceData = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if EI_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("EI_")))
			{
				OutData.EquippableItems.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("EI_")))
			{
				OutData.EquippableItems.Add(CurrentDef);
			}
			CurrentDef = FManifestEquippableItemDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInAbilities = false;
			bInItemTags = false;
			// v3.4: Reset weapon ability array states
			bInWeaponAbilities = false;
			bInMainhandAbilities = false;
			bInOffhandAbilities = false;
			// v3.9.8: Reset clothing mesh states
			bInClothingMesh = false;
			bInClothingMaterials = false;
			bInCurrentMaterial = false;
			bInVectorParams = false;
			bInScalarParams = false;
			bInMorphs = false;
			bInCurrentMorph = false;
			bInMorphNames = false;
			bInEquipmentEffectValues = false;
			bInEquipmentAbilities = false;  // v4.2
			// v4.8: Reset stats and activities states
			bInStats = false;
			CurrentStat = FManifestItemStatDefinition();
			bInActivitiesToGrant = false;
			// v4.8.2: Reset pickup mesh and trace data states
			bInPickupMeshData = false;
			bInPickupMeshMaterials = false;
			bInTraceData = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("display_name:")))
			{
				CurrentDef.DisplayName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentDef.Description = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("equipment_slot:")))
			{
				CurrentDef.EquipmentSlot = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("equipment_modifier_ge:")))
			{
				CurrentDef.EquipmentModifierGE = GetLineValue(TrimmedLine);
			}
			// v3.3: EquippableItem stat properties
			else if (TrimmedLine.StartsWith(TEXT("attack_rating:")))
			{
				CurrentDef.AttackRating = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("armor_rating:")))
			{
				CurrentDef.ArmorRating = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("stealth_rating:")))
			{
				CurrentDef.StealthRating = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			// v3.3: NarrativeItem base properties
			else if (TrimmedLine.StartsWith(TEXT("thumbnail:")))
			{
				CurrentDef.Thumbnail = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("weight:")))
			{
				CurrentDef.Weight = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("base_value:")))
			{
				CurrentDef.BaseValue = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("base_score:")))
			{
				CurrentDef.BaseScore = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("stackable:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bStackable = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("max_stack_size:")))
			{
				CurrentDef.MaxStackSize = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("use_recharge_duration:")))
			{
				CurrentDef.UseRechargeDuration = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			// v3.4: WeaponItem properties
			else if (TrimmedLine.StartsWith(TEXT("weapon_visual_class:")))
			{
				CurrentDef.WeaponVisualClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("weapon_hand:")))
			{
				CurrentDef.WeaponHand = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("pawn_follows_control_rotation:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bPawnFollowsControlRotation = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("pawn_orients_rotation_to_movement:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bPawnOrientsRotationToMovement = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("attack_damage:")))
			{
				CurrentDef.AttackDamage = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("heavy_attack_damage_multiplier:")))
			{
				CurrentDef.HeavyAttackDamageMultiplier = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("allow_manual_reload:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bAllowManualReload = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("required_ammo:")))
			{
				CurrentDef.RequiredAmmo = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("bots_consume_ammo:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bBotsConsumeAmmo = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("bot_attack_range:")))
			{
				CurrentDef.BotAttackRange = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("clip_size:")))
			{
				CurrentDef.ClipSize = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			// v3.4: RangedWeaponItem properties
			else if (TrimmedLine.StartsWith(TEXT("aim_fov_pct:")))
			{
				CurrentDef.AimFOVPct = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("base_spread_degrees:")))
			{
				CurrentDef.BaseSpreadDegrees = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("max_spread_degrees:")))
			{
				CurrentDef.MaxSpreadDegrees = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("spread_fire_bump:")))
			{
				CurrentDef.SpreadFireBump = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("spread_decrease_speed:")))
			{
				CurrentDef.SpreadDecreaseSpeed = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			// v3.10: Additional RangedWeaponItem properties
			else if (TrimmedLine.StartsWith(TEXT("crosshair_widget:")))
			{
				CurrentDef.CrosshairWidget = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("aim_weapon_render_fov:")))
			{
				CurrentDef.AimWeaponRenderFOV = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("aim_weapon_fstop:")))
			{
				CurrentDef.AimWeaponFStop = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("move_speed_add_degrees:")))
			{
				CurrentDef.MoveSpeedAddDegrees = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("crouch_spread_multiplier:")))
			{
				CurrentDef.CrouchSpreadMultiplier = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("aim_spread_multiplier:")))
			{
				CurrentDef.AimSpreadMultiplier = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("recoil_impulse_translation_min:")))
			{
				// Parse FVector from "(X=0,Y=0,Z=0)" or "0,0,0" format
				FString VecStr = GetLineValue(TrimmedLine);
				CurrentDef.RecoilImpulseTranslationMin.InitFromString(VecStr);
			}
			else if (TrimmedLine.StartsWith(TEXT("recoil_impulse_translation_max:")))
			{
				FString VecStr = GetLineValue(TrimmedLine);
				CurrentDef.RecoilImpulseTranslationMax.InitFromString(VecStr);
			}
			else if (TrimmedLine.StartsWith(TEXT("hip_recoil_impulse_translation_min:")))
			{
				FString VecStr = GetLineValue(TrimmedLine);
				CurrentDef.HipRecoilImpulseTranslationMin.InitFromString(VecStr);
			}
			else if (TrimmedLine.StartsWith(TEXT("hip_recoil_impulse_translation_max:")))
			{
				FString VecStr = GetLineValue(TrimmedLine);
				CurrentDef.HipRecoilImpulseTranslationMax.InitFromString(VecStr);
			}
			// v3.10: Weapon attachment slots TMap parsing
			else if (TrimmedLine.Equals(TEXT("weapon_attachment_slots:")) || TrimmedLine.StartsWith(TEXT("weapon_attachment_slots:")))
			{
				bInWeaponAttachmentSlots = true;
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				CurrentAttachmentSlot = FManifestWeaponAttachmentSlot();
			}
			else if (bInWeaponAttachmentSlots)
			{
				// Check for new slot entry (starts with "- slot:")
				if (TrimmedLine.StartsWith(TEXT("- slot:")))
				{
					// Save previous slot if valid
					if (!CurrentAttachmentSlot.Slot.IsEmpty())
					{
						CurrentDef.WeaponAttachmentSlots.Add(CurrentAttachmentSlot);
					}
					// Start new slot
					CurrentAttachmentSlot = FManifestWeaponAttachmentSlot();
					CurrentAttachmentSlot.Slot = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("socket:")))
				{
					CurrentAttachmentSlot.Socket = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("offset:")))
				{
					FString VecStr = GetLineValue(TrimmedLine);
					// Support both [x,y,z] and (X=x,Y=y,Z=z) formats
					VecStr.ReplaceInline(TEXT("["), TEXT(""));
					VecStr.ReplaceInline(TEXT("]"), TEXT(""));
					TArray<FString> Parts;
					VecStr.ParseIntoArray(Parts, TEXT(","), true);
					if (Parts.Num() >= 3)
					{
						CurrentAttachmentSlot.Offset = FVector(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd())
						);
					}
					else
					{
						CurrentAttachmentSlot.Offset.InitFromString(VecStr);
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("rotation:")))
				{
					FString RotStr = GetLineValue(TrimmedLine);
					// Support [pitch,yaw,roll] format
					RotStr.ReplaceInline(TEXT("["), TEXT(""));
					RotStr.ReplaceInline(TEXT("]"), TEXT(""));
					TArray<FString> Parts;
					RotStr.ParseIntoArray(Parts, TEXT(","), true);
					if (Parts.Num() >= 3)
					{
						CurrentAttachmentSlot.Rotation = FRotator(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),  // Pitch
							FCString::Atof(*Parts[1].TrimStartAndEnd()),  // Yaw
							FCString::Atof(*Parts[2].TrimStartAndEnd())   // Roll
						);
					}
					else
					{
						CurrentAttachmentSlot.Rotation.InitFromString(RotStr);
					}
				}
				// Exit section on next top-level property (not indented enough or doesn't start with space/dash)
				else if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("-")) &&
				         (Line.Len() == TrimmedLine.Len() || (Line.Len() - TrimmedLine.Len()) <= 4))
				{
					// Save last slot and exit section
					if (!CurrentAttachmentSlot.Slot.IsEmpty())
					{
						CurrentDef.WeaponAttachmentSlots.Add(CurrentAttachmentSlot);
						CurrentAttachmentSlot = FManifestWeaponAttachmentSlot();
					}
					bInWeaponAttachmentSlots = false;
					// Don't consume the line - let it be processed by other handlers
					continue;
				}
			}
			// v3.9.6: NarrativeItem usage properties
			else if (TrimmedLine.StartsWith(TEXT("add_default_use_option:")))
			{
				CurrentDef.bAddDefaultUseOption = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("consume_on_use:")))
			{
				CurrentDef.bConsumeOnUse = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("used_with_other_item:")))
			{
				CurrentDef.bUsedWithOtherItem = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("use_action_text:")))
			{
				CurrentDef.UseActionText = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("can_activate:")))
			{
				CurrentDef.bCanActivate = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("toggle_active_on_use:")))
			{
				CurrentDef.bToggleActiveOnUse = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("use_sound:")))
			{
				CurrentDef.UseSound = GetLineValue(TrimmedLine);
			}
			// v3.9.6: Weapon attachment arrays
			else if (TrimmedLine.Equals(TEXT("holster_attachments:")) || TrimmedLine.StartsWith(TEXT("holster_attachments:")))
			{
				bInHolsterAttachments = true;
				bInWieldAttachments = false;
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
			}
			else if (TrimmedLine.Equals(TEXT("wield_attachments:")) || TrimmedLine.StartsWith(TEXT("wield_attachments:")))
			{
				bInHolsterAttachments = false;
				bInWieldAttachments = true;
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
			}
			else if (bInHolsterAttachments || bInWieldAttachments)
			{
				// Parse attachment entry
				if (TrimmedLine.StartsWith(TEXT("- slot:")))
				{
					if (bInHolsterAttachments)
					{
						CurrentDef.HolsterAttachmentSlots.Add(GetLineValue(TrimmedLine));
						CurrentDef.HolsterAttachmentSockets.Add(TEXT(""));
						CurrentDef.HolsterAttachmentOffsets.Add(FVector::ZeroVector);
						CurrentDef.HolsterAttachmentRotations.Add(FRotator::ZeroRotator);
					}
					else
					{
						CurrentDef.WieldAttachmentSlots.Add(GetLineValue(TrimmedLine));
						CurrentDef.WieldAttachmentSockets.Add(TEXT(""));
						CurrentDef.WieldAttachmentOffsets.Add(FVector::ZeroVector);
						CurrentDef.WieldAttachmentRotations.Add(FRotator::ZeroRotator);
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("socket:")))
				{
					if (bInHolsterAttachments && CurrentDef.HolsterAttachmentSockets.Num() > 0)
					{
						CurrentDef.HolsterAttachmentSockets[CurrentDef.HolsterAttachmentSockets.Num() - 1] = GetLineValue(TrimmedLine);
					}
					else if (bInWieldAttachments && CurrentDef.WieldAttachmentSockets.Num() > 0)
					{
						CurrentDef.WieldAttachmentSockets[CurrentDef.WieldAttachmentSockets.Num() - 1] = GetLineValue(TrimmedLine);
					}
				}
				// v4.0: Parse offset (format: "x, y, z" or "{x: val, y: val, z: val}")
				else if (TrimmedLine.StartsWith(TEXT("offset:")))
				{
					FString OffsetStr = GetLineValue(TrimmedLine);
					FVector Offset = FVector::ZeroVector;

					// Try parsing "x, y, z" format
					TArray<FString> Parts;
					OffsetStr.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 3)
					{
						Offset.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
						Offset.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
						Offset.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
					}

					if (bInHolsterAttachments && CurrentDef.HolsterAttachmentOffsets.Num() > 0)
					{
						CurrentDef.HolsterAttachmentOffsets[CurrentDef.HolsterAttachmentOffsets.Num() - 1] = Offset;
					}
					else if (bInWieldAttachments && CurrentDef.WieldAttachmentOffsets.Num() > 0)
					{
						CurrentDef.WieldAttachmentOffsets[CurrentDef.WieldAttachmentOffsets.Num() - 1] = Offset;
					}
				}
				// v4.0: Parse rotation (format: "pitch, yaw, roll")
				else if (TrimmedLine.StartsWith(TEXT("rotation:")))
				{
					FString RotStr = GetLineValue(TrimmedLine);
					FRotator Rotation = FRotator::ZeroRotator;

					// Try parsing "pitch, yaw, roll" format
					TArray<FString> Parts;
					RotStr.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 3)
					{
						Rotation.Pitch = FCString::Atof(*Parts[0].TrimStartAndEnd());
						Rotation.Yaw = FCString::Atof(*Parts[1].TrimStartAndEnd());
						Rotation.Roll = FCString::Atof(*Parts[2].TrimStartAndEnd());
					}

					if (bInHolsterAttachments && CurrentDef.HolsterAttachmentRotations.Num() > 0)
					{
						CurrentDef.HolsterAttachmentRotations[CurrentDef.HolsterAttachmentRotations.Num() - 1] = Rotation;
					}
					else if (bInWieldAttachments && CurrentDef.WieldAttachmentRotations.Num() > 0)
					{
						CurrentDef.WieldAttachmentRotations[CurrentDef.WieldAttachmentRotations.Num() - 1] = Rotation;
					}
				}
			}
			// v3.4: Weapon ability arrays
			else if (TrimmedLine.Equals(TEXT("weapon_abilities:")) || TrimmedLine.StartsWith(TEXT("weapon_abilities:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = true;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
			}
			else if (TrimmedLine.Equals(TEXT("mainhand_abilities:")) || TrimmedLine.StartsWith(TEXT("mainhand_abilities:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = true;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
			}
			else if (TrimmedLine.Equals(TEXT("offhand_abilities:")) || TrimmedLine.StartsWith(TEXT("offhand_abilities:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = true;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
			}
			else if (TrimmedLine.Equals(TEXT("item_tags:")) || TrimmedLine.StartsWith(TEXT("item_tags:")))
			{
				bInAbilities = false;
				bInItemTags = true;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
			}
			else if (TrimmedLine.Equals(TEXT("abilities_to_grant:")) || TrimmedLine.StartsWith(TEXT("abilities_to_grant:")))
			{
				bInAbilities = true;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = false;
			}
			// v3.9.12: Equipment effect values section
			else if (TrimmedLine.Equals(TEXT("equipment_effect_values:")) || TrimmedLine.StartsWith(TEXT("equipment_effect_values:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = false;
				bInEquipmentEffectValues = true;
				bInEquipmentAbilities = false;
			}
			// v4.2: Equipment abilities section (granted when equipped)
			else if (TrimmedLine.Equals(TEXT("equipment_abilities:")) || TrimmedLine.StartsWith(TEXT("equipment_abilities:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = false;
				bInEquipmentEffectValues = false;
				bInEquipmentAbilities = true;
				// Handle inline array format [GA_Ability1, GA_Ability2]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					FString ArrayContent = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Abilities;
					ArrayContent.ParseIntoArray(Abilities, TEXT(","), true);
					for (const FString& Ability : Abilities)
					{
						CurrentDef.EquipmentAbilities.Add(Ability.TrimStartAndEnd());
					}
					bInEquipmentAbilities = false;
				}
			}
			// v4.2: Equipment abilities array items
			else if (bInEquipmentAbilities)
			{
				if (TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString AbilityName = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!AbilityName.IsEmpty())
					{
						CurrentDef.EquipmentAbilities.Add(AbilityName);
					}
				}
			}
			// v4.8: Stats section (UI stat display)
			else if (TrimmedLine.Equals(TEXT("stats:")) || TrimmedLine.StartsWith(TEXT("stats:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = false;
				bInEquipmentEffectValues = false;
				bInEquipmentAbilities = false;
				bInStats = true;
				bInActivitiesToGrant = false;
			}
			// v4.8: Stats array item parsing
			else if (bInStats)
			{
				if (TrimmedLine.StartsWith(TEXT("- stat_name:")) || TrimmedLine.StartsWith(TEXT("- name:")))
				{
					// Save previous stat if valid
					if (!CurrentStat.StatName.IsEmpty())
					{
						CurrentDef.Stats.Add(CurrentStat);
					}
					CurrentStat = FManifestItemStatDefinition();
					CurrentStat.StatName = GetLineValue(TrimmedLine.Mid(2));
				}
				else if (TrimmedLine.StartsWith(TEXT("stat_value:")) || TrimmedLine.StartsWith(TEXT("value:")))
				{
					CurrentStat.StatValue = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("stat_icon:")) || TrimmedLine.StartsWith(TEXT("icon:")))
				{
					CurrentStat.StatIcon = GetLineValue(TrimmedLine);
				}
			}
			// v4.8: ActivitiesToGrant section
			else if (TrimmedLine.Equals(TEXT("activities_to_grant:")) || TrimmedLine.StartsWith(TEXT("activities_to_grant:")))
			{
				// Save pending stat before switching sections
				if (bInStats && !CurrentStat.StatName.IsEmpty())
				{
					CurrentDef.Stats.Add(CurrentStat);
					CurrentStat = FManifestItemStatDefinition();
				}
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = false;
				bInEquipmentEffectValues = false;
				bInEquipmentAbilities = false;
				bInStats = false;
				bInActivitiesToGrant = true;
				// Handle inline array format [BPA_Activity1, BPA_Activity2]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					FString ArrayContent = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Activities;
					ArrayContent.ParseIntoArray(Activities, TEXT(","), true);
					for (const FString& Activity : Activities)
					{
						CurrentDef.ActivitiesToGrant.Add(Activity.TrimStartAndEnd());
					}
					bInActivitiesToGrant = false;
				}
			}
			// v4.8: ActivitiesToGrant array items
			else if (bInActivitiesToGrant)
			{
				if (TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString ActivityName = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!ActivityName.IsEmpty())
					{
						CurrentDef.ActivitiesToGrant.Add(ActivityName);
					}
				}
			}
			// v4.8.2: ItemWidgetOverride - custom inventory UI widget
			else if (TrimmedLine.StartsWith(TEXT("item_widget_override:")))
			{
				CurrentDef.ItemWidgetOverride = GetLineValue(TrimmedLine);
			}
			// v4.8.2: bWantsTickByDefault - enable item ticking
			else if (TrimmedLine.StartsWith(TEXT("wants_tick_by_default:")) || TrimmedLine.StartsWith(TEXT("tick_by_default:")))
			{
				CurrentDef.bWantsTickByDefault = GetLineValue(TrimmedLine).ToBool();
			}
			// v4.8.2: PickupMeshData section
			else if (TrimmedLine.Equals(TEXT("pickup_mesh_data:")) || TrimmedLine.StartsWith(TEXT("pickup_mesh_data:")))
			{
				bInPickupMeshData = true;
				bInPickupMeshMaterials = false;
				// Reset other states
				bInAbilities = false;
				bInItemTags = false;
				bInStats = false;
				bInActivitiesToGrant = false;
			}
			// v4.8.2: PickupMeshData property parsing
			else if (bInPickupMeshData)
			{
				if (TrimmedLine.StartsWith(TEXT("mesh:")) || TrimmedLine.StartsWith(TEXT("pickup_mesh:")))
				{
					CurrentDef.PickupMeshData.PickupMesh = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.Equals(TEXT("materials:")) || TrimmedLine.StartsWith(TEXT("materials:")))
				{
					bInPickupMeshMaterials = true;
					// Check for inline array
					FString InlineValue = GetLineValue(TrimmedLine);
					if (!InlineValue.IsEmpty() && InlineValue.StartsWith(TEXT("[")))
					{
						// Parse inline array [Mat1, Mat2, ...]
						FString ArrayContent = InlineValue.Mid(1);
						ArrayContent.RemoveFromEnd(TEXT("]"));
						TArray<FString> Materials;
						ArrayContent.ParseIntoArray(Materials, TEXT(","));
						for (FString& Mat : Materials)
						{
							Mat = Mat.TrimStartAndEnd();
							if (!Mat.IsEmpty())
							{
								CurrentDef.PickupMeshData.PickupMeshMaterials.Add(Mat);
							}
						}
						bInPickupMeshMaterials = false;
					}
				}
				else if (bInPickupMeshMaterials && TrimmedLine.StartsWith(TEXT("- ")))
				{
					FString MatPath = TrimmedLine.Mid(2).TrimStartAndEnd();
					if (!MatPath.IsEmpty())
					{
						CurrentDef.PickupMeshData.PickupMeshMaterials.Add(MatPath);
					}
				}
			}
			// v4.8.2: TraceData section (for RangedWeaponItem)
			else if (TrimmedLine.Equals(TEXT("trace_data:")) || TrimmedLine.StartsWith(TEXT("trace_data:")))
			{
				bInTraceData = true;
				// Reset other states
				bInAbilities = false;
				bInItemTags = false;
				bInStats = false;
				bInActivitiesToGrant = false;
				bInPickupMeshData = false;
			}
			// v4.8.2: TraceData property parsing
			else if (bInTraceData)
			{
				if (TrimmedLine.StartsWith(TEXT("distance:")) || TrimmedLine.StartsWith(TEXT("trace_distance:")))
				{
					CurrentDef.TraceData.TraceDistance = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("radius:")) || TrimmedLine.StartsWith(TEXT("trace_radius:")))
				{
					CurrentDef.TraceData.TraceRadius = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("multi:")) || TrimmedLine.StartsWith(TEXT("trace_multi:")))
				{
					CurrentDef.TraceData.bTraceMulti = GetLineValue(TrimmedLine).ToBool();
				}
			}

			// v3.9.8: Clothing mesh section
			else if (TrimmedLine.Equals(TEXT("clothing_mesh:")) || TrimmedLine.StartsWith(TEXT("clothing_mesh:")))
			{
				bInAbilities = false;
				bInItemTags = false;
				bInWeaponAbilities = false;
				bInMainhandAbilities = false;
				bInOffhandAbilities = false;
				bInHolsterAttachments = false;
				bInWieldAttachments = false;
				bInClothingMesh = true;
				bInClothingMaterials = false;
				bInCurrentMaterial = false;
				bInVectorParams = false;
				bInScalarParams = false;
				bInMorphs = false;
				bInCurrentMorph = false;
				bInMorphNames = false;
			bInEquipmentEffectValues = false;
			}
			// v3.9.8: Clothing mesh property parsing
			else if (bInClothingMesh)
			{
				// Check for nested section headers first
				if (TrimmedLine.Equals(TEXT("materials:")) || TrimmedLine.StartsWith(TEXT("materials:")))
				{
					bInClothingMaterials = true;
					bInCurrentMaterial = false;
					bInVectorParams = false;
					bInScalarParams = false;
					bInMorphs = false;
					bInCurrentMorph = false;
					bInMorphNames = false;
			bInEquipmentEffectValues = false;
				}
				else if (TrimmedLine.Equals(TEXT("morphs:")) || TrimmedLine.StartsWith(TEXT("morphs:")))
				{
					bInClothingMaterials = false;
					bInCurrentMaterial = false;
					bInVectorParams = false;
					bInScalarParams = false;
					bInMorphs = true;
					bInCurrentMorph = false;
					bInMorphNames = false;
			bInEquipmentEffectValues = false;
				}
				else if (TrimmedLine.Equals(TEXT("vector_params:")) || TrimmedLine.StartsWith(TEXT("vector_params:")))
				{
					bInVectorParams = true;
					bInScalarParams = false;
				}
				else if (TrimmedLine.Equals(TEXT("scalar_params:")) || TrimmedLine.StartsWith(TEXT("scalar_params:")))
				{
					bInVectorParams = false;
					bInScalarParams = true;
				}
				else if (TrimmedLine.Equals(TEXT("morph_names:")) || TrimmedLine.StartsWith(TEXT("morph_names:")))
				{
					bInMorphNames = true;
				}
				// Parse morph_names array items
				else if (bInMorphNames && bInCurrentMorph && TrimmedLine.StartsWith(TEXT("-")))
				{
					// v3.9.10: Use StripYamlComment to remove inline comments
					FString MorphName = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
					// Remove quotes if present
					if (MorphName.Len() >= 2 && ((MorphName.StartsWith(TEXT("\"")) && MorphName.EndsWith(TEXT("\""))) ||
						(MorphName.StartsWith(TEXT("'")) && MorphName.EndsWith(TEXT("'")))))
					{
						MorphName = MorphName.Mid(1, MorphName.Len() - 2);
					}
					if (!MorphName.IsEmpty() && CurrentDef.ClothingMesh.Morphs.Num() > 0)
					{
						CurrentDef.ClothingMesh.Morphs.Last().MorphNames.Add(MorphName);
					}
				}
				// Parse morphs array items (new morph entry)
				else if (bInMorphs && TrimmedLine.StartsWith(TEXT("- scalar_tag:")))
				{
					FManifestClothingMorph NewMorph;
					NewMorph.ScalarTag = GetLineValue(TrimmedLine.Mid(2));
					CurrentDef.ClothingMesh.Morphs.Add(NewMorph);
					bInCurrentMorph = true;
					bInMorphNames = false;
			bInEquipmentEffectValues = false;
				}
				// Parse vector_params array items
				else if (bInVectorParams && bInCurrentMaterial && TrimmedLine.StartsWith(TEXT("- parameter_name:")))
				{
					FManifestMaterialParamBinding NewBinding;
					NewBinding.ParameterName = GetLineValue(TrimmedLine.Mid(2));
					if (CurrentDef.ClothingMesh.Materials.Num() > 0)
					{
						CurrentDef.ClothingMesh.Materials.Last().VectorParams.Add(NewBinding);
					}
				}
				else if (bInVectorParams && bInCurrentMaterial && TrimmedLine.StartsWith(TEXT("tag_id:")))
				{
					if (CurrentDef.ClothingMesh.Materials.Num() > 0 &&
						CurrentDef.ClothingMesh.Materials.Last().VectorParams.Num() > 0)
					{
						CurrentDef.ClothingMesh.Materials.Last().VectorParams.Last().TagId = GetLineValue(TrimmedLine);
					}
				}
				// Parse scalar_params array items
				else if (bInScalarParams && bInCurrentMaterial && TrimmedLine.StartsWith(TEXT("- parameter_name:")))
				{
					FManifestMaterialParamBinding NewBinding;
					NewBinding.ParameterName = GetLineValue(TrimmedLine.Mid(2));
					if (CurrentDef.ClothingMesh.Materials.Num() > 0)
					{
						CurrentDef.ClothingMesh.Materials.Last().ScalarParams.Add(NewBinding);
					}
				}
				else if (bInScalarParams && bInCurrentMaterial && TrimmedLine.StartsWith(TEXT("tag_id:")))
				{
					if (CurrentDef.ClothingMesh.Materials.Num() > 0 &&
						CurrentDef.ClothingMesh.Materials.Last().ScalarParams.Num() > 0)
					{
						CurrentDef.ClothingMesh.Materials.Last().ScalarParams.Last().TagId = GetLineValue(TrimmedLine);
					}
				}
				// Parse materials array items (new material entry)
				else if (bInClothingMaterials && TrimmedLine.StartsWith(TEXT("- material:")))
				{
					FManifestClothingMaterial NewMaterial;
					NewMaterial.Material = GetLineValue(TrimmedLine.Mid(2));
					CurrentDef.ClothingMesh.Materials.Add(NewMaterial);
					bInCurrentMaterial = true;
					bInVectorParams = false;
					bInScalarParams = false;
				}
				// Parse clothing_mesh simple properties
				else if (TrimmedLine.StartsWith(TEXT("mesh:")))
				{
					CurrentDef.ClothingMesh.Mesh = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("use_leader_pose:")))
				{
					FString Value = GetLineValue(TrimmedLine).ToLower();
					CurrentDef.ClothingMesh.bUseLeaderPose = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
				}
				else if (TrimmedLine.StartsWith(TEXT("is_static_mesh:")))
				{
					FString Value = GetLineValue(TrimmedLine).ToLower();
					CurrentDef.ClothingMesh.bIsStaticMesh = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
				}
				else if (TrimmedLine.StartsWith(TEXT("static_mesh:")))
				{
					CurrentDef.ClothingMesh.StaticMesh = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_socket:")))
				{
					CurrentDef.ClothingMesh.AttachSocket = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_location:")))
				{
					// Parse {x: 0, y: 0, z: 0} format
					FString VectorStr = GetLineValue(TrimmedLine);
					CurrentDef.ClothingMesh.AttachLocation = ParseVectorFromString(VectorStr);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_rotation:")))
				{
					// Parse {pitch: 0, yaw: 0, roll: 0} format
					FString RotatorStr = GetLineValue(TrimmedLine);
					CurrentDef.ClothingMesh.AttachRotation = ParseRotatorFromString(RotatorStr);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_scale:")))
				{
					// Parse {x: 1, y: 1, z: 1} format
					FString VectorStr = GetLineValue(TrimmedLine);
					CurrentDef.ClothingMesh.AttachScale = ParseVectorFromString(VectorStr);
				}
				else if (TrimmedLine.StartsWith(TEXT("mesh_anim_bp:")))
				{
					CurrentDef.ClothingMesh.MeshAnimBP = GetLineValue(TrimmedLine);
				}
			}
			else if (bInItemTags && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Tag = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Tag.Len() >= 2 && ((Tag.StartsWith(TEXT("\"")) && Tag.EndsWith(TEXT("\""))) ||
					(Tag.StartsWith(TEXT("'")) && Tag.EndsWith(TEXT("'")))))
				{
					Tag = Tag.Mid(1, Tag.Len() - 2);
				}
				if (!Tag.IsEmpty())
				{
					CurrentDef.ItemTags.Add(Tag);
				}
			}
			else if (bInWeaponAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Ability = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Ability.Len() >= 2 && ((Ability.StartsWith(TEXT("\"")) && Ability.EndsWith(TEXT("\""))) ||
					(Ability.StartsWith(TEXT("'")) && Ability.EndsWith(TEXT("'")))))
				{
					Ability = Ability.Mid(1, Ability.Len() - 2);
				}
				if (!Ability.IsEmpty())
				{
					CurrentDef.WeaponAbilities.Add(Ability);
				}
			}
			else if (bInMainhandAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Ability = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Ability.Len() >= 2 && ((Ability.StartsWith(TEXT("\"")) && Ability.EndsWith(TEXT("\""))) ||
					(Ability.StartsWith(TEXT("'")) && Ability.EndsWith(TEXT("'")))))
				{
					Ability = Ability.Mid(1, Ability.Len() - 2);
				}
				if (!Ability.IsEmpty())
				{
					CurrentDef.MainhandAbilities.Add(Ability);
				}
			}
			else if (bInOffhandAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Ability = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Ability.Len() >= 2 && ((Ability.StartsWith(TEXT("\"")) && Ability.EndsWith(TEXT("\""))) ||
					(Ability.StartsWith(TEXT("'")) && Ability.EndsWith(TEXT("'")))))
				{
					Ability = Ability.Mid(1, Ability.Len() - 2);
				}
				if (!Ability.IsEmpty())
				{
					CurrentDef.OffhandAbilities.Add(Ability);
				}
			}
			// v3.9.12: Equipment effect values key:value parsing
			else if (bInEquipmentEffectValues && TrimmedLine.Contains(TEXT(":")))
			{
				// Parse "Tag: Value" format (e.g., "SetByCaller.Armor: 15.0")
				int32 ColonIndex;
				if (TrimmedLine.FindChar(TEXT(':'), ColonIndex))
				{
					FString Key = TrimmedLine.Left(ColonIndex).TrimStartAndEnd();
					FString ValueStr = StripYamlComment(TrimmedLine.Mid(ColonIndex + 1).TrimStartAndEnd());
					if (!Key.IsEmpty() && !ValueStr.IsEmpty())
					{
						float Value = FCString::Atof(*ValueStr);
						CurrentDef.EquipmentEffectValues.Add(Key, Value);
					}
				}
			}
			else if (bInAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Ability = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Ability.Len() >= 2 && ((Ability.StartsWith(TEXT("\"")) && Ability.EndsWith(TEXT("\""))) ||
					(Ability.StartsWith(TEXT("'")) && Ability.EndsWith(TEXT("'")))))
				{
					Ability = Ability.Mid(1, Ability.Len() - 2);
				}
				if (!Ability.IsEmpty())
				{
					CurrentDef.AbilitiesToGrant.Add(Ability);
				}
			}
		}

		LineIndex++;
	}

	// v2.6.14: Prefix validation - only add if EI_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("EI_")))
	{
		OutData.EquippableItems.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseActivities(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestActivityDefinition CurrentDef;
	bool bInItem = false;
	// v3.3: Tag array parsing states
	bool bInOwnedTags = false;
	bool bInBlockTags = false;
	bool bInRequireTags = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v2.6.14: Prefix validation - only add if BPA_ prefix
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BPA_")))
			{
				OutData.Activities.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BPA_")))
			{
				OutData.Activities.Add(CurrentDef);
			}
			CurrentDef = FManifestActivityDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInOwnedTags = false;
			bInBlockTags = false;
			bInRequireTags = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("behavior_tree:")))
			{
				CurrentDef.BehaviorTree = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentDef.Description = GetLineValue(TrimmedLine);
			}
			// v3.3: NarrativeActivityBase properties
			else if (TrimmedLine.StartsWith(TEXT("activity_name:")))
			{
				CurrentDef.ActivityName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("supported_goal_type:")))
			{
				CurrentDef.SupportedGoalType = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("is_interruptable:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bIsInterruptable = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			else if (TrimmedLine.StartsWith(TEXT("save_activity:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bSaveActivity = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
			}
			// v3.3: Tag array sections
			else if (TrimmedLine.Equals(TEXT("owned_tags:")) || TrimmedLine.StartsWith(TEXT("owned_tags:")))
			{
				bInOwnedTags = true;
				bInBlockTags = false;
				bInRequireTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("block_tags:")) || TrimmedLine.StartsWith(TEXT("block_tags:")))
			{
				bInOwnedTags = false;
				bInBlockTags = true;
				bInRequireTags = false;
			}
			else if (TrimmedLine.Equals(TEXT("require_tags:")) || TrimmedLine.StartsWith(TEXT("require_tags:")))
			{
				bInOwnedTags = false;
				bInBlockTags = false;
				bInRequireTags = true;
			}
			else if ((bInOwnedTags || bInBlockTags || bInRequireTags) && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Tag = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Tag.Len() >= 2 && ((Tag.StartsWith(TEXT("\"")) && Tag.EndsWith(TEXT("\""))) ||
					(Tag.StartsWith(TEXT("'")) && Tag.EndsWith(TEXT("'")))))
				{
					Tag = Tag.Mid(1, Tag.Len() - 2);
				}
				if (!Tag.IsEmpty())
				{
					if (bInOwnedTags)
					{
						CurrentDef.OwnedTags.Add(Tag);
					}
					else if (bInBlockTags)
					{
						CurrentDef.BlockTags.Add(Tag);
					}
					else if (bInRequireTags)
					{
						CurrentDef.RequireTags.Add(Tag);
					}
				}
			}
		}

		LineIndex++;
	}

	// v2.6.14: Prefix validation - only add if BPA_ prefix
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BPA_")))
	{
		OutData.Activities.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseAbilityConfigurations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestAbilityConfigurationDefinition CurrentDef;
	bool bInItem = false;
	bool bInAbilities = false;
	bool bInStartupEffects = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.AbilityConfigurations.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.AbilityConfigurations.Add(CurrentDef);
			}
			CurrentDef = FManifestAbilityConfigurationDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInAbilities = false;
			bInStartupEffects = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInAbilities = false;
				bInStartupEffects = false;
			}
			// v3.1: default_attributes (single GE for attribute initialization)
			else if (TrimmedLine.StartsWith(TEXT("default_attributes:")))
			{
				CurrentDef.DefaultAttributes = GetLineValue(TrimmedLine);
				bInAbilities = false;
				bInStartupEffects = false;
			}
			// v3.1: startup_effects array
			else if (TrimmedLine.Equals(TEXT("startup_effects:")) || TrimmedLine.StartsWith(TEXT("startup_effects:")))
			{
				bInStartupEffects = true;
				bInAbilities = false;
			}
			else if (TrimmedLine.Equals(TEXT("abilities:")) || TrimmedLine.StartsWith(TEXT("abilities:")))
			{
				bInAbilities = true;
				bInStartupEffects = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Value = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Value.Len() >= 2 && ((Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\""))) ||
					(Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))))
				{
					Value = Value.Mid(1, Value.Len() - 2);
				}
				if (!Value.IsEmpty())
				{
					if (bInAbilities)
					{
						CurrentDef.Abilities.Add(Value);
					}
					else if (bInStartupEffects)
					{
						CurrentDef.StartupEffects.Add(Value);
					}
				}
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.AbilityConfigurations.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseActivityConfigurations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestActivityConfigurationDefinition CurrentDef;
	bool bInItem = false;
	bool bInActivities = false;
	bool bInGoalGenerators = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.ActivityConfigurations.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.ActivityConfigurations.Add(CurrentDef);
			}
			CurrentDef = FManifestActivityConfigurationDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInActivities = false;
			bInGoalGenerators = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInActivities = false;
				bInGoalGenerators = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("default_activity:")))
			{
				CurrentDef.DefaultActivity = GetLineValue(TrimmedLine);
				bInActivities = false;
				bInGoalGenerators = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("request_interval:")))
			{
				CurrentDef.RescoreInterval = FCString::Atof(*GetLineValue(TrimmedLine));
				bInActivities = false;
				bInGoalGenerators = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("rescore_interval:")))
			{
				CurrentDef.RescoreInterval = FCString::Atof(*GetLineValue(TrimmedLine));
				bInActivities = false;
				bInGoalGenerators = false;
			}
			else if (TrimmedLine.Equals(TEXT("activities:")) || TrimmedLine.StartsWith(TEXT("activities:")))
			{
				bInActivities = true;
				bInGoalGenerators = false;
			}
			// v3.1: goal_generators array
			else if (TrimmedLine.Equals(TEXT("goal_generators:")) || TrimmedLine.StartsWith(TEXT("goal_generators:")))
			{
				bInGoalGenerators = true;
				bInActivities = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Value = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Value.Len() >= 2 && ((Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\""))) ||
					(Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))))
				{
					Value = Value.Mid(1, Value.Len() - 2);
				}
				if (!Value.IsEmpty())
				{
					if (bInActivities)
					{
						CurrentDef.Activities.Add(Value);
					}
					else if (bInGoalGenerators)
					{
						CurrentDef.GoalGenerators.Add(Value);
					}
				}
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.ActivityConfigurations.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseItemCollections(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestItemCollectionDefinition CurrentDef;
	bool bInItem = false;
	bool bInItems = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.ItemCollections.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.ItemCollections.Add(CurrentDef);
			}
			CurrentDef = FManifestItemCollectionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInItems = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("items:")) || TrimmedLine.StartsWith(TEXT("items:")))
			{
				bInItems = true;
			}
			else if (bInItems && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Item = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Item.Len() >= 2 && ((Item.StartsWith(TEXT("\"")) && Item.EndsWith(TEXT("\""))) ||
					(Item.StartsWith(TEXT("'")) && Item.EndsWith(TEXT("'")))))
				{
					Item = Item.Mid(1, Item.Len() - 2);
				}
				if (!Item.IsEmpty())
				{
					CurrentDef.Items.Add(Item);
				}
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.ItemCollections.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseNarrativeEvents(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestNarrativeEventDefinition CurrentDef;
	bool bInItem = false;
	bool bInNPCTargets = false;
	bool bInCharacterTargets = false;
	bool bInPlayerTargets = false;
	// v4.3: Event conditions
	bool bInConditions = false;
	bool bInCondition = false;
	bool bInConditionProperties = false;
	FManifestDialogueConditionDefinition CurrentCondition;

	// v4.3: Helper to save current condition
	auto SaveCurrentCondition = [&]()
	{
		if (!CurrentCondition.Type.IsEmpty())
		{
			CurrentDef.Conditions.Add(CurrentCondition);
			CurrentCondition = FManifestDialogueConditionDefinition();
		}
	};

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			SaveCurrentCondition();  // v4.3: Save pending condition
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NarrativeEvents.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			SaveCurrentCondition();  // v4.3: Save pending condition before new item
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NarrativeEvents.Add(CurrentDef);
			}
			CurrentDef = FManifestNarrativeEventDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInNPCTargets = false;
			bInCharacterTargets = false;
			bInPlayerTargets = false;
			bInConditions = false;  // v4.3: Reset conditions state
			bInCondition = false;
			bInConditionProperties = false;
		}
		else if (bInItem)
		{
			// Check for array item (starts with "- ")
			if (TrimmedLine.StartsWith(TEXT("- ")) && !TrimmedLine.StartsWith(TEXT("- name:")))
			{
				FString ArrayValue = TrimmedLine.Mid(2).TrimStart();
				if (bInNPCTargets)
				{
					CurrentDef.NPCTargets.Add(ArrayValue);
				}
				else if (bInCharacterTargets)
				{
					CurrentDef.CharacterTargets.Add(ArrayValue);
				}
				else if (bInPlayerTargets)
				{
					CurrentDef.PlayerTargets.Add(ArrayValue);
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("parent_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("event_tag:")))
			{
				CurrentDef.EventTag = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("event_type:")))
			{
				CurrentDef.EventType = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentDef.Description = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			// v3.2: New event configuration properties
			else if (TrimmedLine.StartsWith(TEXT("event_runtime:")))
			{
				CurrentDef.EventRuntime = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("event_filter:")))
			{
				CurrentDef.EventFilter = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("party_policy:")))
			{
				CurrentDef.PartyEventPolicy = GetLineValue(TrimmedLine);
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("refire_on_load:")))
			{
				CurrentDef.bRefireOnLoad = GetLineValue(TrimmedLine).ToBool();
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			// v3.2: Target arrays
			else if (TrimmedLine.StartsWith(TEXT("npc_targets:")))
			{
				bInNPCTargets = true;
				bInCharacterTargets = false;
				bInPlayerTargets = false;
				// Check for inline array format: npc_targets: [NPC_A, NPC_B]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					Value = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Items;
					Value.ParseIntoArray(Items, TEXT(","), true);
					for (FString& Item : Items)
					{
						CurrentDef.NPCTargets.Add(Item.TrimStartAndEnd());
					}
					bInNPCTargets = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("character_targets:")))
			{
				bInNPCTargets = false;
				bInCharacterTargets = true;
				bInPlayerTargets = false;
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					Value = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Items;
					Value.ParseIntoArray(Items, TEXT(","), true);
					for (FString& Item : Items)
					{
						CurrentDef.CharacterTargets.Add(Item.TrimStartAndEnd());
					}
					bInCharacterTargets = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("player_targets:")))
			{
				bInNPCTargets = false;
				bInCharacterTargets = false;
				bInPlayerTargets = true;
				bInConditions = false;  // v4.3
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					Value = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Items;
					Value.ParseIntoArray(Items, TEXT(","), true);
					for (FString& Item : Items)
					{
						CurrentDef.PlayerTargets.Add(Item.TrimStartAndEnd());
					}
					bInPlayerTargets = false;
				}
			}
			// v4.3: Event conditions section
			else if (TrimmedLine.StartsWith(TEXT("conditions:")))
			{
				SaveCurrentCondition();
				bInConditions = true;
				bInCondition = false;
				bInConditionProperties = false;
				bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
			}
			else if (bInConditions)
			{
				if (TrimmedLine.StartsWith(TEXT("- type:")) || TrimmedLine.StartsWith(TEXT("- class:")))
				{
					SaveCurrentCondition();
					CurrentCondition.Type = GetLineValue(TrimmedLine);
					bInCondition = true;
					bInConditionProperties = false;
				}
				else if (bInCondition)
				{
					if (TrimmedLine.StartsWith(TEXT("type:")) || TrimmedLine.StartsWith(TEXT("class:")))
					{
						CurrentCondition.Type = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("not:")) || TrimmedLine.StartsWith(TEXT("invert:")))
					{
						CurrentCondition.bNot = GetLineValue(TrimmedLine).ToBool();
					}
					else if (TrimmedLine.StartsWith(TEXT("properties:")))
					{
						bInConditionProperties = true;
					}
					else if (bInConditionProperties || (!TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":"))))
					{
						// Parse property
						FString PropKey, PropValue;
						if (TrimmedLine.Split(TEXT(":"), &PropKey, &PropValue))
						{
							PropKey = PropKey.TrimStart().TrimEnd();
							PropValue = PropValue.TrimStart().TrimEnd().TrimQuotes();
							if (!PropKey.IsEmpty() && !PropKey.StartsWith(TEXT("-")) && PropKey != TEXT("type") && PropKey != TEXT("class"))
							{
								CurrentCondition.Properties.Add(PropKey, PropValue);
							}
						}
					}
				}
			}
			// v4.1: Child class properties introspection
			// Format: property_name: value (any key not recognized as standard property)
			else
			{
				// Check if this looks like a property definition (contains colon, not an array item)
				int32 ColonPos = TrimmedLine.Find(TEXT(":"));
				if (ColonPos != INDEX_NONE && !TrimmedLine.StartsWith(TEXT("-")))
				{
					FString PropertyName = TrimmedLine.Left(ColonPos).TrimEnd();
					FString PropertyValue = TrimmedLine.Mid(ColonPos + 1).TrimStart();
					// Add to properties map for child class introspection
					CurrentDef.Properties.Add(PropertyName, PropertyValue);
					bInNPCTargets = bInCharacterTargets = bInPlayerTargets = false;
				}
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.NarrativeEvents.Add(CurrentDef);
	}
}

// ============================================================================
// v4.0: ParseGameplayCues - NEW GENERATOR
// ============================================================================
void FGasAbilityGeneratorParser::ParseGameplayCues(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestGameplayCueDefinition CurrentDef;
	bool bInItem = false;
	bool bInSpawnCondition = false;
	bool bInPlacement = false;
	bool bInBurstEffects = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.GameplayCues.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.GameplayCues.Add(CurrentDef);
			}
			CurrentDef = FManifestGameplayCueDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInSpawnCondition = false;
			bInPlacement = false;
			bInBurstEffects = false;
		}
		else if (bInItem)
		{
			// Check for subsection starts
			if (TrimmedLine.StartsWith(TEXT("spawn_condition:")))
			{
				bInSpawnCondition = true;
				bInPlacement = false;
				bInBurstEffects = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("placement:")))
			{
				bInSpawnCondition = false;
				bInPlacement = true;
				bInBurstEffects = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("burst_effects:")))
			{
				bInSpawnCondition = false;
				bInPlacement = false;
				bInBurstEffects = true;
			}
			// Main properties
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInSpawnCondition = bInPlacement = bInBurstEffects = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cue_type:")))
			{
				CurrentDef.CueType = GetLineValue(TrimmedLine);
				bInSpawnCondition = bInPlacement = bInBurstEffects = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("gameplay_cue_tag:")))
			{
				CurrentDef.GameplayCueTag = GetLineValue(TrimmedLine);
				bInSpawnCondition = bInPlacement = bInBurstEffects = false;
			}
			// Spawn condition properties
			else if (bInSpawnCondition)
			{
				if (TrimmedLine.StartsWith(TEXT("attach_policy:")))
				{
					CurrentDef.SpawnCondition.AttachPolicy = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_socket:")))
				{
					CurrentDef.SpawnCondition.AttachSocket = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("spawn_probability:")))
				{
					CurrentDef.SpawnCondition.SpawnProbability = FCString::Atof(*GetLineValue(TrimmedLine));
				}
			}
			// Placement properties
			else if (bInPlacement)
			{
				if (TrimmedLine.StartsWith(TEXT("socket_name:")))
				{
					CurrentDef.Placement.SocketName = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("attach_to_owner:")))
				{
					CurrentDef.Placement.bAttachToOwner = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("relative_offset:")))
				{
					// Parse "(X, Y, Z)" or "X, Y, Z" format
					FString OffsetStr = GetLineValue(TrimmedLine);
					OffsetStr.ReplaceInline(TEXT("("), TEXT(""));
					OffsetStr.ReplaceInline(TEXT(")"), TEXT(""));
					TArray<FString> Parts;
					OffsetStr.ParseIntoArray(Parts, TEXT(","), true);
					if (Parts.Num() >= 3)
					{
						CurrentDef.Placement.RelativeOffset = FVector(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()));
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("relative_rotation:")))
				{
					// Parse "(P, Y, R)" or "P, Y, R" format
					FString RotStr = GetLineValue(TrimmedLine);
					RotStr.ReplaceInline(TEXT("("), TEXT(""));
					RotStr.ReplaceInline(TEXT(")"), TEXT(""));
					TArray<FString> Parts;
					RotStr.ParseIntoArray(Parts, TEXT(","), true);
					if (Parts.Num() >= 3)
					{
						CurrentDef.Placement.RelativeRotation = FRotator(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()));
					}
				}
			}
			// Burst effects properties
			else if (bInBurstEffects)
			{
				if (TrimmedLine.StartsWith(TEXT("particle_system:")))
				{
					CurrentDef.BurstEffects.ParticleSystem = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("sound:")))
				{
					CurrentDef.BurstEffects.Sound = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("camera_shake:")))
				{
					CurrentDef.BurstEffects.CameraShake = GetLineValue(TrimmedLine);
				}
			}
		}

		LineIndex++;
	}

	// Add final item
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.GameplayCues.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseNPCDefinitions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestNPCDefinitionDefinition CurrentDef;
	bool bInItem = false;
	// v3.6: Array parsing states
	bool bInOwnedTags = false;
	bool bInFactions = false;
	bool bInActivitySchedules = false;
	// v3.7: Item loadout collections array state
	bool bInItemLoadoutCollections = false;

	// v3.9.5: Loot table roll parsing states
	bool bInDefaultItemLoadout = false;
	bool bInTradingItemLoadout = false;
	FManifestLootTableRollDefinition CurrentRoll;
	bool bInRollItems = false;                    // Parsing "items:" array inside a roll
	bool bInRollItemCollections = false;          // Parsing "item_collections:" array inside a roll
	FManifestItemWithQuantityDefinition CurrentItemDef;
	bool bInItemDef = false;                      // Currently building an item definition

	// Lambda to save current roll if valid
	auto SaveCurrentRoll = [&]() {
		if (CurrentRoll.ItemsToGrant.Num() > 0 || CurrentRoll.ItemCollectionsToGrant.Num() > 0 || !CurrentRoll.TableToRoll.IsEmpty())
		{
			if (bInDefaultItemLoadout)
			{
				CurrentDef.DefaultItemLoadout.Add(CurrentRoll);
			}
			else if (bInTradingItemLoadout)
			{
				CurrentDef.TradingItemLoadout.Add(CurrentRoll);
			}
		}
		CurrentRoll = FManifestLootTableRollDefinition();
		bInRollItems = false;
		bInRollItemCollections = false;
		bInItemDef = false;
	};

	// Lambda to save current item definition if valid
	auto SaveCurrentItemDef = [&]() {
		if (!CurrentItemDef.Item.IsEmpty())
		{
			CurrentRoll.ItemsToGrant.Add(CurrentItemDef);
		}
		CurrentItemDef = FManifestItemWithQuantityDefinition();
		bInItemDef = false;
	};

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			// v3.9.5: Save any pending loot table roll
			if (bInItemDef) SaveCurrentItemDef();
			if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();

			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NPCDefinitions.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// v3.9.5: Save any pending loot table roll before starting new item
			if (bInItemDef) SaveCurrentItemDef();
			if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();

			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NPCDefinitions.Add(CurrentDef);
			}
			CurrentDef = FManifestNPCDefinitionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			// v3.6: Reset array parsing states
			bInOwnedTags = false;
			bInFactions = false;
			bInActivitySchedules = false;
			bInItemLoadoutCollections = false;
			// v3.9.5: Reset loot table parsing states
			bInDefaultItemLoadout = false;
			bInTradingItemLoadout = false;
			bInRollItems = false;
			bInRollItemCollections = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("npc_id:")))
			{
				CurrentDef.NPCID = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("npc_name:")) || TrimmedLine.StartsWith(TEXT("display_name:")))
			{
				CurrentDef.NPCName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("npc_class_path:")) || TrimmedLine.StartsWith(TEXT("npc_blueprint:")))
			{
				CurrentDef.NPCClassPath = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("ability_configuration:")))
			{
				CurrentDef.AbilityConfiguration = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("activity_configuration:")))
			{
				CurrentDef.ActivityConfiguration = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("min_level:")))
			{
				CurrentDef.MinLevel = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("max_level:")))
			{
				CurrentDef.MaxLevel = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("allow_multiple_instances:")))
			{
				CurrentDef.bAllowMultipleInstances = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("is_vendor:")))
			{
				CurrentDef.bIsVendor = GetLineValue(TrimmedLine).ToBool();
			}
			// v4.8: UniqueNPCGUID for save system
			else if (TrimmedLine.StartsWith(TEXT("unique_npc_guid:")))
			{
				CurrentDef.UniqueNPCGUID = GetLineValue(TrimmedLine);
			}
			// v4.8: TriggerSets array (inherited from CharacterDefinition)
			else if (TrimmedLine.StartsWith(TEXT("trigger_sets:")))
			{
				// Parse inline array [TS_Set1, TS_Set2] or single value
				FString TriggerSetsValue = GetLineValue(TrimmedLine);
				if (TriggerSetsValue.StartsWith(TEXT("[")) && TriggerSetsValue.EndsWith(TEXT("]")))
				{
					TriggerSetsValue = TriggerSetsValue.Mid(1, TriggerSetsValue.Len() - 2);
					TArray<FString> TriggerSetsArray;
					TriggerSetsValue.ParseIntoArray(TriggerSetsArray, TEXT(","));
					for (FString& TriggerSet : TriggerSetsArray)
					{
						TriggerSet = TriggerSet.TrimStartAndEnd();
						if (!TriggerSet.IsEmpty())
						{
							CurrentDef.TriggerSets.Add(TriggerSet);
						}
					}
				}
				else if (!TriggerSetsValue.IsEmpty())
				{
					CurrentDef.TriggerSets.Add(TriggerSetsValue);
				}
			}
			// v3.3: Dialogue properties
			else if (TrimmedLine.StartsWith(TEXT("dialogue:")))
			{
				CurrentDef.Dialogue = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("tagged_dialogue_set:")))
			{
				CurrentDef.TaggedDialogueSet = GetLineValue(TrimmedLine);
			}
			// v3.3: Vendor properties
			else if (TrimmedLine.StartsWith(TEXT("trading_currency:")))
			{
				CurrentDef.TradingCurrency = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("buy_item_percentage:")))
			{
				CurrentDef.BuyItemPercentage = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("sell_item_percentage:")))
			{
				CurrentDef.SellItemPercentage = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("shop_name:")))
			{
				CurrentDef.ShopFriendlyName = GetLineValue(TrimmedLine);
			}
			// v3.3: CharacterDefinition inherited properties
			else if (TrimmedLine.StartsWith(TEXT("default_appearance:")))
			{
				CurrentDef.DefaultAppearance = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_currency:")))
			{
				CurrentDef.DefaultCurrency = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("attack_priority:")))
			{
				CurrentDef.AttackPriority = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("default_owned_tags:")))
			{
				// Parse inline array [Tag1, Tag2] or single value
				FString TagsValue = GetLineValue(TrimmedLine);
				if (TagsValue.StartsWith(TEXT("[")) && TagsValue.EndsWith(TEXT("]")))
				{
					TagsValue = TagsValue.Mid(1, TagsValue.Len() - 2);
					TArray<FString> Tags;
					TagsValue.ParseIntoArray(Tags, TEXT(","));
					for (FString& Tag : Tags)
					{
						Tag = Tag.TrimStartAndEnd();
						if (!Tag.IsEmpty())
						{
							CurrentDef.DefaultOwnedTags.Add(Tag);
						}
					}
				}
				else if (!TagsValue.IsEmpty())
				{
					CurrentDef.DefaultOwnedTags.Add(TagsValue);
				}
				bInOwnedTags = true;
				bInFactions = false;
				bInActivitySchedules = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("default_factions:")))
			{
				// Parse inline array [Faction1, Faction2] or single value
				FString FactionsValue = GetLineValue(TrimmedLine);
				if (FactionsValue.StartsWith(TEXT("[")) && FactionsValue.EndsWith(TEXT("]")))
				{
					FactionsValue = FactionsValue.Mid(1, FactionsValue.Len() - 2);
					TArray<FString> Factions;
					FactionsValue.ParseIntoArray(Factions, TEXT(","));
					for (FString& Faction : Factions)
					{
						Faction = Faction.TrimStartAndEnd();
						if (!Faction.IsEmpty())
						{
							CurrentDef.DefaultFactions.Add(Faction);
						}
					}
				}
				else if (!FactionsValue.IsEmpty())
				{
					CurrentDef.DefaultFactions.Add(FactionsValue);
				}
				bInFactions = true;
				bInOwnedTags = false;
				bInActivitySchedules = false;
			}
			// v3.6: Activity schedules array
			else if (TrimmedLine.StartsWith(TEXT("activity_schedules:")))
			{
				// Parse inline array [Schedule1, Schedule2] or start YAML list
				FString SchedulesValue = GetLineValue(TrimmedLine);
				if (SchedulesValue.StartsWith(TEXT("[")) && SchedulesValue.EndsWith(TEXT("]")))
				{
					SchedulesValue = SchedulesValue.Mid(1, SchedulesValue.Len() - 2);
					TArray<FString> Schedules;
					SchedulesValue.ParseIntoArray(Schedules, TEXT(","));
					for (FString& Schedule : Schedules)
					{
						Schedule = Schedule.TrimStartAndEnd();
						if (!Schedule.IsEmpty())
						{
							CurrentDef.ActivitySchedules.Add(Schedule);
						}
					}
				}
				else if (!SchedulesValue.IsEmpty())
				{
					CurrentDef.ActivitySchedules.Add(SchedulesValue);
				}
				bInActivitySchedules = true;
				bInOwnedTags = false;
				bInFactions = false;
				bInItemLoadoutCollections = false;
			}
			// v3.7: Auto-create flags for related assets
			else if (TrimmedLine.StartsWith(TEXT("auto_create_dialogue:")))
			{
				CurrentDef.bAutoCreateDialogue = GetLineValue(TrimmedLine).ToBool();
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("auto_create_tagged_dialogue:")))
			{
				CurrentDef.bAutoCreateTaggedDialogue = GetLineValue(TrimmedLine).ToBool();
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("auto_create_item_loadout:")))
			{
				CurrentDef.bAutoCreateItemLoadout = GetLineValue(TrimmedLine).ToBool();
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
			}
			// v3.7: Default item loadout collections array
			else if (TrimmedLine.StartsWith(TEXT("default_item_loadout_collections:")))
			{
				// Parse inline array [IC_1, IC_2] or start YAML list
				FString CollectionsValue = GetLineValue(TrimmedLine);
				if (CollectionsValue.StartsWith(TEXT("[")) && CollectionsValue.EndsWith(TEXT("]")))
				{
					CollectionsValue = CollectionsValue.Mid(1, CollectionsValue.Len() - 2);
					TArray<FString> Collections;
					CollectionsValue.ParseIntoArray(Collections, TEXT(","));
					for (FString& Collection : Collections)
					{
						Collection = Collection.TrimStartAndEnd();
						if (!Collection.IsEmpty())
						{
							CurrentDef.DefaultItemLoadoutCollections.Add(Collection);
						}
					}
				}
				else if (!CollectionsValue.IsEmpty())
				{
					CurrentDef.DefaultItemLoadoutCollections.Add(CollectionsValue);
				}
				bInItemLoadoutCollections = true;
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				// v3.9.5: Exit loot table parsing
				if (bInItemDef) SaveCurrentItemDef();
				if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();
				bInDefaultItemLoadout = false;
				bInTradingItemLoadout = false;
			}
			// v3.9.5: Default item loadout (full loot table roll array)
			else if (TrimmedLine.StartsWith(TEXT("default_item_loadout:")))
			{
				// Save any pending roll and start new loadout section
				if (bInItemDef) SaveCurrentItemDef();
				if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();
				bInDefaultItemLoadout = true;
				bInTradingItemLoadout = false;
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
			}
			// v3.9.5: Trading item loadout (vendor inventory)
			else if (TrimmedLine.StartsWith(TEXT("trading_item_loadout:")))
			{
				// Save any pending roll and start new loadout section
				if (bInItemDef) SaveCurrentItemDef();
				if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();
				bInTradingItemLoadout = true;
				bInDefaultItemLoadout = false;
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
			}
			// v3.9.5: Loot table roll properties (inside default_item_loadout or trading_item_loadout)
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("items:")))
			{
				// Start items array inside current roll
				if (bInItemDef) SaveCurrentItemDef();
				bInRollItems = true;
				bInRollItemCollections = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("item_collections:")))
			{
				// Start item collections array inside current roll
				if (bInItemDef) SaveCurrentItemDef();
				bInRollItemCollections = true;
				bInRollItems = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("table_to_roll:")))
			{
				if (bInItemDef) SaveCurrentItemDef();
				CurrentRoll.TableToRoll = GetLineValue(TrimmedLine);
				bInRollItems = false;
				bInRollItemCollections = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("num_rolls:")))
			{
				if (bInItemDef) SaveCurrentItemDef();
				CurrentRoll.NumRolls = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInRollItems = false;
				bInRollItemCollections = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("chance:")))
			{
				if (bInItemDef) SaveCurrentItemDef();
				CurrentRoll.Chance = FCString::Atof(*GetLineValue(TrimmedLine));
				bInRollItems = false;
				bInRollItemCollections = false;
			}
			// v3.9.5: Item definition properties (inside items array of a roll)
			else if (bInRollItems && TrimmedLine.StartsWith(TEXT("- item:")))
			{
				// New item definition - save previous if any
				if (bInItemDef) SaveCurrentItemDef();
				CurrentItemDef = FManifestItemWithQuantityDefinition();
				CurrentItemDef.Item = GetLineValue(TrimmedLine.Mid(2)); // Skip "- "
				bInItemDef = true;
			}
			else if (bInRollItems && bInItemDef && TrimmedLine.StartsWith(TEXT("quantity:")))
			{
				CurrentItemDef.Quantity = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			// v3.9.5: Handle new loot table roll start (e.g., "- items:" or "- item_collections:")
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("- items:")))
			{
				// Save previous roll if any, then start new roll
				if (bInItemDef) SaveCurrentItemDef();
				SaveCurrentRoll();
				bInRollItems = true;
				bInRollItemCollections = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("- item_collections:")))
			{
				// Save previous roll if any, then start new roll
				if (bInItemDef) SaveCurrentItemDef();
				SaveCurrentRoll();
				bInRollItemCollections = true;
				bInRollItems = false;
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("- chance:")))
			{
				// Shorthand: a roll starting directly with chance
				if (bInItemDef) SaveCurrentItemDef();
				SaveCurrentRoll();
				CurrentRoll.Chance = FCString::Atof(*GetLineValue(TrimmedLine.Mid(2)));
			}
			else if ((bInDefaultItemLoadout || bInTradingItemLoadout) && TrimmedLine.StartsWith(TEXT("- table_to_roll:")))
			{
				// Shorthand: a roll starting directly with table_to_roll
				if (bInItemDef) SaveCurrentItemDef();
				SaveCurrentRoll();
				CurrentRoll.TableToRoll = GetLineValue(TrimmedLine.Mid(2));
			}
			// v3.6: Handle YAML list items for arrays
			else if (TrimmedLine.StartsWith(TEXT("- ")) && !TrimmedLine.Contains(TEXT(":")))
			{
				FString ItemValue = TrimmedLine.Mid(2).TrimStartAndEnd();
				if (!ItemValue.IsEmpty())
				{
					if (bInActivitySchedules)
					{
						CurrentDef.ActivitySchedules.Add(ItemValue);
					}
					else if (bInOwnedTags)
					{
						CurrentDef.DefaultOwnedTags.Add(ItemValue);
					}
					else if (bInFactions)
					{
						CurrentDef.DefaultFactions.Add(ItemValue);
					}
					else if (bInItemLoadoutCollections)
					{
						CurrentDef.DefaultItemLoadoutCollections.Add(ItemValue);
					}
					// v3.9.5: Item collection inside a loot table roll
					else if (bInRollItemCollections)
					{
						CurrentRoll.ItemCollectionsToGrant.Add(ItemValue);
					}
				}
			}
			else
			{
				// Any other property resets array parsing state
				bInOwnedTags = false;
				bInFactions = false;
				bInActivitySchedules = false;
				bInItemLoadoutCollections = false;
				// v3.9.5: Reset loot table parsing if we see a non-loadout property
				// But only if we're not inside nested items/collections parsing
				if (!bInRollItems && !bInRollItemCollections)
				{
					if (bInItemDef) SaveCurrentItemDef();
					if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();
					bInDefaultItemLoadout = false;
					bInTradingItemLoadout = false;
				}
			}
		}

		LineIndex++;
	}

	// v3.9.5: Save any pending loot table roll at end of file
	if (bInItemDef) SaveCurrentItemDef();
	if (bInDefaultItemLoadout || bInTradingItemLoadout) SaveCurrentRoll();

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.NPCDefinitions.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseCharacterDefinitions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestCharacterDefinitionDefinition CurrentDef;
	bool bInItem = false;
	// v3.5: Array parsing states
	bool bInOwnedTags = false;
	bool bInFactions = false;
	bool bInTriggerSets = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.CharacterDefinitions.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.CharacterDefinitions.Add(CurrentDef);
			}
			CurrentDef = FManifestCharacterDefinitionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			// v3.5: Reset array states
			bInOwnedTags = false;
			bInFactions = false;
			bInTriggerSets = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInOwnedTags = false; bInFactions = false; bInTriggerSets = false;
			}
			// v3.5: Array properties
			else if (TrimmedLine.Equals(TEXT("default_owned_tags:")) || TrimmedLine.StartsWith(TEXT("default_owned_tags:")))
			{
				bInOwnedTags = true;
				bInFactions = false;
				bInTriggerSets = false;
				// Check for inline array format: [Tag1, Tag2]
				FString Value = GetLineValue(TrimmedLine);
				if (!Value.IsEmpty() && Value.StartsWith(TEXT("[")))
				{
					Value = Value.Mid(1);
					if (Value.EndsWith(TEXT("]"))) Value = Value.LeftChop(1);
					TArray<FString> Items;
					Value.ParseIntoArray(Items, TEXT(","));
					for (FString& Item : Items)
					{
						Item = Item.TrimStartAndEnd();
						if (!Item.IsEmpty()) CurrentDef.DefaultOwnedTags.Add(Item);
					}
					bInOwnedTags = false;
				}
			}
			else if (TrimmedLine.Equals(TEXT("default_factions:")) || TrimmedLine.StartsWith(TEXT("default_factions:")))
			{
				bInOwnedTags = false;
				bInFactions = true;
				bInTriggerSets = false;
				FString Value = GetLineValue(TrimmedLine);
				if (!Value.IsEmpty() && Value.StartsWith(TEXT("[")))
				{
					Value = Value.Mid(1);
					if (Value.EndsWith(TEXT("]"))) Value = Value.LeftChop(1);
					TArray<FString> Items;
					Value.ParseIntoArray(Items, TEXT(","));
					for (FString& Item : Items)
					{
						Item = Item.TrimStartAndEnd();
						if (!Item.IsEmpty()) CurrentDef.DefaultFactions.Add(Item);
					}
					bInFactions = false;
				}
			}
			else if (TrimmedLine.Equals(TEXT("trigger_sets:")) || TrimmedLine.StartsWith(TEXT("trigger_sets:")))
			{
				bInOwnedTags = false;
				bInFactions = false;
				bInTriggerSets = true;
			}
			else if (bInOwnedTags && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Tag = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Tag.Len() >= 2 && ((Tag.StartsWith(TEXT("\"")) && Tag.EndsWith(TEXT("\""))) ||
					(Tag.StartsWith(TEXT("'")) && Tag.EndsWith(TEXT("'")))))
				{
					Tag = Tag.Mid(1, Tag.Len() - 2);
				}
				if (!Tag.IsEmpty()) CurrentDef.DefaultOwnedTags.Add(Tag);
			}
			else if (bInFactions && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Faction = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (Faction.Len() >= 2 && ((Faction.StartsWith(TEXT("\"")) && Faction.EndsWith(TEXT("\""))) ||
					(Faction.StartsWith(TEXT("'")) && Faction.EndsWith(TEXT("'")))))
				{
					Faction = Faction.Mid(1, Faction.Len() - 2);
				}
				if (!Faction.IsEmpty()) CurrentDef.DefaultFactions.Add(Faction);
			}
			else if (bInTriggerSets && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString TriggerSet = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (TriggerSet.Len() >= 2 && ((TriggerSet.StartsWith(TEXT("\"")) && TriggerSet.EndsWith(TEXT("\""))) ||
					(TriggerSet.StartsWith(TEXT("'")) && TriggerSet.EndsWith(TEXT("'")))))
				{
					TriggerSet = TriggerSet.Mid(1, TriggerSet.Len() - 2);
				}
				if (!TriggerSet.IsEmpty()) CurrentDef.TriggerSets.Add(TriggerSet);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_currency:")))
			{
				CurrentDef.DefaultCurrency = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInOwnedTags = false; bInFactions = false; bInTriggerSets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("attack_priority:")))
			{
				CurrentDef.AttackPriority = FCString::Atof(*GetLineValue(TrimmedLine));
				bInOwnedTags = false; bInFactions = false; bInTriggerSets = false;
			}
			// v3.5: New properties
			else if (TrimmedLine.StartsWith(TEXT("default_appearance:")))
			{
				CurrentDef.DefaultAppearance = GetLineValue(TrimmedLine);
				bInOwnedTags = false; bInFactions = false; bInTriggerSets = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("ability_configuration:")))
			{
				CurrentDef.AbilityConfiguration = GetLineValue(TrimmedLine);
				bInOwnedTags = false; bInFactions = false; bInTriggerSets = false;
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.CharacterDefinitions.Add(CurrentDef);
	}
}

// v4.8.3: CharacterAppearance parser - creates UCharacterAppearance data assets
void FGasAbilityGeneratorParser::ParseCharacterAppearances(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestCharacterAppearanceDefinition CurrentDef;
	bool bInItem = false;
	bool bInMeshes = false;
	bool bInMeshList = false;
	FString CurrentMeshTag;
	bool bInScalars = false;
	bool bInVectors = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.CharacterAppearances.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.CharacterAppearances.Add(CurrentDef);
			}
			CurrentDef = FManifestCharacterAppearanceDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInMeshes = false;
			bInMeshList = false;
			bInScalars = false;
			bInVectors = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInMeshes = false; bInScalars = false; bInVectors = false;
			}
			else if (TrimmedLine.Equals(TEXT("meshes:")) || TrimmedLine.StartsWith(TEXT("meshes:")))
			{
				bInMeshes = true;
				bInMeshList = false;
				bInScalars = false;
				bInVectors = false;
			}
			else if (bInMeshes)
			{
				// Mesh tag with array, e.g., "Narrative.Equipment.Slot.Mesh.Body:"
				if (TrimmedLine.EndsWith(TEXT(":")))
				{
					CurrentMeshTag = TrimmedLine.LeftChop(1).TrimStartAndEnd();
					bInMeshList = true;
					if (!CurrentDef.Meshes.Contains(CurrentMeshTag))
					{
						CurrentDef.Meshes.Add(CurrentMeshTag, TArray<FString>());
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("- ")) && bInMeshList && !CurrentMeshTag.IsEmpty())
				{
					FString MeshPath = GetLineValue(TrimmedLine.Mid(2));
					if (!MeshPath.IsEmpty())
					{
						CurrentDef.Meshes[CurrentMeshTag].Add(MeshPath);
					}
				}
				else if (!TrimmedLine.StartsWith(TEXT("-")) && !TrimmedLine.EndsWith(TEXT(":")))
				{
					// End of meshes section
					bInMeshes = false;
					bInMeshList = false;
					LineIndex--;
					continue;
				}
			}
			else if (TrimmedLine.Equals(TEXT("scalar_values:")) || TrimmedLine.StartsWith(TEXT("scalar_values:")))
			{
				bInScalars = true;
				bInMeshes = false;
				bInVectors = false;
			}
			else if (bInScalars)
			{
				// Scalar entry: "Narrative.CharacterCreator.Scalars.Height: 0.5"
				if (TrimmedLine.Contains(TEXT(":")))
				{
					int32 ColonPos;
					if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
					{
						FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
						FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
						if (!Key.IsEmpty() && !Value.IsEmpty())
						{
							CurrentDef.ScalarValues.Add(Key, FCString::Atof(*Value));
						}
					}
				}
				else if (!TrimmedLine.IsEmpty())
				{
					bInScalars = false;
					LineIndex--;
					continue;
				}
			}
			else if (TrimmedLine.Equals(TEXT("vector_values:")) || TrimmedLine.StartsWith(TEXT("vector_values:")))
			{
				bInVectors = true;
				bInMeshes = false;
				bInScalars = false;
			}
			else if (bInVectors)
			{
				// Vector entry: "Narrative.CharacterCreator.Vectors.SkinColor: #FFC0A0"
				if (TrimmedLine.Contains(TEXT(":")))
				{
					int32 ColonPos;
					if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
					{
						FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
						FString Value = TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd();
						if (!Key.IsEmpty() && !Value.IsEmpty())
						{
							CurrentDef.VectorValues.Add(Key, Value);
						}
					}
				}
				else if (!TrimmedLine.IsEmpty())
				{
					bInVectors = false;
					LineIndex--;
					continue;
				}
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.CharacterAppearances.Add(CurrentDef);
	}
}

// v4.9: TriggerSet parser - creates UTriggerSet DataAssets with instanced triggers
void FGasAbilityGeneratorParser::ParseTriggerSets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestTriggerSetDefinition CurrentSet;
	FManifestTriggerDefinition CurrentTrigger;
	FManifestTriggerEventDefinition CurrentEvent;
	bool bInSet = false;
	bool bInTriggers = false;
	bool bInTrigger = false;
	bool bInEvents = false;
	bool bInEvent = false;
	bool bInProperties = false;
	int32 TriggersIndent = -1;
	int32 EventsIndent = -1;
	int32 PropertiesIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save any pending event
			if (bInEvent && !CurrentEvent.EventClass.IsEmpty())
			{
				CurrentTrigger.Events.Add(CurrentEvent);
			}
			// Save any pending trigger
			if (bInTrigger && !CurrentTrigger.TriggerClass.IsEmpty())
			{
				CurrentSet.Triggers.Add(CurrentTrigger);
			}
			// Save any pending set
			if (bInSet && !CurrentSet.Name.IsEmpty())
			{
				OutData.TriggerSets.Add(CurrentSet);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New set entry
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous event if any
			if (bInEvent && !CurrentEvent.EventClass.IsEmpty())
			{
				CurrentTrigger.Events.Add(CurrentEvent);
				CurrentEvent = FManifestTriggerEventDefinition();
				bInEvent = false;
			}
			// Save previous trigger if any
			if (bInTrigger && !CurrentTrigger.TriggerClass.IsEmpty())
			{
				CurrentSet.Triggers.Add(CurrentTrigger);
				CurrentTrigger = FManifestTriggerDefinition();
				bInTrigger = false;
			}
			// Save previous set if any
			if (bInSet && !CurrentSet.Name.IsEmpty())
			{
				OutData.TriggerSets.Add(CurrentSet);
			}

			CurrentSet = FManifestTriggerSetDefinition();
			CurrentSet.Name = GetLineValue(TrimmedLine);
			bInSet = true;
			bInTriggers = false;
			bInEvents = false;
			bInProperties = false;
		}
		else if (bInSet)
		{
			// Set-level properties
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentSet.Folder = StripYamlComment(GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.Equals(TEXT("triggers:")) || TrimmedLine.StartsWith(TEXT("triggers:")))
			{
				bInTriggers = true;
				bInEvents = false;
				bInProperties = false;
				TriggersIndent = CurrentIndent;
			}
			// Trigger array item
			else if (bInTriggers && TrimmedLine.StartsWith(TEXT("- trigger_class:")))
			{
				// Save previous event if any
				if (bInEvent && !CurrentEvent.EventClass.IsEmpty())
				{
					CurrentTrigger.Events.Add(CurrentEvent);
					CurrentEvent = FManifestTriggerEventDefinition();
					bInEvent = false;
				}
				// Save previous trigger if any
				if (bInTrigger && !CurrentTrigger.TriggerClass.IsEmpty())
				{
					CurrentSet.Triggers.Add(CurrentTrigger);
				}

				CurrentTrigger = FManifestTriggerDefinition();
				CurrentTrigger.TriggerClass = StripYamlComment(GetLineValue(TrimmedLine));
				bInTrigger = true;
				bInEvents = false;
				bInProperties = false;
			}
			else if (bInTrigger && !bInEvents && !bInProperties)
			{
				// Trigger-level properties
				if (TrimmedLine.StartsWith(TEXT("start_time:")))
				{
					CurrentTrigger.StartTime = FCString::Atof(*StripYamlComment(GetLineValue(TrimmedLine)));
				}
				else if (TrimmedLine.StartsWith(TEXT("end_time:")))
				{
					CurrentTrigger.EndTime = FCString::Atof(*StripYamlComment(GetLineValue(TrimmedLine)));
				}
				else if (TrimmedLine.Equals(TEXT("events:")) || TrimmedLine.StartsWith(TEXT("events:")))
				{
					bInEvents = true;
					bInProperties = false;
					EventsIndent = CurrentIndent;
				}
			}
			// Event array item
			else if (bInEvents && TrimmedLine.StartsWith(TEXT("- event_class:")))
			{
				// Save previous event if any
				if (bInEvent && !CurrentEvent.EventClass.IsEmpty())
				{
					CurrentTrigger.Events.Add(CurrentEvent);
				}

				CurrentEvent = FManifestTriggerEventDefinition();
				CurrentEvent.EventClass = StripYamlComment(GetLineValue(TrimmedLine));
				bInEvent = true;
				bInProperties = false;
			}
			else if (bInEvent && !bInProperties)
			{
				// Event-level properties
				if (TrimmedLine.StartsWith(TEXT("runtime:")))
				{
					CurrentEvent.Runtime = StripYamlComment(GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.Equals(TEXT("properties:")) || TrimmedLine.StartsWith(TEXT("properties:")))
				{
					bInProperties = true;
					PropertiesIndent = CurrentIndent;
				}
			}
			else if (bInProperties)
			{
				// Check if we should exit properties
				if (CurrentIndent <= PropertiesIndent && !TrimmedLine.IsEmpty())
				{
					bInProperties = false;
					LineIndex--;
					LineIndex++;
					continue;
				}
				// Parse property key: value
				if (TrimmedLine.Contains(TEXT(":")))
				{
					int32 ColonPos;
					if (TrimmedLine.FindChar(TEXT(':'), ColonPos))
					{
						FString Key = TrimmedLine.Left(ColonPos).TrimStartAndEnd();
						FString Value = StripYamlComment(TrimmedLine.Mid(ColonPos + 1).TrimStartAndEnd());
						if (!Key.IsEmpty() && !Value.IsEmpty())
						{
							CurrentEvent.Properties.Add(Key, Value);
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// Save any remaining items
	if (bInEvent && !CurrentEvent.EventClass.IsEmpty())
	{
		CurrentTrigger.Events.Add(CurrentEvent);
	}
	if (bInTrigger && !CurrentTrigger.TriggerClass.IsEmpty())
	{
		CurrentSet.Triggers.Add(CurrentTrigger);
	}
	if (bInSet && !CurrentSet.Name.IsEmpty())
	{
		OutData.TriggerSets.Add(CurrentSet);
	}
}

// v2.5.7: TaggedDialogueSet parser - creates UTaggedDialogueSet data assets
void FGasAbilityGeneratorParser::ParseTaggedDialogueSets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestTaggedDialogueSetDefinition CurrentSet;
	FManifestTaggedDialogueEntry CurrentDialogue;
	bool bInSet = false;
	bool bInDialogues = false;
	bool bInDialogueItem = false;
	int32 DialoguesIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save any pending dialogue
			if (bInDialogueItem && !CurrentDialogue.Tag.IsEmpty())
			{
				CurrentSet.Dialogues.Add(CurrentDialogue);
			}
			// Save any pending set
			if (bInSet && !CurrentSet.Name.IsEmpty())
			{
				OutData.TaggedDialogueSets.Add(CurrentSet);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New set entry
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous set if any
			if (bInDialogueItem && !CurrentDialogue.Tag.IsEmpty())
			{
				CurrentSet.Dialogues.Add(CurrentDialogue);
			}
			if (bInSet && !CurrentSet.Name.IsEmpty())
			{
				OutData.TaggedDialogueSets.Add(CurrentSet);
			}

			CurrentSet = FManifestTaggedDialogueSetDefinition();
			CurrentDialogue = FManifestTaggedDialogueEntry();
			CurrentSet.Name = GetLineValue(TrimmedLine.Mid(2));
			bInSet = true;
			bInDialogues = false;
			bInDialogueItem = false;
			DialoguesIndent = -1;
		}
		else if (bInSet)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentSet.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("dialogues:")))
			{
				bInDialogues = true;
				DialoguesIndent = CurrentIndent;
			}
			else if (bInDialogues)
			{
				// New dialogue entry
				if (TrimmedLine.StartsWith(TEXT("- tag:")))
				{
					// Save previous dialogue if any
					if (bInDialogueItem && !CurrentDialogue.Tag.IsEmpty())
					{
						CurrentSet.Dialogues.Add(CurrentDialogue);
					}
					CurrentDialogue = FManifestTaggedDialogueEntry();
					CurrentDialogue.Tag = GetLineValue(TrimmedLine.Mid(2));
					bInDialogueItem = true;
				}
				else if (bInDialogueItem)
				{
					if (TrimmedLine.StartsWith(TEXT("dialogue_class:")) || TrimmedLine.StartsWith(TEXT("dialogue:")))
					{
						CurrentDialogue.DialogueClass = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("cooldown:")))
					{
						CurrentDialogue.Cooldown = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("max_distance:")))
					{
						CurrentDialogue.MaxDistance = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("required_tags:")))
					{
						// Could be inline array or multi-line - handle simple case
						FString TagsValue = GetLineValue(TrimmedLine);
						if (!TagsValue.IsEmpty() && TagsValue != TEXT("[]"))
						{
							TagsValue.ParseIntoArray(CurrentDialogue.RequiredTags, TEXT(","));
							for (FString& Tag : CurrentDialogue.RequiredTags)
							{
								Tag = Tag.TrimStartAndEnd();
							}
						}
					}
					else if (TrimmedLine.StartsWith(TEXT("blocked_tags:")))
					{
						FString TagsValue = GetLineValue(TrimmedLine);
						if (!TagsValue.IsEmpty() && TagsValue != TEXT("[]"))
						{
							TagsValue.ParseIntoArray(CurrentDialogue.BlockedTags, TEXT(","));
							for (FString& Tag : CurrentDialogue.BlockedTags)
							{
								Tag = Tag.TrimStartAndEnd();
							}
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// Save any remaining entries
	if (bInDialogueItem && !CurrentDialogue.Tag.IsEmpty())
	{
		CurrentSet.Dialogues.Add(CurrentDialogue);
	}
	if (bInSet && !CurrentSet.Name.IsEmpty())
	{
		OutData.TaggedDialogueSets.Add(CurrentSet);
	}
}

// v3.9.10: Strip YAML inline comments from values
// Handles: "value  # comment" -> "value"
// Preserves: "value with # in quotes" (when quoted)
// Properly handles escape sequences inside quotes
FString FGasAbilityGeneratorParser::StripYamlComment(const FString& Value)
{
	FString In = Value.TrimStartAndEnd();

	bool bInSingle = false;
	bool bInDouble = false;
	bool bEscaped = false;

	for (int32 i = 0; i < In.Len(); ++i)
	{
		const TCHAR C = In[i];

		if (bEscaped)
		{
			bEscaped = false;
			continue;
		}

		if (C == TEXT('\\'))
		{
			// Only treat backslash as escape inside quotes
			if (bInSingle || bInDouble)
			{
				bEscaped = true;
			}
			continue;
		}

		if (C == TEXT('"') && !bInSingle)
		{
			bInDouble = !bInDouble;
			continue;
		}

		if (C == TEXT('\'') && !bInDouble)
		{
			bInSingle = !bInSingle;
			continue;
		}

		if (C == TEXT('#') && !bInSingle && !bInDouble)
		{
			// YAML inline comment starts here
			return In.Left(i).TrimEnd();
		}
	}

	return In.TrimEnd();
}

FString FGasAbilityGeneratorParser::GetLineValue(const FString& Line)
{
	int32 ColonIndex;
	if (!Line.FindChar(TEXT(':'), ColonIndex))
	{
		// v3.9.10: Strip comments from bare values too
		return StripYamlComment(Line);
	}

	FString Value = Line.Mid(ColonIndex + 1).TrimStartAndEnd();

	// Remove quotes if present
	if (Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")))
	{
		Value = Value.Mid(1, Value.Len() - 2);
	}
	else if (Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))
	{
		Value = Value.Mid(1, Value.Len() - 2);
	}

	// v3.9.10: Strip inline YAML comments
	return StripYamlComment(Value);
}

int32 FGasAbilityGeneratorParser::GetIndentLevel(const FString& Line)
{
	int32 Indent = 0;
	for (int32 i = 0; i < Line.Len(); i++)
	{
		if (Line[i] == TEXT(' '))
		{
			Indent++;
		}
		else if (Line[i] == TEXT('\t'))
		{
			Indent += 2; // Treat tab as 2 spaces
		}
		else
		{
			break;
		}
	}
	return Indent;
}

bool FGasAbilityGeneratorParser::IsArrayItem(const FString& Line)
{
	FString TrimmedLine = Line.TrimStart();
	return TrimmedLine.StartsWith(TEXT("- "));
}

FString FGasAbilityGeneratorParser::GetArrayItemValue(const FString& Line)
{
	FString TrimmedLine = Line.TrimStart();
	if (TrimmedLine.StartsWith(TEXT("- ")))
	{
		FString Value = TrimmedLine.Mid(2).TrimStartAndEnd();
		
		// Remove quotes if present
		if (Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")))
		{
			Value = Value.Mid(1, Value.Len() - 2);
		}
		else if (Value.StartsWith(TEXT("'")) && Value.EndsWith(TEXT("'")))
		{
			Value = Value.Mid(1, Value.Len() - 2);
		}
		
		return Value;
	}
	return FString();
}

bool FGasAbilityGeneratorParser::IsSectionHeader(const FString& Line, const FString& SectionName)
{
	FString TrimmedLine = Line.TrimStart();
	return TrimmedLine.Equals(SectionName) || TrimmedLine.StartsWith(SectionName);
}

bool FGasAbilityGeneratorParser::ShouldExitSection(const FString& Line, int32 SectionIndent)
{
	// Skip empty lines - don't exit section for them
	FString TrimmedLine = Line.TrimStart();
	if (TrimmedLine.IsEmpty())
	{
		return false;
	}
	
	// v2.0.9 FIX: Exit section if we hit a line with less or equal indent
	// that isn't an array item or comment
	int32 CurrentIndent = GetIndentLevel(Line);
	
	if (CurrentIndent <= SectionIndent && 
	    !TrimmedLine.IsEmpty() && 
	    !TrimmedLine.StartsWith(TEXT("#")))
	{
		// Check if it's a new section header (contains colon at end)
		if (TrimmedLine.Contains(TEXT(":")))
		{
			return true;
		}
	}
	
	return false;
}

// v2.6.10: Niagara System parser - creates UNiagaraSystem assets with enhanced properties
// v2.6.11: Added user_parameters parsing support
void FGasAbilityGeneratorParser::ParseNiagaraSystems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestNiagaraSystemDefinition CurrentDef;
	bool bInItem = false;
	bool bInEmitters = false;
	bool bInUserParameters = false;
	bool bInFXDescriptor = false;  // v2.9.0
	FManifestNiagaraUserParameter CurrentUserParam;
	int32 EmittersIndent = -1;
	int32 UserParamsIndent = -1;
	int32 FXDescriptorIndent = -1;  // v2.9.0
	// v4.9: Emitter-specific overrides
	bool bInEmitterOverrides = false;
	bool bInEmitterOverrideParams = false;
	FManifestNiagaraEmitterOverride CurrentEmitterOverride;
	int32 EmitterOverridesIndent = -1;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save any pending user parameter
			if (bInUserParameters && !CurrentUserParam.Name.IsEmpty())
			{
				CurrentDef.UserParameters.Add(CurrentUserParam);
			}
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("NS_")))
			{
				OutData.NiagaraSystems.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// v2.8.4: Exit user_parameters when we encounter a top-level item (at SectionIndent + 2)
		// This fixes the bug where subsequent NS_ entries were not parsed
		if (bInUserParameters && TrimmedLine.StartsWith(TEXT("- name:")) && CurrentIndent <= SectionIndent + 2)
		{
			// Save pending user parameter
			if (!CurrentUserParam.Name.IsEmpty())
			{
				CurrentDef.UserParameters.Add(CurrentUserParam);
				CurrentUserParam = FManifestNiagaraUserParameter();
			}
			bInUserParameters = false;
			UserParamsIndent = -1;
		}

		// New item entry - v2.6.13: only if not in user_parameters subsection
		if (!bInUserParameters && !bInEmitterOverrides && TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save any pending user parameter
			if (bInUserParameters && !CurrentUserParam.Name.IsEmpty())
			{
				CurrentDef.UserParameters.Add(CurrentUserParam);
			}
			// v4.9: Save any pending emitter override
			if (bInEmitterOverrides && !CurrentEmitterOverride.EmitterName.IsEmpty())
			{
				CurrentDef.EmitterOverrides.Add(CurrentEmitterOverride);
			}
			// Save previous system if any
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("NS_")))
			{
				OutData.NiagaraSystems.Add(CurrentDef);
			}

			CurrentDef = FManifestNiagaraSystemDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInEmitters = false;
			bInUserParameters = false;
			bInFXDescriptor = false;  // v2.9.0
			bInEmitterOverrides = false;  // v4.9
			bInEmitterOverrideParams = false;  // v4.9
			CurrentUserParam = FManifestNiagaraUserParameter();
			CurrentEmitterOverride = FManifestNiagaraEmitterOverride();  // v4.9
			EmittersIndent = -1;
			UserParamsIndent = -1;
			FXDescriptorIndent = -1;  // v2.9.0
			EmitterOverridesIndent = -1;  // v4.9
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("template_system:")) || TrimmedLine.StartsWith(TEXT("template:")))
			{
				CurrentDef.TemplateSystem = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			// v4.9: FX Preset reference
			else if (TrimmedLine.StartsWith(TEXT("preset:")))
			{
				CurrentDef.Preset = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("emitters:")))
			{
				bInEmitters = true;
				bInUserParameters = false;
				EmittersIndent = CurrentIndent;
			}
			else if (bInEmitters && TrimmedLine.StartsWith(TEXT("-")) && !bInUserParameters)
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString EmitterName = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				if (!EmitterName.IsEmpty())
				{
					CurrentDef.Emitters.Add(EmitterName);
				}
			}
			// v2.6.11: User parameters section
			else if (TrimmedLine.StartsWith(TEXT("user_parameters:")))
			{
				bInUserParameters = true;
				bInEmitters = false;
				UserParamsIndent = CurrentIndent;
				CurrentUserParam = FManifestNiagaraUserParameter();
			}
			// v2.6.11: User parameter item
			else if (bInUserParameters && TrimmedLine.StartsWith(TEXT("- name:")))
			{
				// Save previous parameter if any
				if (!CurrentUserParam.Name.IsEmpty())
				{
					CurrentDef.UserParameters.Add(CurrentUserParam);
				}
				CurrentUserParam = FManifestNiagaraUserParameter();
				CurrentUserParam.Name = GetLineValue(TrimmedLine.Mid(2));
			}
			else if (bInUserParameters && TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentUserParam.Type = GetLineValue(TrimmedLine);
			}
			else if (bInUserParameters && (TrimmedLine.StartsWith(TEXT("default:")) || TrimmedLine.StartsWith(TEXT("default_value:"))))
			{
				CurrentUserParam.DefaultValue = GetLineValue(TrimmedLine);
			}
			// v2.6.10: Warmup settings
			else if (TrimmedLine.StartsWith(TEXT("warmup_time:")))
			{
				CurrentDef.WarmupTime = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("warmup_tick_count:")))
			{
				CurrentDef.WarmupTickCount = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("warmup_tick_delta:")))
			{
				CurrentDef.WarmupTickDelta = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			// v2.6.10: Bounds settings
			else if (TrimmedLine.StartsWith(TEXT("fixed_bounds:")))
			{
				CurrentDef.bFixedBounds = GetLineValue(TrimmedLine).ToBool();
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("bounds_min:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				TArray<FString> Parts;
				Value.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.BoundsMin = FVector(
						FCString::Atof(*Parts[0].TrimStartAndEnd()),
						FCString::Atof(*Parts[1].TrimStartAndEnd()),
						FCString::Atof(*Parts[2].TrimStartAndEnd())
					);
				}
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("bounds_max:")))
			{
				FString Value = GetLineValue(TrimmedLine);
				TArray<FString> Parts;
				Value.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.BoundsMax = FVector(
						FCString::Atof(*Parts[0].TrimStartAndEnd()),
						FCString::Atof(*Parts[1].TrimStartAndEnd()),
						FCString::Atof(*Parts[2].TrimStartAndEnd())
					);
				}
				bInEmitters = false;
				bInUserParameters = false;
			}
			// v2.6.10: Determinism settings
			else if (TrimmedLine.StartsWith(TEXT("determinism:")) || TrimmedLine.StartsWith(TEXT("deterministic:")))
			{
				CurrentDef.bDeterminism = GetLineValue(TrimmedLine).ToBool();
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("random_seed:")))
			{
				CurrentDef.RandomSeed = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			// v2.6.10: Effect type settings
			else if (TrimmedLine.StartsWith(TEXT("effect_type:")))
			{
				CurrentDef.EffectType = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("pooling_method:")) || TrimmedLine.StartsWith(TEXT("pooling:")))
			{
				CurrentDef.PoolingMethod = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("max_pool_size:")) || TrimmedLine.StartsWith(TEXT("pool_size:")))
			{
				CurrentDef.MaxPoolSize = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
				bInFXDescriptor = false;
			}
			// v4.9: LOD/Scalability settings
			else if (TrimmedLine.StartsWith(TEXT("cull_distance_low:")))
			{
				CurrentDef.CullDistanceLow = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_distance_medium:")))
			{
				CurrentDef.CullDistanceMedium = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_distance_high:")))
			{
				CurrentDef.CullDistanceHigh = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_distance_epic:")))
			{
				CurrentDef.CullDistanceEpic = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_distance_cinematic:")))
			{
				CurrentDef.CullDistanceCinematic = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("cull_distance:")) || TrimmedLine.StartsWith(TEXT("cull_max_distance:")))
			{
				CurrentDef.CullMaxDistance = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("significance_distance:")))
			{
				CurrentDef.SignificanceDistance = FCString::Atof(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("max_particle_budget:")) || TrimmedLine.StartsWith(TEXT("particle_budget:")))
			{
				CurrentDef.MaxParticleBudget = FCString::Atoi(*GetLineValue(TrimmedLine));
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("scalability_mode:")) || TrimmedLine.StartsWith(TEXT("scalability:")))
			{
				CurrentDef.ScalabilityMode = GetLineValue(TrimmedLine);
				bInEmitters = false;
				bInUserParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("allow_culling_for_local_players:")))
			{
				FString Value = GetLineValue(TrimmedLine).ToLower();
				CurrentDef.bAllowCullingForLocalPlayers = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
				bInEmitters = false;
				bInUserParameters = false;
			}
			// v2.9.0: FX Descriptor section for data-driven Niagara parameter binding
			else if (TrimmedLine.StartsWith(TEXT("fx_descriptor:")))
			{
				bInFXDescriptor = true;
				bInEmitters = false;
				bInUserParameters = false;
				FXDescriptorIndent = CurrentIndent;
			}
			// v2.9.0: FX Descriptor fields
			else if (bInFXDescriptor)
			{
				// Emitter toggles
				if (TrimmedLine.StartsWith(TEXT("particles_enabled:")))
				{
					CurrentDef.FXDescriptor.bParticlesEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("burst_enabled:")))
				{
					CurrentDef.FXDescriptor.bBurstEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("beam_enabled:")))
				{
					CurrentDef.FXDescriptor.bBeamEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("ribbon_enabled:")))
				{
					CurrentDef.FXDescriptor.bRibbonEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("light_enabled:")))
				{
					CurrentDef.FXDescriptor.bLightEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("smoke_enabled:")))
				{
					CurrentDef.FXDescriptor.bSmokeEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("spark_enabled:")) || TrimmedLine.StartsWith(TEXT("sparks_enabled:")))
				{
					CurrentDef.FXDescriptor.bSparkEnabled = GetLineValue(TrimmedLine).ToBool();
				}
				// Core emission
				else if (TrimmedLine.StartsWith(TEXT("spawn_rate:")))
				{
					CurrentDef.FXDescriptor.SpawnRate = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("lifetime_min:")))
				{
					CurrentDef.FXDescriptor.LifetimeMin = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("lifetime_max:")))
				{
					CurrentDef.FXDescriptor.LifetimeMax = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("lifetime:")))
				{
					// Parse [min, max] format
					FString Value = GetLineValue(TrimmedLine);
					Value.RemoveFromStart(TEXT("["));
					Value.RemoveFromEnd(TEXT("]"));
					TArray<FString> Parts;
					Value.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 2)
					{
						CurrentDef.FXDescriptor.LifetimeMin = FCString::Atof(*Parts[0].TrimStartAndEnd());
						CurrentDef.FXDescriptor.LifetimeMax = FCString::Atof(*Parts[1].TrimStartAndEnd());
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("max_particles:")))
				{
					CurrentDef.FXDescriptor.MaxParticles = FCString::Atoi(*GetLineValue(TrimmedLine));
				}
				// Appearance
				else if (TrimmedLine.StartsWith(TEXT("color:")))
				{
					// Parse [R, G, B, A] format
					FString Value = GetLineValue(TrimmedLine);
					Value.RemoveFromStart(TEXT("["));
					Value.RemoveFromEnd(TEXT("]"));
					TArray<FString> Parts;
					Value.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 3)
					{
						CurrentDef.FXDescriptor.Color.R = FCString::Atof(*Parts[0].TrimStartAndEnd());
						CurrentDef.FXDescriptor.Color.G = FCString::Atof(*Parts[1].TrimStartAndEnd());
						CurrentDef.FXDescriptor.Color.B = FCString::Atof(*Parts[2].TrimStartAndEnd());
						CurrentDef.FXDescriptor.Color.A = Parts.Num() >= 4 ? FCString::Atof(*Parts[3].TrimStartAndEnd()) : 1.0f;
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("size_min:")))
				{
					CurrentDef.FXDescriptor.SizeMin = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("size_max:")))
				{
					CurrentDef.FXDescriptor.SizeMax = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("size:")))
				{
					// Parse [min, max] format
					FString Value = GetLineValue(TrimmedLine);
					Value.RemoveFromStart(TEXT("["));
					Value.RemoveFromEnd(TEXT("]"));
					TArray<FString> Parts;
					Value.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 2)
					{
						CurrentDef.FXDescriptor.SizeMin = FCString::Atof(*Parts[0].TrimStartAndEnd());
						CurrentDef.FXDescriptor.SizeMax = FCString::Atof(*Parts[1].TrimStartAndEnd());
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("opacity:")))
				{
					CurrentDef.FXDescriptor.Opacity = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("emissive:")))
				{
					CurrentDef.FXDescriptor.Emissive = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// Motion
				else if (TrimmedLine.StartsWith(TEXT("initial_velocity:")))
				{
					// Parse [X, Y, Z] format
					FString Value = GetLineValue(TrimmedLine);
					Value.RemoveFromStart(TEXT("["));
					Value.RemoveFromEnd(TEXT("]"));
					TArray<FString> Parts;
					Value.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 3)
					{
						CurrentDef.FXDescriptor.InitialVelocity.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
						CurrentDef.FXDescriptor.InitialVelocity.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
						CurrentDef.FXDescriptor.InitialVelocity.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
					}
				}
				else if (TrimmedLine.StartsWith(TEXT("noise_strength:")))
				{
					CurrentDef.FXDescriptor.NoiseStrength = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("gravity_scale:")))
				{
					CurrentDef.FXDescriptor.GravityScale = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// Beam-specific
				else if (TrimmedLine.StartsWith(TEXT("beam_length:")))
				{
					CurrentDef.FXDescriptor.BeamLength = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("beam_width:")))
				{
					CurrentDef.FXDescriptor.BeamWidth = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// Ribbon-specific
				else if (TrimmedLine.StartsWith(TEXT("ribbon_width:")))
				{
					CurrentDef.FXDescriptor.RibbonWidth = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// Light-specific
				else if (TrimmedLine.StartsWith(TEXT("light_intensity:")))
				{
					CurrentDef.FXDescriptor.LightIntensity = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("light_radius:")))
				{
					CurrentDef.FXDescriptor.LightRadius = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				// LOD
				else if (TrimmedLine.StartsWith(TEXT("cull_distance:")))
				{
					CurrentDef.FXDescriptor.CullDistance = FCString::Atof(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("lod_level:")))
				{
					CurrentDef.FXDescriptor.LODLevel = FCString::Atoi(*GetLineValue(TrimmedLine));
				}
			}
			// v4.9: Emitter overrides section
			else if (TrimmedLine.StartsWith(TEXT("emitter_overrides:")))
			{
				bInEmitterOverrides = true;
				bInEmitterOverrideParams = false;
				bInEmitters = false;
				bInUserParameters = false;
				bInFXDescriptor = false;
				EmitterOverridesIndent = CurrentIndent;
				CurrentEmitterOverride = FManifestNiagaraEmitterOverride();
			}
			else if (bInEmitterOverrides)
			{
				// New emitter override entry
				if (TrimmedLine.StartsWith(TEXT("- emitter:")) || TrimmedLine.StartsWith(TEXT("- name:")))
				{
					// Save previous emitter override if valid
					if (!CurrentEmitterOverride.EmitterName.IsEmpty())
					{
						CurrentDef.EmitterOverrides.Add(CurrentEmitterOverride);
					}
					CurrentEmitterOverride = FManifestNiagaraEmitterOverride();
					CurrentEmitterOverride.EmitterName = GetLineValue(TrimmedLine.Mid(2));
					bInEmitterOverrideParams = false;
				}
				else if (TrimmedLine.StartsWith(TEXT("enabled:")))
				{
					FString Value = GetLineValue(TrimmedLine).ToLower();
					CurrentEmitterOverride.bEnabled = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
					CurrentEmitterOverride.bHasEnabled = true; // v4.11: Mark as explicitly specified
				}
				// v4.11: Structural enable - calls SetIsEnabled() on emitter handle (triggers recompile)
				else if (TrimmedLine.StartsWith(TEXT("structural_enabled:")))
				{
					FString Value = GetLineValue(TrimmedLine).ToLower();
					CurrentEmitterOverride.bStructuralEnabled = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
					CurrentEmitterOverride.bHasStructuralEnabled = true; // v4.11: Mark as explicitly specified
				}
				else if (TrimmedLine.StartsWith(TEXT("parameters:")))
				{
					bInEmitterOverrideParams = true;
				}
				else if (bInEmitterOverrideParams && TrimmedLine.Contains(TEXT(":")))
				{
					// Parse parameter key: value pairs
					int32 ColonIdx;
					if (TrimmedLine.FindChar(TEXT(':'), ColonIdx))
					{
						FString ParamName = TrimmedLine.Left(ColonIdx).TrimEnd();
						FString ParamValue = TrimmedLine.Mid(ColonIdx + 1).TrimStart();
						// Strip quotes if present
						if (ParamValue.StartsWith(TEXT("\"")) && ParamValue.EndsWith(TEXT("\"")))
						{
							ParamValue = ParamValue.Mid(1, ParamValue.Len() - 2);
						}
						if (!ParamName.IsEmpty())
						{
							CurrentEmitterOverride.Parameters.Add(ParamName, ParamValue);
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// v2.6.11: Save any pending user parameter before exiting
	if (bInUserParameters && !CurrentUserParam.Name.IsEmpty())
	{
		CurrentDef.UserParameters.Add(CurrentUserParam);
	}

	// v4.9: Save any pending emitter override
	if (bInEmitterOverrides && !CurrentEmitterOverride.EmitterName.IsEmpty())
	{
		CurrentDef.EmitterOverrides.Add(CurrentEmitterOverride);
	}

	// Save any remaining entry
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("NS_")))
	{
		OutData.NiagaraSystems.Add(CurrentDef);
	}
}

// ============================================================================
// v4.9: FX Preset Parsing
// ============================================================================

void FGasAbilityGeneratorParser::ParseFXPresets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestFXPresetDefinition CurrentDef;
	bool bInItem = false;
	bool bInParameters = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (ShouldExitSection(Line, SectionIndent))
		{
			// Save pending item - accept FX_ or Preset_ prefix, or any name
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.FXPresets.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// New preset item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous preset if valid
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.FXPresets.Add(CurrentDef);
			}
			CurrentDef = FManifestFXPresetDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(7));
			bInItem = true;
			bInParameters = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
				bInParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("base_preset:")) || TrimmedLine.StartsWith(TEXT("base:")))
			{
				CurrentDef.BasePreset = GetLineValue(TrimmedLine);
				bInParameters = false;
			}
			else if (TrimmedLine.StartsWith(TEXT("parameters:")))
			{
				bInParameters = true;
			}
			else if (bInParameters && TrimmedLine.Contains(TEXT(":")))
			{
				// Parse parameter key: value pairs
				int32 ColonIdx;
				if (TrimmedLine.FindChar(TEXT(':'), ColonIdx))
				{
					FString ParamName = TrimmedLine.Left(ColonIdx).TrimEnd();
					FString ParamValue = TrimmedLine.Mid(ColonIdx + 1).TrimStart();
					// Strip quotes if present
					if (ParamValue.StartsWith(TEXT("\"")) && ParamValue.EndsWith(TEXT("\"")))
					{
						ParamValue = ParamValue.Mid(1, ParamValue.Len() - 2);
					}
					if (!ParamName.IsEmpty())
					{
						CurrentDef.Parameters.Add(ParamName, ParamValue);
					}
				}
			}
		}

		LineIndex++;
	}

	// Save any remaining entry
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.FXPresets.Add(CurrentDef);
	}
}

// ============================================================================
// v3.9: NPC Pipeline Parsing
// ============================================================================

void FGasAbilityGeneratorParser::ParseActivitySchedules(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestActivityScheduleDefinition CurrentDef;
	FManifestScheduledBehaviorDefinition CurrentBehavior;
	bool bInItem = false;
	bool bInBehaviors = false;
	bool bInBehavior = false;

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for end of section
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				if (bInBehavior && !CurrentBehavior.GoalClass.IsEmpty())
				{
					CurrentDef.Behaviors.Add(CurrentBehavior);
				}
				OutData.ActivitySchedules.Add(CurrentDef);
			}

			CurrentDef = FManifestActivityScheduleDefinition();
			CurrentBehavior = FManifestScheduledBehaviorDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine);
			bInItem = true;
			bInBehaviors = false;
			bInBehavior = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("behaviors:")))
			{
				bInBehaviors = true;
				bInBehavior = false;
			}
			else if (bInBehaviors)
			{
				if (TrimmedLine.StartsWith(TEXT("- time:")))
				{
					// Save previous behavior
					if (bInBehavior && !CurrentBehavior.GoalClass.IsEmpty())
					{
						CurrentDef.Behaviors.Add(CurrentBehavior);
					}
					CurrentBehavior = FManifestScheduledBehaviorDefinition();
					bInBehavior = true;

					// Parse time array [start, end]
					FString TimeStr = GetLineValue(TrimmedLine);
					TimeStr = TimeStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
					TArray<FString> Times;
					TimeStr.ParseIntoArray(Times, TEXT(","));
					if (Times.Num() >= 2)
					{
						CurrentBehavior.StartTime = FCString::Atof(*Times[0].TrimStartAndEnd());
						CurrentBehavior.EndTime = FCString::Atof(*Times[1].TrimStartAndEnd());
					}
				}
				else if (bInBehavior)
				{
					if (TrimmedLine.StartsWith(TEXT("goal:")))
					{
						CurrentBehavior.GoalClass = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("score:")))
					{
						CurrentBehavior.ScoreOverride = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("location:")))
					{
						CurrentBehavior.Location = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("reselect:")))
					{
						CurrentBehavior.bReselect = GetLineValue(TrimmedLine).ToBool();
					}
				}
			}
		}

		LineIndex++;
	}

	// Save last item
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		if (bInBehavior && !CurrentBehavior.GoalClass.IsEmpty())
		{
			CurrentDef.Behaviors.Add(CurrentBehavior);
		}
		OutData.ActivitySchedules.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseGoalItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestGoalItemDefinition CurrentDef;
	bool bInItem = false;
	bool bInOwnedTags = false;
	bool bInBlockTags = false;
	bool bInRequireTags = false;

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for end of section
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New item
		if (TrimmedLine.StartsWith(TEXT("- name:")) || TrimmedLine.StartsWith(TEXT("- id:")))
		{
			// Save previous
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.GoalItems.Add(CurrentDef);
			}

			CurrentDef = FManifestGoalItemDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine);
			bInItem = true;
			bInOwnedTags = false;
			bInBlockTags = false;
			bInRequireTags = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("parent_class:")) || TrimmedLine.StartsWith(TEXT("base_class:")))
			{
				CurrentDef.ParentClass = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_score:")))
			{
				CurrentDef.DefaultScore = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("goal_lifetime:")) || TrimmedLine.StartsWith(TEXT("lifetime:")))
			{
				CurrentDef.GoalLifetime = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("remove_on_succeeded:")) || TrimmedLine.StartsWith(TEXT("remove_on_success:")))
			{
				CurrentDef.bRemoveOnSucceeded = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("save_goal:")))
			{
				CurrentDef.bSaveGoal = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("owned_tags:")))
			{
				bInOwnedTags = true;
				bInBlockTags = false;
				bInRequireTags = false;
				// Check for inline array format [Tag1, Tag2]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					FString ArrayContent = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Tags;
					ArrayContent.ParseIntoArray(Tags, TEXT(","));
					for (FString& Tag : Tags)
					{
						CurrentDef.OwnedTags.Add(Tag.TrimStartAndEnd());
					}
					bInOwnedTags = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("block_tags:")))
			{
				bInOwnedTags = false;
				bInBlockTags = true;
				bInRequireTags = false;
				// Check for inline array format [Tag1, Tag2]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					FString ArrayContent = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Tags;
					ArrayContent.ParseIntoArray(Tags, TEXT(","));
					for (FString& Tag : Tags)
					{
						CurrentDef.BlockTags.Add(Tag.TrimStartAndEnd());
					}
					bInBlockTags = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("require_tags:")))
			{
				bInOwnedTags = false;
				bInBlockTags = false;
				bInRequireTags = true;
				// Check for inline array format [Tag1, Tag2]
				FString Value = GetLineValue(TrimmedLine);
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					FString ArrayContent = Value.Mid(1, Value.Len() - 2);
					TArray<FString> Tags;
					ArrayContent.ParseIntoArray(Tags, TEXT(","));
					for (FString& Tag : Tags)
					{
						CurrentDef.RequireTags.Add(Tag.TrimStartAndEnd());
					}
					bInRequireTags = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("- ")) && !TrimmedLine.Contains(TEXT(":")))
			{
				FString Tag = TrimmedLine.RightChop(2).TrimStartAndEnd();
				if (bInOwnedTags)
				{
					CurrentDef.OwnedTags.Add(Tag);
				}
				else if (bInBlockTags)
				{
					CurrentDef.BlockTags.Add(Tag);
				}
				else if (bInRequireTags)
				{
					CurrentDef.RequireTags.Add(Tag);
				}
			}
		}

		LineIndex++;
	}

	// Save last item
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.GoalItems.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseQuests(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	// v3.9.4: Updated parser for nested branches-in-states structure
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestQuestDefinition CurrentQuest;
	FManifestQuestStateDefinition CurrentState;
	FManifestQuestBranchDefinition CurrentBranch;
	FManifestQuestTaskDefinition CurrentTask;
	FManifestDialogueEventDefinition CurrentEvent;

	bool bInQuest = false;
	bool bInStates = false;
	bool bInState = false;
	bool bInStateBranches = false;  // Branches inside state
	bool bInBranch = false;
	bool bInTasks = false;
	bool bInTask = false;
	bool bInStateEvents = false;
	bool bInBranchEvents = false;
	bool bInEvent = false;
	bool bInEventProperties = false;
	bool bInQuestRewards = false;   // v3.9.6: Rewards section
	bool bInRewardItems = false;    // v3.9.6: Items in rewards
	bool bInDialoguePlayParams = false;  // v4.2: Dialogue play params section
	bool bInRequirements = false;   // v4.3: Requirements section
	bool bInRequirement = false;    // v4.3: Inside a requirement item
	bool bInRequirementProperties = false;  // v4.3: Inside requirement properties
	FManifestQuestRequirementDefinition CurrentRequirement;  // v4.3: Current requirement being parsed

	auto SaveCurrentTask = [&]()
	{
		if (!CurrentTask.TaskClass.IsEmpty())
		{
			CurrentBranch.Tasks.Add(CurrentTask);
			CurrentTask = FManifestQuestTaskDefinition();
		}
	};

	auto SaveCurrentEvent = [&]()
	{
		if (!CurrentEvent.Type.IsEmpty())
		{
			if (bInBranchEvents)
			{
				CurrentBranch.Events.Add(CurrentEvent);
			}
			else if (bInStateEvents)
			{
				CurrentState.Events.Add(CurrentEvent);
			}
			CurrentEvent = FManifestDialogueEventDefinition();
		}
	};

	// v4.3: Save current requirement
	auto SaveCurrentRequirement = [&]()
	{
		if (!CurrentRequirement.Type.IsEmpty())
		{
			CurrentQuest.Requirements.Add(CurrentRequirement);
			CurrentRequirement = FManifestQuestRequirementDefinition();
		}
	};

	auto SaveCurrentBranch = [&]()
	{
		SaveCurrentTask();
		SaveCurrentEvent();
		if (!CurrentBranch.DestinationState.IsEmpty() || CurrentBranch.Tasks.Num() > 0)
		{
			CurrentState.Branches.Add(CurrentBranch);
			CurrentBranch = FManifestQuestBranchDefinition();
		}
	};

	auto SaveCurrentState = [&]()
	{
		SaveCurrentBranch();
		if (!CurrentState.Id.IsEmpty())
		{
			CurrentQuest.States.Add(CurrentState);
			CurrentState = FManifestQuestStateDefinition();
		}
	};

	auto SaveCurrentQuest = [&]()
	{
		SaveCurrentState();
		SaveCurrentRequirement();  // v4.3: Save any pending requirement
		if (!CurrentQuest.Name.IsEmpty())
		{
			OutData.Quests.Add(CurrentQuest);
			CurrentQuest = FManifestQuestDefinition();
		}
	};

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// End of section
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New quest item
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			SaveCurrentQuest();
			CurrentQuest.Name = GetLineValue(TrimmedLine);
			bInQuest = true;
			bInStates = false;
			bInState = false;
			bInStateBranches = false;
			bInBranch = false;
			bInTasks = false;
			bInTask = false;
			bInStateEvents = false;
			bInBranchEvents = false;
			bInEvent = false;
			bInEventProperties = false;
		}
		else if (bInQuest)
		{
			// Quest-level properties
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentQuest.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("quest_name:")))
			{
				CurrentQuest.QuestName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("quest_description:")))
			{
				CurrentQuest.QuestDescription = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("tracked:")))
			{
				CurrentQuest.bTracked = GetLineValue(TrimmedLine).ToBool();
			}
			// v4.1: Quest visibility and dialogue control
			else if (TrimmedLine.StartsWith(TEXT("hidden:")))
			{
				CurrentQuest.bHidden = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("resume_dialogue_after_load:")))
			{
				CurrentQuest.bResumeDialogueAfterLoad = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("dialogue:")))
			{
				CurrentQuest.Dialogue = GetLineValue(TrimmedLine);
			}
			// v4.2: Dialogue play params section
			else if (TrimmedLine.Equals(TEXT("dialogue_play_params:")) || TrimmedLine.StartsWith(TEXT("dialogue_play_params:")))
			{
				bInDialoguePlayParams = true;
			}
			else if (bInDialoguePlayParams && !bInStates)
			{
				if (TrimmedLine.StartsWith(TEXT("start_from_id:")))
				{
					CurrentQuest.DialoguePlayParams.StartFromID = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("priority:")))
				{
					CurrentQuest.DialoguePlayParams.Priority = FCString::Atoi(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("free_movement:")))
				{
					CurrentQuest.DialoguePlayParams.bOverride_bFreeMovement = true;
					CurrentQuest.DialoguePlayParams.bFreeMovement = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("stop_movement:")))
				{
					CurrentQuest.DialoguePlayParams.bOverride_bStopMovement = true;
					CurrentQuest.DialoguePlayParams.bStopMovement = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("unskippable:")))
				{
					CurrentQuest.DialoguePlayParams.bOverride_bUnskippable = true;
					CurrentQuest.DialoguePlayParams.bUnskippable = GetLineValue(TrimmedLine).ToBool();
				}
				else if (TrimmedLine.StartsWith(TEXT("can_be_exited:")))
				{
					CurrentQuest.DialoguePlayParams.bOverride_bCanBeExited = true;
					CurrentQuest.DialoguePlayParams.bCanBeExited = GetLineValue(TrimmedLine).ToBool();
				}
				else if (!TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":")))
				{
					// End of dialogue_play_params section (another field started)
					bInDialoguePlayParams = false;
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("start_state:")))
			{
				CurrentQuest.StartState = GetLineValue(TrimmedLine);
				bInDialoguePlayParams = false;
			}
			// v3.9.6: Questgiver
			else if (TrimmedLine.StartsWith(TEXT("questgiver:")))
			{
				CurrentQuest.Questgiver = GetLineValue(TrimmedLine);
			}
			// v3.9.6: Rewards section
			else if (TrimmedLine.StartsWith(TEXT("rewards:")))
			{
				bInQuestRewards = true;
				bInRewardItems = false;
			}
			else if (bInQuestRewards && !bInStates)
			{
				if (TrimmedLine.StartsWith(TEXT("currency:")))
				{
					CurrentQuest.Rewards.Currency = FCString::Atoi(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("xp:")))
				{
					CurrentQuest.Rewards.XP = FCString::Atoi(*GetLineValue(TrimmedLine));
				}
				else if (TrimmedLine.StartsWith(TEXT("items:")))
				{
					bInRewardItems = true;
				}
				else if (bInRewardItems)
				{
					if (TrimmedLine.StartsWith(TEXT("- item:")))
					{
						CurrentQuest.Rewards.Items.Add(GetLineValue(TrimmedLine));
						CurrentQuest.Rewards.ItemQuantities.Add(1); // Default quantity
					}
					else if (TrimmedLine.StartsWith(TEXT("- ")) && !TrimmedLine.Contains(TEXT(":")))
					{
						// Simple item name
						FString ItemName = TrimmedLine.Mid(2).TrimStart();
						CurrentQuest.Rewards.Items.Add(ItemName);
						CurrentQuest.Rewards.ItemQuantities.Add(1);
					}
					else if (TrimmedLine.StartsWith(TEXT("quantity:")) && CurrentQuest.Rewards.Items.Num() > 0)
					{
						// Update the last item's quantity
						CurrentQuest.Rewards.ItemQuantities[CurrentQuest.Rewards.ItemQuantities.Num() - 1] =
							FCString::Atoi(*GetLineValue(TrimmedLine));
					}
				}
			}
			// v4.3: Requirements section
			else if (TrimmedLine.StartsWith(TEXT("requirements:")))
			{
				bInRequirements = true;
				bInRequirement = false;
				bInRequirementProperties = false;
				bInQuestRewards = false;
				bInRewardItems = false;
			}
			else if (bInRequirements && !bInStates)
			{
				if (TrimmedLine.StartsWith(TEXT("- type:")) || TrimmedLine.StartsWith(TEXT("- class:")))
				{
					SaveCurrentRequirement();
					CurrentRequirement.Type = GetLineValue(TrimmedLine);
					bInRequirement = true;
					bInRequirementProperties = false;
				}
				else if (bInRequirement)
				{
					if (TrimmedLine.StartsWith(TEXT("type:")) || TrimmedLine.StartsWith(TEXT("class:")))
					{
						CurrentRequirement.Type = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("properties:")))
					{
						bInRequirementProperties = true;
					}
					else if (bInRequirementProperties || (!TrimmedLine.StartsWith(TEXT("-")) && TrimmedLine.Contains(TEXT(":"))))
					{
						// Parse property
						FString PropKey, PropValue;
						if (TrimmedLine.Split(TEXT(":"), &PropKey, &PropValue))
						{
							PropKey = PropKey.TrimStart().TrimEnd();
							PropValue = PropValue.TrimStart().TrimEnd().TrimQuotes();
							if (!PropKey.IsEmpty() && !PropKey.StartsWith(TEXT("-")))
							{
								CurrentRequirement.Properties.Add(PropKey, PropValue);
							}
						}
					}
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("states:")))
			{
				SaveCurrentRequirement();  // v4.3: Save any pending requirement
				bInStates = true;
				bInState = false;
				bInStateBranches = false;
				bInStateEvents = false;
				bInBranchEvents = false;
				bInQuestRewards = false;  // v3.9.6: Exit rewards section
				bInRewardItems = false;
				bInRequirements = false;  // v4.3: Exit requirements section
				bInRequirement = false;
			}
			else if (bInStates)
			{
				// State parsing
				if (TrimmedLine.StartsWith(TEXT("- id:")))
				{
					SaveCurrentState();
					CurrentState.Id = GetLineValue(TrimmedLine);
					bInState = true;
					bInStateBranches = false;
					bInBranch = false;
					bInTasks = false;
					bInTask = false;
					bInStateEvents = false;
					bInBranchEvents = false;
					bInEvent = false;
				}
				else if (bInState)
				{
					if (TrimmedLine.StartsWith(TEXT("description:")) && !bInBranch && !bInEvent)
					{
						CurrentState.Description = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("type:")))
					{
						CurrentState.Type = GetLineValue(TrimmedLine);
					}
					// v4.2: State custom event callback
					else if ((TrimmedLine.StartsWith(TEXT("on_entered_func_name:")) || TrimmedLine.StartsWith(TEXT("on_entered_func:"))) && !bInBranch)
					{
						CurrentState.OnEnteredFuncName = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("branches:")) && !bInTask)
					{
						SaveCurrentEvent();
						bInStateBranches = true;
						bInBranch = false;
						bInStateEvents = false;
						bInBranchEvents = false;
						bInEvent = false;
					}
					else if (TrimmedLine.StartsWith(TEXT("events:")) && !bInStateBranches)
					{
						bInStateEvents = true;
						bInBranchEvents = false;
						bInEvent = false;
					}
					else if (bInStateBranches)
					{
						// Branch parsing (inside state)
						if (TrimmedLine.StartsWith(TEXT("- destination:")) || TrimmedLine.StartsWith(TEXT("- id:")))
						{
							SaveCurrentBranch();
							if (TrimmedLine.StartsWith(TEXT("- destination:")))
							{
								CurrentBranch.DestinationState = GetLineValue(TrimmedLine);
							}
							else
							{
								CurrentBranch.Id = GetLineValue(TrimmedLine);
							}
							bInBranch = true;
							bInTasks = false;
							bInTask = false;
							bInBranchEvents = false;
							bInEvent = false;
						}
						else if (bInBranch)
						{
							if (TrimmedLine.StartsWith(TEXT("destination:")) && !bInTask)
							{
								CurrentBranch.DestinationState = GetLineValue(TrimmedLine);
							}
							else if (TrimmedLine.StartsWith(TEXT("id:")) && !bInTask)
							{
								CurrentBranch.Id = GetLineValue(TrimmedLine);
							}
							else if (TrimmedLine.StartsWith(TEXT("hidden:")) && !bInTask)
							{
								CurrentBranch.bHidden = GetLineValue(TrimmedLine).ToBool();
							}
							// v4.2: Branch custom event callback
							else if ((TrimmedLine.StartsWith(TEXT("on_entered_func_name:")) || TrimmedLine.StartsWith(TEXT("on_entered_func:"))) && !bInTask)
							{
								CurrentBranch.OnEnteredFuncName = GetLineValue(TrimmedLine);
							}
							else if (TrimmedLine.StartsWith(TEXT("tasks:")))
							{
								SaveCurrentEvent();
								bInTasks = true;
								bInTask = false;
								bInBranchEvents = false;
								bInEvent = false;
							}
							else if (TrimmedLine.StartsWith(TEXT("events:")) && !bInTasks)
							{
								SaveCurrentTask();
								bInBranchEvents = true;
								bInStateEvents = false;
								bInTasks = false;
								bInEvent = false;
							}
							else if (bInTasks)
							{
								// Task parsing
								if (TrimmedLine.StartsWith(TEXT("- task:")))
								{
									SaveCurrentTask();
									CurrentTask.TaskClass = GetLineValue(TrimmedLine);
									bInTask = true;
								}
								else if (bInTask)
								{
									if (TrimmedLine.StartsWith(TEXT("argument:")))
									{
										CurrentTask.Argument = GetLineValue(TrimmedLine);
									}
									else if (TrimmedLine.StartsWith(TEXT("quantity:")))
									{
										CurrentTask.Quantity = FCString::Atoi(*GetLineValue(TrimmedLine));
									}
									else if (TrimmedLine.StartsWith(TEXT("optional:")))
									{
										CurrentTask.bOptional = GetLineValue(TrimmedLine).ToBool();
									}
									else if (TrimmedLine.StartsWith(TEXT("hidden:")))
									{
										CurrentTask.bHidden = GetLineValue(TrimmedLine).ToBool();
									}
								}
							}
							else if (bInBranchEvents)
							{
								// Event parsing (branch events)
								if (TrimmedLine.StartsWith(TEXT("- type:")))
								{
									SaveCurrentEvent();
									CurrentEvent.Type = GetLineValue(TrimmedLine);
									bInEvent = true;
									bInEventProperties = false;
								}
								else if (bInEvent)
								{
									if (TrimmedLine.StartsWith(TEXT("runtime:")))
									{
										CurrentEvent.Runtime = GetLineValue(TrimmedLine);
									}
									else if (TrimmedLine.StartsWith(TEXT("properties:")))
									{
										bInEventProperties = true;
									}
									else if (bInEventProperties && TrimmedLine.Contains(TEXT(":")))
									{
										int32 ColonIdx;
										TrimmedLine.FindChar(TEXT(':'), ColonIdx);
										FString Key = TrimmedLine.Left(ColonIdx).TrimEnd();
										FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStart();
										CurrentEvent.Properties.Add(Key, Value);
									}
								}
							}
						}
					}
					else if (bInStateEvents)
					{
						// Event parsing (state events)
						if (TrimmedLine.StartsWith(TEXT("- type:")))
						{
							SaveCurrentEvent();
							CurrentEvent.Type = GetLineValue(TrimmedLine);
							bInEvent = true;
							bInEventProperties = false;
						}
						else if (bInEvent)
						{
							if (TrimmedLine.StartsWith(TEXT("runtime:")))
							{
								CurrentEvent.Runtime = GetLineValue(TrimmedLine);
							}
							else if (TrimmedLine.StartsWith(TEXT("properties:")))
							{
								bInEventProperties = true;
							}
							else if (bInEventProperties && TrimmedLine.Contains(TEXT(":")))
							{
								int32 ColonIdx;
								TrimmedLine.FindChar(TEXT(':'), ColonIdx);
								FString Key = TrimmedLine.Left(ColonIdx).TrimEnd();
								FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStart();
								CurrentEvent.Properties.Add(Key, Value);
							}
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// Save final quest
	SaveCurrentQuest();
}

// v3.9.8: Pipeline Config parser
void FGasAbilityGeneratorParser::ParsePipelineConfig(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("default_folder:")))
		{
			OutData.PipelineConfig.DefaultFolder = GetLineValue(TrimmedLine);
		}
		else if (TrimmedLine.StartsWith(TEXT("auto_create_collections:")))
		{
			FString Value = GetLineValue(TrimmedLine).ToLower();
			OutData.PipelineConfig.bAutoCreateCollections = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
		}
		else if (TrimmedLine.StartsWith(TEXT("auto_create_loadouts:")))
		{
			FString Value = GetLineValue(TrimmedLine).ToLower();
			OutData.PipelineConfig.bAutoCreateLoadouts = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
		}

		LineIndex++;
	}
}

// v3.9.8: Pipeline Items parser
void FGasAbilityGeneratorParser::ParsePipelineItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestPipelineItemDefinition CurrentDef;
	bool bInItem = false;
	bool bInClothingMesh = false;
	bool bInStats = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInItem && !CurrentDef.Mesh.IsEmpty())
			{
				OutData.PipelineItems.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- mesh:")))
		{
			// Save previous item
			if (bInItem && !CurrentDef.Mesh.IsEmpty())
			{
				OutData.PipelineItems.Add(CurrentDef);
			}
			CurrentDef = FManifestPipelineItemDefinition();
			CurrentDef.Mesh = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInClothingMesh = false;
			bInStats = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("name:")))
			{
				CurrentDef.Name = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("type:")))
			{
				CurrentDef.Type = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("slot:")))
			{
				CurrentDef.Slot = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("display_name:")))
			{
				CurrentDef.DisplayName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("target_collection:")))
			{
				CurrentDef.TargetCollection = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("clothing_mesh:")) || TrimmedLine.StartsWith(TEXT("clothing_mesh:")))
			{
				bInClothingMesh = true;
				bInStats = false;
			}
			else if (TrimmedLine.Equals(TEXT("stats:")) || TrimmedLine.StartsWith(TEXT("stats:")))
			{
				bInStats = true;
				bInClothingMesh = false;
			}
			else if (bInClothingMesh)
			{
				// Parse clothing mesh properties
				if (TrimmedLine.StartsWith(TEXT("mesh:")))
				{
					CurrentDef.ClothingMesh.Mesh = GetLineValue(TrimmedLine);
				}
				else if (TrimmedLine.StartsWith(TEXT("use_leader_pose:")))
				{
					FString Value = GetLineValue(TrimmedLine).ToLower();
					CurrentDef.ClothingMesh.bUseLeaderPose = (Value == TEXT("true") || Value == TEXT("1") || Value == TEXT("yes"));
				}
			}
			else if (bInStats)
			{
				// Parse stat key-value pairs
				if (TrimmedLine.Contains(TEXT(":")))
				{
					int32 ColonIdx;
					TrimmedLine.FindChar(TEXT(':'), ColonIdx);
					FString Key = TrimmedLine.Left(ColonIdx).TrimEnd();
					FString Value = TrimmedLine.Mid(ColonIdx + 1).TrimStart();
					CurrentDef.Stats.Add(Key, Value);
				}
			}
		}

		LineIndex++;
	}

	// Save final item
	if (bInItem && !CurrentDef.Mesh.IsEmpty())
	{
		OutData.PipelineItems.Add(CurrentDef);
	}
}

// v3.9.8: Pipeline Collections parser
void FGasAbilityGeneratorParser::ParsePipelineCollections(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestPipelineCollectionDefinition CurrentDef;
	bool bInCollection = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInCollection && !CurrentDef.Name.IsEmpty())
			{
				OutData.PipelineCollections.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous collection
			if (bInCollection && !CurrentDef.Name.IsEmpty())
			{
				OutData.PipelineCollections.Add(CurrentDef);
			}
			CurrentDef = FManifestPipelineCollectionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInCollection = true;
		}
		else if (bInCollection)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			// Items would be auto-populated from pipeline_items with matching target_collection
		}

		LineIndex++;
	}

	// Save final collection
	if (bInCollection && !CurrentDef.Name.IsEmpty())
	{
		OutData.PipelineCollections.Add(CurrentDef);
	}
}

// v3.9.8: Pipeline Loadouts parser
void FGasAbilityGeneratorParser::ParsePipelineLoadouts(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestPipelineLoadoutDefinition CurrentDef;
	bool bInLoadout = false;
	bool bInCollections = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
			if (bInLoadout && !CurrentDef.NPCDefinition.IsEmpty())
			{
				OutData.PipelineLoadouts.Add(CurrentDef);
			}
			LineIndex--;
			return;
		}

		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		if (TrimmedLine.StartsWith(TEXT("- npc_definition:")))
		{
			// Save previous loadout
			if (bInLoadout && !CurrentDef.NPCDefinition.IsEmpty())
			{
				OutData.PipelineLoadouts.Add(CurrentDef);
			}
			CurrentDef = FManifestPipelineLoadoutDefinition();
			CurrentDef.NPCDefinition = GetLineValue(TrimmedLine.Mid(2));
			bInLoadout = true;
			bInCollections = false;
		}
		else if (bInLoadout)
		{
			if (TrimmedLine.Equals(TEXT("collections:")) || TrimmedLine.StartsWith(TEXT("collections:")))
			{
				bInCollections = true;
			}
			else if (bInCollections && TrimmedLine.StartsWith(TEXT("-")))
			{
				// v3.9.10: Use StripYamlComment to remove inline comments
				FString Collection = StripYamlComment(TrimmedLine.Mid(1).TrimStart());
				// Remove quotes if present
				if (Collection.Len() >= 2 && ((Collection.StartsWith(TEXT("\"")) && Collection.EndsWith(TEXT("\""))) ||
					(Collection.StartsWith(TEXT("'")) && Collection.EndsWith(TEXT("'")))))
				{
					Collection = Collection.Mid(1, Collection.Len() - 2);
				}
				if (!Collection.IsEmpty())
				{
					CurrentDef.Collections.Add(Collection);
				}
			}
		}

		LineIndex++;
	}

	// Save final loadout
	if (bInLoadout && !CurrentDef.NPCDefinition.IsEmpty())
	{
		OutData.PipelineLoadouts.Add(CurrentDef);
	}
}

// v3.9.8: Parse FVector from string format {x: 1, y: 2, z: 3}
FVector FGasAbilityGeneratorParser::ParseVectorFromString(const FString& VectorStr)
{
	FVector Result = FVector::ZeroVector;

	// Remove braces and parse
	FString CleanStr = VectorStr;
	CleanStr.ReplaceInline(TEXT("{"), TEXT(""));
	CleanStr.ReplaceInline(TEXT("}"), TEXT(""));
	CleanStr = CleanStr.TrimStartAndEnd();

	// Parse x, y, z components
	TArray<FString> Components;
	CleanStr.ParseIntoArray(Components, TEXT(","));

	for (const FString& Component : Components)
	{
		FString TrimmedComponent = Component.TrimStartAndEnd();
		int32 ColonIdx;
		if (TrimmedComponent.FindChar(TEXT(':'), ColonIdx))
		{
			FString Key = TrimmedComponent.Left(ColonIdx).TrimStartAndEnd().ToLower();
			FString Value = TrimmedComponent.Mid(ColonIdx + 1).TrimStartAndEnd();
			float FloatValue = FCString::Atof(*Value);

			if (Key == TEXT("x"))
			{
				Result.X = FloatValue;
			}
			else if (Key == TEXT("y"))
			{
				Result.Y = FloatValue;
			}
			else if (Key == TEXT("z"))
			{
				Result.Z = FloatValue;
			}
		}
	}

	return Result;
}

// v3.9.8: Parse FRotator from string format {pitch: 0, yaw: 90, roll: 0}
FRotator FGasAbilityGeneratorParser::ParseRotatorFromString(const FString& RotatorStr)
{
	FRotator Result = FRotator::ZeroRotator;

	// Remove braces and parse
	FString CleanStr = RotatorStr;
	CleanStr.ReplaceInline(TEXT("{"), TEXT(""));
	CleanStr.ReplaceInline(TEXT("}"), TEXT(""));
	CleanStr = CleanStr.TrimStartAndEnd();

	// Parse pitch, yaw, roll components
	TArray<FString> Components;
	CleanStr.ParseIntoArray(Components, TEXT(","));

	for (const FString& Component : Components)
	{
		FString TrimmedComponent = Component.TrimStartAndEnd();
		int32 ColonIdx;
		if (TrimmedComponent.FindChar(TEXT(':'), ColonIdx))
		{
			FString Key = TrimmedComponent.Left(ColonIdx).TrimStartAndEnd().ToLower();
			FString Value = TrimmedComponent.Mid(ColonIdx + 1).TrimStartAndEnd();
			float FloatValue = FCString::Atof(*Value);

			if (Key == TEXT("pitch"))
			{
				Result.Pitch = FloatValue;
			}
			else if (Key == TEXT("yaw"))
			{
				Result.Yaw = FloatValue;
			}
			else if (Key == TEXT("roll"))
			{
				Result.Roll = FloatValue;
			}
		}
	}

	return Result;
}

// v3.9.9: Parse POI Placements
void FGasAbilityGeneratorParser::ParsePOIPlacements(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestPOIPlacement CurrentDef;
	bool bInItem = false;
	bool bInLinkedPOIs = false;

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for end of section
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New item - starts with "- poi_tag:"
		if (TrimmedLine.StartsWith(TEXT("- poi_tag:")))
		{
			// Save previous
			if (bInItem && !CurrentDef.POITag.IsEmpty())
			{
				OutData.POIPlacements.Add(CurrentDef);
			}

			CurrentDef = FManifestPOIPlacement();
			CurrentDef.POITag = GetLineValue(TrimmedLine);
			bInItem = true;
			bInLinkedPOIs = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("location:")))
			{
				// Parse [X, Y, Z] format
				FString LocStr = GetLineValue(TrimmedLine);
				LocStr = LocStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				LocStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.Location.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentDef.Location.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.Location.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("rotation:")))
			{
				// Parse [Roll, Pitch, Yaw] format
				FString RotStr = GetLineValue(TrimmedLine);
				RotStr = RotStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				RotStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.Rotation.Roll = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentDef.Rotation.Pitch = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.Rotation.Yaw = FCString::Atof(*Parts[2].TrimStartAndEnd());
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("display_name:")))
			{
				CurrentDef.DisplayName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("create_map_marker:")))
			{
				CurrentDef.bCreateMapMarker = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("supports_fast_travel:")))
			{
				CurrentDef.bSupportsFastTravel = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("map_icon:")))
			{
				CurrentDef.MapIcon = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("linked_pois:")))
			{
				bInLinkedPOIs = true;
			}
			else if (bInLinkedPOIs && TrimmedLine.StartsWith(TEXT("-")))
			{
				FString POITag = TrimmedLine.Mid(1).TrimStartAndEnd();
				if (!POITag.IsEmpty())
				{
					CurrentDef.LinkedPOIs.Add(POITag);
				}
			}
			else if (!TrimmedLine.StartsWith(TEXT("-")))
			{
				// Not a linked poi entry, end linked_pois section
				bInLinkedPOIs = false;
			}
		}

		LineIndex++;
	}

	// Save last item
	if (bInItem && !CurrentDef.POITag.IsEmpty())
	{
		OutData.POIPlacements.Add(CurrentDef);
	}
}

// v3.9.9: Parse NPC Spawner Placements
void FGasAbilityGeneratorParser::ParseNPCSpawnerPlacements(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestNPCSpawnerPlacement CurrentDef;
	FManifestNPCSpawnEntry CurrentNPC;
	bool bInItem = false;
	bool bInNPCs = false;
	bool bInNPC = false;
	bool bInSpawnParams = false;
	bool bInOwnedTags = false;
	bool bInFactions = false;

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for end of section
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New spawner - starts with "- name:"
		if (TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save previous
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				if (bInNPC && !CurrentNPC.NPCDefinition.IsEmpty())
				{
					CurrentDef.NPCs.Add(CurrentNPC);
				}
				OutData.NPCSpawnerPlacements.Add(CurrentDef);
			}

			CurrentDef = FManifestNPCSpawnerPlacement();
			CurrentNPC = FManifestNPCSpawnEntry();
			CurrentDef.Name = GetLineValue(TrimmedLine);
			bInItem = true;
			bInNPCs = false;
			bInNPC = false;
			bInSpawnParams = false;
			bInOwnedTags = false;
			bInFactions = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("location:")))
			{
				FString LocStr = GetLineValue(TrimmedLine);
				LocStr = LocStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				LocStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.Location.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentDef.Location.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.Location.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("rotation:")))
			{
				FString RotStr = GetLineValue(TrimmedLine);
				RotStr = RotStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				RotStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.Rotation.Roll = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentDef.Rotation.Pitch = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.Rotation.Yaw = FCString::Atof(*Parts[2].TrimStartAndEnd());
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("near_poi:")))
			{
				CurrentDef.NearPOI = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("poi_offset:")))
			{
				FString OffStr = GetLineValue(TrimmedLine);
				OffStr = OffStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				OffStr.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 3)
				{
					CurrentDef.POIOffset.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
					CurrentDef.POIOffset.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.POIOffset.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
				}
			}
			else if (TrimmedLine.StartsWith(TEXT("activate_on_begin_play:")))
			{
				CurrentDef.bActivateOnBeginPlay = GetLineValue(TrimmedLine).ToBool();
			}
			// v3.9.10: Spawner activation via NarrativeEvent
			else if (TrimmedLine.StartsWith(TEXT("activation_event:")))
			{
				CurrentDef.ActivationEvent = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("deactivation_event:")))
			{
				CurrentDef.DeactivationEvent = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("npcs:")))
			{
				bInNPCs = true;
				bInNPC = false;
				bInSpawnParams = false;
			}
			else if (bInNPCs)
			{
				// New NPC entry
				if (TrimmedLine.StartsWith(TEXT("- npc_definition:")))
				{
					// Save previous NPC
					if (bInNPC && !CurrentNPC.NPCDefinition.IsEmpty())
					{
						CurrentDef.NPCs.Add(CurrentNPC);
					}
					CurrentNPC = FManifestNPCSpawnEntry();
					CurrentNPC.NPCDefinition = GetLineValue(TrimmedLine);
					bInNPC = true;
					bInSpawnParams = false;
					bInOwnedTags = false;
					bInFactions = false;
				}
				else if (bInNPC)
				{
					if (TrimmedLine.StartsWith(TEXT("relative_location:")))
					{
						FString LocStr = GetLineValue(TrimmedLine);
						LocStr = LocStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
						TArray<FString> Parts;
						LocStr.ParseIntoArray(Parts, TEXT(","));
						if (Parts.Num() >= 3)
						{
							CurrentNPC.RelativeLocation.X = FCString::Atof(*Parts[0].TrimStartAndEnd());
							CurrentNPC.RelativeLocation.Y = FCString::Atof(*Parts[1].TrimStartAndEnd());
							CurrentNPC.RelativeLocation.Z = FCString::Atof(*Parts[2].TrimStartAndEnd());
						}
					}
					else if (TrimmedLine.StartsWith(TEXT("relative_rotation:")))
					{
						FString RotStr = GetLineValue(TrimmedLine);
						RotStr = RotStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
						TArray<FString> Parts;
						RotStr.ParseIntoArray(Parts, TEXT(","));
						if (Parts.Num() >= 3)
						{
							CurrentNPC.RelativeRotation.Roll = FCString::Atof(*Parts[0].TrimStartAndEnd());
							CurrentNPC.RelativeRotation.Pitch = FCString::Atof(*Parts[1].TrimStartAndEnd());
							CurrentNPC.RelativeRotation.Yaw = FCString::Atof(*Parts[2].TrimStartAndEnd());
						}
					}
					else if (TrimmedLine.StartsWith(TEXT("dont_spawn_if_killed:")))
					{
						CurrentNPC.bDontSpawnIfKilled = GetLineValue(TrimmedLine).ToBool();
					}
					else if (TrimmedLine.StartsWith(TEXT("optional_goal:")))
					{
						CurrentNPC.OptionalGoal = GetLineValue(TrimmedLine);
					}
					else if (TrimmedLine.StartsWith(TEXT("untether_distance:")))
					{
						CurrentNPC.UntetherDistance = FCString::Atof(*GetLineValue(TrimmedLine));
					}
					else if (TrimmedLine.StartsWith(TEXT("spawn_params:")))
					{
						bInSpawnParams = true;
						bInOwnedTags = false;
						bInFactions = false;
					}
					else if (bInSpawnParams)
					{
						if (TrimmedLine.StartsWith(TEXT("override_level_range:")))
						{
							CurrentNPC.SpawnParams.bOverrideLevelRange = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.StartsWith(TEXT("min_level:")))
						{
							CurrentNPC.SpawnParams.MinLevel = FCString::Atoi(*GetLineValue(TrimmedLine));
						}
						else if (TrimmedLine.StartsWith(TEXT("max_level:")))
						{
							CurrentNPC.SpawnParams.MaxLevel = FCString::Atoi(*GetLineValue(TrimmedLine));
						}
						else if (TrimmedLine.StartsWith(TEXT("override_owned_tags:")))
						{
							CurrentNPC.SpawnParams.bOverrideOwnedTags = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.StartsWith(TEXT("default_owned_tags:")))
						{
							bInOwnedTags = true;
							bInFactions = false;
							// Check for inline array [tag1, tag2]
							FString TagsStr = GetLineValue(TrimmedLine);
							if (TagsStr.StartsWith(TEXT("[")))
							{
								TagsStr = TagsStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
								TArray<FString> Tags;
								TagsStr.ParseIntoArray(Tags, TEXT(","));
								for (const FString& Tag : Tags)
								{
									CurrentNPC.SpawnParams.DefaultOwnedTags.Add(Tag.TrimStartAndEnd());
								}
								bInOwnedTags = false;
							}
						}
						else if (bInOwnedTags && TrimmedLine.StartsWith(TEXT("-")))
						{
							FString Tag = TrimmedLine.Mid(1).TrimStartAndEnd();
							if (!Tag.IsEmpty())
							{
								CurrentNPC.SpawnParams.DefaultOwnedTags.Add(Tag);
							}
						}
						else if (TrimmedLine.StartsWith(TEXT("override_factions:")))
						{
							CurrentNPC.SpawnParams.bOverrideFactions = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.StartsWith(TEXT("default_factions:")))
						{
							bInFactions = true;
							bInOwnedTags = false;
							// Check for inline array
							FString FactionsStr = GetLineValue(TrimmedLine);
							if (FactionsStr.StartsWith(TEXT("[")))
							{
								FactionsStr = FactionsStr.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
								TArray<FString> Factions;
								FactionsStr.ParseIntoArray(Factions, TEXT(","));
								for (const FString& Faction : Factions)
								{
									CurrentNPC.SpawnParams.DefaultFactions.Add(Faction.TrimStartAndEnd());
								}
								bInFactions = false;
							}
						}
						else if (bInFactions && TrimmedLine.StartsWith(TEXT("-")))
						{
							FString Faction = TrimmedLine.Mid(1).TrimStartAndEnd();
							if (!Faction.IsEmpty())
							{
								CurrentNPC.SpawnParams.DefaultFactions.Add(Faction);
							}
						}
						else if (TrimmedLine.StartsWith(TEXT("override_activity_configuration:")))
						{
							CurrentNPC.SpawnParams.bOverrideActivityConfiguration = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.StartsWith(TEXT("activity_configuration:")))
						{
							CurrentNPC.SpawnParams.ActivityConfiguration = GetLineValue(TrimmedLine);
						}
						else if (TrimmedLine.StartsWith(TEXT("override_appearance:")))
						{
							CurrentNPC.SpawnParams.bOverrideAppearance = GetLineValue(TrimmedLine).ToBool();
						}
						else if (TrimmedLine.StartsWith(TEXT("default_appearance:")))
						{
							CurrentNPC.SpawnParams.DefaultAppearance = GetLineValue(TrimmedLine);
						}
						else if (!TrimmedLine.StartsWith(TEXT("-")))
						{
							// Non-spawn-params property encountered
							bInSpawnParams = false;
							bInOwnedTags = false;
							bInFactions = false;
						}
					}
				}
			}
		}

		LineIndex++;
	}

	// Save last spawner
	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		if (bInNPC && !CurrentNPC.NPCDefinition.IsEmpty())
		{
			CurrentDef.NPCs.Add(CurrentNPC);
		}
		OutData.NPCSpawnerPlacements.Add(CurrentDef);
	}
}

// v4.13: Category C - FormStateEffects preset parser (P1.1)
// Manifest syntax:
//   form_state_effects:
//     - form: Armor
//       invulnerable: true
//     - form: Crawler
//       invulnerable: false
void FGasAbilityGeneratorParser::ParseFormStateEffects(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;  // Skip header

	FManifestFormStateEffectDefinition CurrentDef;
	bool bInItem = false;

	while (LineIndex < Lines.Num())
	{
		FString Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		int32 CurrentIndent = GetIndentLevel(Line);

		if (Line.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
		{
			LineIndex++;
			continue;
		}

		// Check for end of section (non-list line at or before section indent)
		if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
		{
			break;
		}

		// New form state entry
		if (TrimmedLine.StartsWith(TEXT("- form:")))
		{
			// Save previous
			if (bInItem && !CurrentDef.Form.IsEmpty())
			{
				OutData.FormStateEffects.Add(CurrentDef);
			}

			CurrentDef = FManifestFormStateEffectDefinition();
			CurrentDef.Form = GetLineValue(TrimmedLine);
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("invulnerable:")))
			{
				CurrentDef.bInvulnerable = GetLineValue(TrimmedLine).ToBool();
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
	}

	// Save last item
	if (bInItem && !CurrentDef.Form.IsEmpty())
	{
		OutData.FormStateEffects.Add(CurrentDef);
	}
}
