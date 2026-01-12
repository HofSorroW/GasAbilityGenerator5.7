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
				FString Value = TrimmedLine.Mid(1).TrimStart();
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
	int32 ItemIndent = -1;
	FManifestActorVariableDefinition CurrentVar;

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
			bInVariables = false;
			bInTags = false;
			bInEventGraph = false;
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
					FString TagValue = TrimmedLine.Mid(1).TrimStart();
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
			if (bInVariables && !CurrentVar.Name.IsEmpty())
			{
				CurrentDef.Variables.Add(CurrentVar);
				CurrentVar = FManifestWidgetVariableDefinition();
			}
			bInVariables = false;

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
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			// v2.2.0: Parse event_graph reference
			else if (TrimmedLine.StartsWith(TEXT("event_graph:")))
			{
				CurrentDef.EventGraphName = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
				bInVariables = true;
				VariablesIndent = CurrentIndent;
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
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		
		if (ShouldExitSection(Line, SectionIndent))
		{
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
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BB_")))
			{
				OutData.Blackboards.Add(CurrentDef);
			}
			CurrentDef = FManifestBlackboardDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}

		LineIndex++;
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
	
	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();
		
		if (ShouldExitSection(Line, SectionIndent))
		{
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
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("BT_")))
			{
				OutData.BehaviorTrees.Add(CurrentDef);
			}
			CurrentDef = FManifestBehaviorTreeDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("blackboard_asset:")))
			{
				CurrentDef.BlackboardAsset = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
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

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

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

		// New material function item - v2.6.13: only if not in a subsection
		if (!bInInputs && !bInOutputs && !bInExpressions && !bInConnections && TrimmedLine.StartsWith(TEXT("- name:")))
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
// v2.3.0: New Asset Type Parsers
// ============================================================================

void FGasAbilityGeneratorParser::ParseFloatCurves(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestFloatCurveDefinition CurrentDef;
	bool bInItem = false;
	bool bInKeys = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
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
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("FC_")))
			{
				OutData.FloatCurves.Add(CurrentDef);
			}
			CurrentDef = FManifestFloatCurveDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
			bInKeys = false;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("keys:")) || TrimmedLine.StartsWith(TEXT("keys:")))
			{
				bInKeys = true;
			}
			else if (bInKeys && TrimmedLine.StartsWith(TEXT("-")))
			{
				// Parse key as [time, value]
				FString KeyValue = TrimmedLine.Mid(1).TrimStart();
				KeyValue = KeyValue.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
				TArray<FString> Parts;
				KeyValue.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 2)
				{
					float Time = FCString::Atof(*Parts[0].TrimStartAndEnd());
					float Value = FCString::Atof(*Parts[1].TrimStartAndEnd());
					CurrentDef.Keys.Add(TPair<float, float>(Time, Value));
				}
			}
		}

		LineIndex++;
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
				FString Section = TrimmedLine.Mid(1).TrimStart();
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

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
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
			// v2.6.14: Prefix validation
			if (bInItem && !CurrentDef.Name.IsEmpty() &&
				(CurrentDef.Name.StartsWith(TEXT("AN_")) || CurrentDef.Name.StartsWith(TEXT("NAS_"))))
			{
				OutData.AnimationNotifies.Add(CurrentDef);
			}
			CurrentDef = FManifestAnimationNotifyDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("notify_class:")))
			{
				CurrentDef.NotifyClass = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
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
	int32 ItemIndent = -1;
	FManifestActorVariableDefinition CurrentVar;

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
			else if (TrimmedLine.Equals(TEXT("variables:")) || TrimmedLine.StartsWith(TEXT("variables:")))
			{
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
			else if (TrimmedLine.Equals(TEXT("abilities_to_grant:")) || TrimmedLine.StartsWith(TEXT("abilities_to_grant:")))
			{
				bInAbilities = true;
			}
			else if (bInAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				FString Ability = TrimmedLine.Mid(1).TrimStart();
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
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.Equals(TEXT("abilities:")) || TrimmedLine.StartsWith(TEXT("abilities:")))
			{
				bInAbilities = true;
			}
			else if (bInAbilities && TrimmedLine.StartsWith(TEXT("-")))
			{
				FString Ability = TrimmedLine.Mid(1).TrimStart();
				if (Ability.Len() >= 2 && ((Ability.StartsWith(TEXT("\"")) && Ability.EndsWith(TEXT("\""))) ||
					(Ability.StartsWith(TEXT("'")) && Ability.EndsWith(TEXT("'")))))
				{
					Ability = Ability.Mid(1, Ability.Len() - 2);
				}
				if (!Ability.IsEmpty())
				{
					CurrentDef.Abilities.Add(Ability);
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
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_activity:")))
			{
				CurrentDef.DefaultActivity = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("request_interval:")))
			{
				CurrentDef.RescoreInterval = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("rescore_interval:")))
			{
				CurrentDef.RescoreInterval = FCString::Atof(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.Equals(TEXT("activities:")) || TrimmedLine.StartsWith(TEXT("activities:")))
			{
				bInActivities = true;
			}
			else if (bInActivities && TrimmedLine.StartsWith(TEXT("-")))
			{
				FString Activity = TrimmedLine.Mid(1).TrimStart();
				if (Activity.Len() >= 2 && ((Activity.StartsWith(TEXT("\"")) && Activity.EndsWith(TEXT("\""))) ||
					(Activity.StartsWith(TEXT("'")) && Activity.EndsWith(TEXT("'")))))
				{
					Activity = Activity.Mid(1, Activity.Len() - 2);
				}
				if (!Activity.IsEmpty())
				{
					CurrentDef.Activities.Add(Activity);
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
				FString Item = TrimmedLine.Mid(1).TrimStart();
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

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
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
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NarrativeEvents.Add(CurrentDef);
			}
			CurrentDef = FManifestNarrativeEventDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
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
			else if (TrimmedLine.StartsWith(TEXT("event_tag:")))
			{
				CurrentDef.EventTag = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("event_type:")))
			{
				CurrentDef.EventType = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("description:")))
			{
				CurrentDef.Description = GetLineValue(TrimmedLine);
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.NarrativeEvents.Add(CurrentDef);
	}
}

void FGasAbilityGeneratorParser::ParseNPCDefinitions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData)
{
	int32 SectionIndent = GetIndentLevel(Lines[LineIndex]);
	LineIndex++;

	FManifestNPCDefinitionDefinition CurrentDef;
	bool bInItem = false;

	while (LineIndex < Lines.Num())
	{
		const FString& Line = Lines[LineIndex];
		FString TrimmedLine = Line.TrimStart();

		if (ShouldExitSection(Line, SectionIndent))
		{
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
			if (bInItem && !CurrentDef.Name.IsEmpty())
			{
				OutData.NPCDefinitions.Add(CurrentDef);
			}
			CurrentDef = FManifestNPCDefinitionDefinition();
			CurrentDef.Name = GetLineValue(TrimmedLine.Mid(2));
			bInItem = true;
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
		}

		LineIndex++;
	}

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
		}
		else if (bInItem)
		{
			if (TrimmedLine.StartsWith(TEXT("folder:")))
			{
				CurrentDef.Folder = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_owned_tags:")))
			{
				CurrentDef.DefaultOwnedTags = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_factions:")))
			{
				CurrentDef.DefaultFactions = GetLineValue(TrimmedLine);
			}
			else if (TrimmedLine.StartsWith(TEXT("default_currency:")))
			{
				CurrentDef.DefaultCurrency = FCString::Atoi(*GetLineValue(TrimmedLine));
			}
			else if (TrimmedLine.StartsWith(TEXT("attack_priority:")))
			{
				CurrentDef.AttackPriority = FCString::Atof(*GetLineValue(TrimmedLine));
			}
		}

		LineIndex++;
	}

	if (bInItem && !CurrentDef.Name.IsEmpty())
	{
		OutData.CharacterDefinitions.Add(CurrentDef);
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

FString FGasAbilityGeneratorParser::GetLineValue(const FString& Line)
{
	int32 ColonIndex;
	if (!Line.FindChar(TEXT(':'), ColonIndex))
	{
		return Line.TrimStartAndEnd();
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
	
	return Value;
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
	FManifestNiagaraUserParameter CurrentUserParam;
	int32 EmittersIndent = -1;
	int32 UserParamsIndent = -1;

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

		// New item entry - v2.6.13: only if not in user_parameters subsection
		if (!bInUserParameters && TrimmedLine.StartsWith(TEXT("- name:")))
		{
			// Save any pending user parameter
			if (bInUserParameters && !CurrentUserParam.Name.IsEmpty())
			{
				CurrentDef.UserParameters.Add(CurrentUserParam);
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
			CurrentUserParam = FManifestNiagaraUserParameter();
			EmittersIndent = -1;
			UserParamsIndent = -1;
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
			else if (TrimmedLine.StartsWith(TEXT("emitters:")))
			{
				bInEmitters = true;
				bInUserParameters = false;
				EmittersIndent = CurrentIndent;
			}
			else if (bInEmitters && TrimmedLine.StartsWith(TEXT("-")) && !bInUserParameters)
			{
				FString EmitterName = TrimmedLine.Mid(1).TrimStart();
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
			}
		}

		LineIndex++;
	}

	// v2.6.11: Save any pending user parameter before exiting
	if (bInUserParameters && !CurrentUserParam.Name.IsEmpty())
	{
		CurrentDef.UserParameters.Add(CurrentUserParam);
	}

	// Save any remaining entry
	if (bInItem && !CurrentDef.Name.IsEmpty() && CurrentDef.Name.StartsWith(TEXT("NS_")))
	{
		OutData.NiagaraSystems.Add(CurrentDef);
	}
}
