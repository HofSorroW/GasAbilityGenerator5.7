# Complete Implementation: Goal_ and Schedule_ Generators

**For Terminal 1 (Writer Terminal)**
**From:** Terminal 3 (AI Research)
**Date:** 2026-01-13
**Status:** READY TO IMPLEMENT

---

## EXECUTIVE SUMMARY

### What to Implement
1. **Goal_ Generator** - Creates Blueprint assets deriving from `UNPCGoalItem`
2. **Schedule_ Generator** - Creates DataAsset `UNPCActivitySchedule` with time-based behaviors

### What NOT to Implement
- **Faction Generator** - NOT NEEDED. Factions are FGameplayTag, already handled by existing tag/NPC generators

### Key Findings from Research

| Question | Answer |
|----------|--------|
| Goals: DataAsset or Blueprint? | **Blueprint** (UObject subclass, NOT DataAsset) |
| Schedules: DataAsset or Blueprint? | **DataAsset** (UNPCActivitySchedule) |
| Faction storage? | **Global** in ANarrativeGameState, uses FGameplayTag |
| GoalGenerators needed? | **Optional** - static goals work fine via AddGoal() |

### Complexity Assessment

| Generator | Complexity | Notes |
|-----------|------------|-------|
| Goal_ | **Medium** | Straightforward Blueprint creation, follow FActivityGenerator pattern |
| Schedule_ | **High** | DataAsset with instanced subobjects, abstract class challenges |

---

## PART 1: NARRATIVE PRO CLASS REFERENCE

### 1.1 Goal System Classes

**Location:** `NarrativeArsenal/Public/AI/Activities/`

```cpp
// NPCGoalItem.h
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class UNPCGoalItem : public UObject
{
    // Tags granted when activity acts on this goal (removed when goal ends)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (ExposeOnSpawn=true))
    FGameplayTagContainer OwnedTags;

    // Force low score if owner has these tags (goal won't be acted on)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    FGameplayTagContainer BlockTags;

    // Required tags on owner to act on this goal
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    FGameplayTagContainer RequireTags;

    // Auto-remove when owning activity completes the goal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "NPC Goal", meta = (ExposeOnSpawn=true))
    bool bRemoveOnSucceeded;

    // Default score if activity doesn't override ScoreGoal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "NPC Goal", meta = (ExposeOnSpawn=true))
    float DefaultScore;

    // Save goal to disk?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "NPC Goal", meta = (ExposeOnSpawn=true))
    bool bSaveGoal;

    // Expiry time (<0 = never expire, needs manual removal)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "NPC Goal", meta = (ExposeOnSpawn=true))
    float GoalLifetime;
};
```

**Existing Narrative Pro Goals (for reference):**
- `Goal_Attack` - Attack a target
- `Goal_Flee` - Flee from threat
- `Goal_FollowCharacter` - Follow another character
- `Goal_MoveToDestination` - Move to location
- `Goal_Idle` - Idle behavior
- `Goal_Interact` - Interact with object
- `Goal_Patrol` - Patrol route

### 1.2 Schedule System Classes

**Location:** `NarrativeArsenal/Public/AI/Activities/NPCActivitySchedule.h`

```cpp
// Base scheduled behavior (in NarrativeGameState.h)
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class UScheduledBehavior : public UObject
{
    UPROPERTY(EditDefaultsOnly, Category = "Scheduled Behavior")
    bool bDisabled;

    // Time to begin (0-2400, where 100 = 1 hour)
    UPROPERTY(EditDefaultsOnly, Category = "Scheduled Behavior")
    float StartTime;

    // Time to end
    UPROPERTY(EditDefaultsOnly, Category = "Scheduled Behavior")
    float EndTime;
};

// NPC-specific scheduled behavior
UCLASS()
class UScheduledBehavior_NPC : public UScheduledBehavior
{
    // Inherits StartTime, EndTime, bDisabled
};

// Adds goal at start, removes at end (ABSTRACT - needs Blueprint subclass)
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class UScheduledBehavior_AddNPCGoal : public UScheduledBehavior_NPC
{
protected:
    // Override in Blueprint to provide the goal
    UFUNCTION(BlueprintNativeEvent, Category = "NPC Goals")
    UNPCGoalItem* ProvideGoal() const;

    // >0 to override created goal's score
    UPROPERTY(EditDefaultsOnly, Category = "Scheduled Behavior - Add NPC Goal")
    float ScoreOverride;

    // Trigger activity reselection after adding goal
    UPROPERTY(EditDefaultsOnly, Category = "Scheduled Behavior - Add NPC Goal")
    bool bReselect;
};

// The DataAsset that holds scheduled behaviors
UCLASS(Blueprintable, BlueprintType)
class UNPCActivitySchedule : public UDataAsset
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "NPC Activity")
    TArray<TObjectPtr<UScheduledBehavior_NPC>> Activities;
};
```

**Time Scale:**
```
0000 = Midnight
0600 = 6:00 AM
1200 = Noon
1800 = 6:00 PM
2400 = Midnight (wraps)
```

### 1.3 Faction System (NO GENERATOR NEEDED)

Factions use FGameplayTag with `Narrative.Factions.*` hierarchy:

```cpp
// In ANarrativeGameState (NarrativeGameState.h)
UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category = "Factions")
TMap<FGameplayTag, FFactionAttitudeData> FactionAllianceMap;

// Attitude types: ETeamAttitude::Friendly, Neutral, Hostile
```

**Already supported by existing generators:**
- `FTagGenerator` - Creates faction tags
- `FNPCDefinitionGenerator` - Sets DefaultFactions array
- `FCharacterDefinitionGenerator` - Sets DefaultFactions array

---

## PART 2: TYPES TO ADD (GasAbilityGeneratorTypes.h)

### 2.1 Goal Definition Struct

Add after `FManifestActivityDefinition` (~line 1768):

```cpp
/**
 * Goal definition for NPC AI goals
 * Goals are Blueprint classes deriving from UNPCGoalItem
 */
struct FManifestGoalDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;                 // Default: UNPCGoalItem

	// Tag-based activation/blocking
	TArray<FString> OwnedTags;           // Tags granted when activity acts on goal
	TArray<FString> BlockTags;           // Tags that prevent goal from being acted on
	TArray<FString> RequireTags;         // Tags required on owner to act on goal

	// Scoring and lifecycle
	float DefaultScore = 1.0f;           // Priority score for goal selection
	bool bRemoveOnSucceeded = true;      // Auto-remove when activity succeeds
	float GoalLifetime = -1.0f;          // Expiry time (<0 = never expire)
	bool bSaveGoal = false;              // Persist goal to save game

	/** Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(DefaultScore * 1000.f)) << 8;
		Hash ^= (bRemoveOnSucceeded ? 1ULL : 0ULL) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(GoalLifetime * 100.f)) << 17;
		Hash ^= (bSaveGoal ? 1ULL : 0ULL) << 32;

		for (const FString& Tag : OwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Tag : BlockTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const FString& Tag : RequireTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};
```

### 2.2 Schedule Definition Structs

```cpp
/**
 * Scheduled behavior entry for NPC schedules
 */
struct FManifestScheduledBehaviorEntry
{
	FString Type;                        // Behavior class name
	float StartTime = 0.0f;              // Time of day (0-2400)
	float EndTime = 2400.0f;             // Time of day to end
	bool bDisabled = false;              // Skip this entry

	// For AddNPCGoal behavior types
	float ScoreOverride = 0.0f;          // Override goal score (>0 to use)
	bool bReselect = false;              // Trigger activity reselection

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Type);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(StartTime * 100.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(EndTime * 100.f)) << 24;
		Hash ^= (bDisabled ? 1ULL : 0ULL) << 40;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScoreOverride * 100.f)) << 41;
		Hash ^= (bReselect ? 1ULL : 0ULL) << 56;
		return Hash;
	}
};

/**
 * Schedule definition for NPC daily routines
 * Schedules are DataAssets (UNPCActivitySchedule)
 */
struct FManifestScheduleDefinition
{
	FString Name;
	FString Folder;
	TArray<FManifestScheduledBehaviorEntry> Activities;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		for (const FManifestScheduledBehaviorEntry& Entry : Activities)
		{
			Hash ^= Entry.ComputeHash();
			Hash = (Hash << 11) | (Hash >> 53);
		}
		return Hash;
	}
};
```

### 2.3 Add to FManifestData struct (~line 2806)

```cpp
TArray<FManifestGoalDefinition> Goals;
TArray<FManifestScheduleDefinition> Schedules;
```

### 2.4 Update BuildAssetWhitelist()

```cpp
for (const auto& Def : Goals) AssetWhitelist.Add(Def.Name);
for (const auto& Def : Schedules) AssetWhitelist.Add(Def.Name);
```

---

## PART 3: GENERATOR DECLARATIONS (GasAbilityGeneratorGenerators.h)

Add after `FActivityGenerator` (~line 627):

```cpp
class GASABILITYGENERATOR_API FGoalGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestGoalDefinition& Definition);
};

class GASABILITYGENERATOR_API FScheduleGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestScheduleDefinition& Definition);
};
```

---

## PART 4: GENERATOR IMPLEMENTATIONS (GasAbilityGeneratorGenerators.cpp)

### 4.1 Add Includes at Top

```cpp
#include "AI/Activities/NPCGoalItem.h"
#include "AI/Activities/NPCActivitySchedule.h"
```

### 4.2 Goal Generator Implementation

```cpp
// ============================================================================
// GOAL GENERATOR (Goal_)
// Creates Blueprint assets deriving from UNPCGoalItem
// ============================================================================

FGenerationResult FGoalGenerator::Generate(const FManifestGoalDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Goals") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// Validate prefix
	if (!Definition.Name.StartsWith(TEXT("Goal_")))
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Goal name must start with 'Goal_' prefix, got: %s"), *Definition.Name));
	}

	if (ValidateAgainstManifest(Definition.Name, TEXT("Goal"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Goal"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Find parent class
	UClass* ParentClass = nullptr;
	if (!Definition.ParentClass.IsEmpty())
	{
		ParentClass = FindClass(Definition.ParentClass);
		if (!ParentClass)
		{
			LogGeneration(FString::Printf(TEXT("  Warning: Parent class '%s' not found, using UNPCGoalItem"), *Definition.ParentClass));
		}
	}

	if (!ParentClass)
	{
		ParentClass = UNPCGoalItem::StaticClass();
	}

	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("UNPCGoalItem class not found - is NarrativeArsenal linked?"));
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create Blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *Definition.Name,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Goal Blueprint"));
	}

	// Compile blueprint first
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Set CDO properties via reflection
	if (Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			// === OwnedTags ===
			if (Definition.OwnedTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("OwnedTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.OwnedTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added OwnedTag: %s"), *TagString));
							}
							else
							{
								LogGeneration(FString::Printf(TEXT("  Warning: Tag '%s' not found"), *TagString));
							}
						}
					}
				}
			}

			// === BlockTags ===
			if (Definition.BlockTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BlockTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.BlockTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added BlockTag: %s"), *TagString));
							}
						}
					}
				}
			}

			// === RequireTags ===
			if (Definition.RequireTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("RequireTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.RequireTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added RequireTag: %s"), *TagString));
							}
						}
					}
				}
			}

			// === DefaultScore ===
			FFloatProperty* ScoreProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DefaultScore")));
			if (ScoreProp)
			{
				ScoreProp->SetPropertyValue_InContainer(CDO, Definition.DefaultScore);
				LogGeneration(FString::Printf(TEXT("  Set DefaultScore: %.2f"), Definition.DefaultScore));
			}

			// === bRemoveOnSucceeded ===
			FBoolProperty* RemoveProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bRemoveOnSucceeded")));
			if (RemoveProp)
			{
				RemoveProp->SetPropertyValue_InContainer(CDO, Definition.bRemoveOnSucceeded);
				LogGeneration(FString::Printf(TEXT("  Set bRemoveOnSucceeded: %s"), Definition.bRemoveOnSucceeded ? TEXT("true") : TEXT("false")));
			}

			// === GoalLifetime ===
			FFloatProperty* LifetimeProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("GoalLifetime")));
			if (LifetimeProp)
			{
				LifetimeProp->SetPropertyValue_InContainer(CDO, Definition.GoalLifetime);
				LogGeneration(FString::Printf(TEXT("  Set GoalLifetime: %.2f"), Definition.GoalLifetime));
			}

			// === bSaveGoal ===
			FBoolProperty* SaveProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bSaveGoal")));
			if (SaveProp)
			{
				SaveProp->SetPropertyValue_InContainer(CDO, Definition.bSaveGoal);
				LogGeneration(FString::Printf(TEXT("  Set bSaveGoal: %s"), Definition.bSaveGoal ? TEXT("true") : TEXT("false")));
			}
		}
	}

	// Recompile after setting properties
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Mark dirty and save
	Blueprint->MarkPackageDirty();
	Package->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(Blueprint);

	// Store metadata for regen safety
	StoreAssetMetadata(Blueprint, Definition.Name, Definition.ComputeHash());

	LogGeneration(FString::Printf(TEXT("Created Goal: %s"), *Definition.Name));

	Result = FGenerationResult(Definition.Name, EGenerationStatus::Created,
		FString::Printf(TEXT("Created at %s"), *AssetPath));
	Result.GeneratedAsset = Blueprint;
	Result.DetermineCategory();

	return Result;
}
```

### 4.3 Schedule Generator Implementation

```cpp
// ============================================================================
// SCHEDULE GENERATOR (Schedule_)
// Creates UNPCActivitySchedule DataAssets with instanced behaviors
// ============================================================================

FGenerationResult FScheduleGenerator::Generate(const FManifestScheduleDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Schedules") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// Validate prefix
	if (!Definition.Name.StartsWith(TEXT("Schedule_")))
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Schedule name must start with 'Schedule_' prefix, got: %s"), *Definition.Name));
	}

	if (ValidateAgainstManifest(Definition.Name, TEXT("Schedule"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Schedule"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create the schedule DataAsset
	UNPCActivitySchedule* Schedule = NewObject<UNPCActivitySchedule>(
		Package, *Definition.Name, RF_Public | RF_Standalone);

	if (!Schedule)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Schedule DataAsset"));
	}

	// Process each scheduled behavior entry
	int32 AddedCount = 0;
	int32 SkippedCount = 0;

	for (const FManifestScheduledBehaviorEntry& Entry : Definition.Activities)
	{
		// Find the behavior class
		UClass* BehaviorClass = nullptr;

		// Try direct lookup
		BehaviorClass = FindObject<UClass>(ANY_PACKAGE, *Entry.Type);

		if (!BehaviorClass)
		{
			// Try common search paths
			TArray<FString> SearchPaths = {
				// Project Blueprints
				FString::Printf(TEXT("/Game/%s/AI/Behaviors/%s.%s_C"), *GetProjectName(), *Entry.Type, *Entry.Type),
				FString::Printf(TEXT("/Game/%s/AI/Schedules/%s.%s_C"), *GetProjectName(), *Entry.Type, *Entry.Type),
				FString::Printf(TEXT("/Game/%s/%s.%s_C"), *GetProjectName(), *Entry.Type, *Entry.Type),
				// Narrative Pro paths
				FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *Entry.Type),
				FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/%s.%s_C"), *Entry.Type, *Entry.Type)
			};

			for (const FString& Path : SearchPaths)
			{
				BehaviorClass = LoadClass<UScheduledBehavior_NPC>(nullptr, *Path);
				if (BehaviorClass) break;
			}
		}

		if (!BehaviorClass)
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: Behavior class '%s' not found"), *Entry.Type));
			LogGeneration(FString::Printf(TEXT("    -> Manual setup needed: Add %s from %.0f to %.0f"), *Entry.Type, Entry.StartTime, Entry.EndTime));
			SkippedCount++;
			continue;
		}

		// Verify it's a valid subclass
		if (!BehaviorClass->IsChildOf(UScheduledBehavior_NPC::StaticClass()))
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: '%s' is not a UScheduledBehavior_NPC subclass"), *Entry.Type));
			SkippedCount++;
			continue;
		}

		// Skip abstract classes (like UScheduledBehavior_AddNPCGoal itself)
		if (BehaviorClass->HasAnyClassFlags(CLASS_Abstract))
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: '%s' is abstract - need concrete Blueprint subclass"), *Entry.Type));
			LogGeneration(FString::Printf(TEXT("    -> Create a Blueprint child of %s that overrides ProvideGoal()"), *Entry.Type));
			SkippedCount++;
			continue;
		}

		// Create instanced subobject with Schedule as outer
		UScheduledBehavior_NPC* Behavior = NewObject<UScheduledBehavior_NPC>(
			Schedule,           // Outer = the schedule (important for instanced!)
			BehaviorClass,
			NAME_None,
			RF_Transactional);

		if (!Behavior)
		{
			LogGeneration(FString::Printf(TEXT("  ERROR: Failed to create behavior instance: %s"), *Entry.Type));
			SkippedCount++;
			continue;
		}

		// Set base UScheduledBehavior properties (public, can set directly)
		Behavior->bDisabled = Entry.bDisabled;
		Behavior->StartTime = Entry.StartTime;
		Behavior->EndTime = Entry.EndTime;

		// Set AddNPCGoal-specific properties via reflection (protected)
		if (Entry.ScoreOverride > 0.f)
		{
			FFloatProperty* ScoreProp = CastField<FFloatProperty>(
				Behavior->GetClass()->FindPropertyByName(TEXT("ScoreOverride")));
			if (ScoreProp)
			{
				ScoreProp->SetPropertyValue_InContainer(Behavior, Entry.ScoreOverride);
			}
		}

		FBoolProperty* ReselectProp = CastField<FBoolProperty>(
			Behavior->GetClass()->FindPropertyByName(TEXT("bReselect")));
		if (ReselectProp)
		{
			ReselectProp->SetPropertyValue_InContainer(Behavior, Entry.bReselect);
		}

		// Add to schedule's Activities array
		Schedule->Activities.Add(Behavior);
		AddedCount++;

		LogGeneration(FString::Printf(TEXT("  Added: %s (%.0f - %.0f)"),
			*Entry.Type, Entry.StartTime, Entry.EndTime));
	}

	// Mark dirty and register
	Schedule->MarkPackageDirty();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Schedule);

	// Store metadata for regen safety
	StoreDataAssetMetadata(Schedule, Definition.Name, Definition.ComputeHash());

	// Build result message
	FString Message;
	if (SkippedCount > 0)
	{
		Message = FString::Printf(TEXT("Created at %s (%d/%d behaviors, %d need manual setup)"),
			*AssetPath, AddedCount, Definition.Activities.Num(), SkippedCount);
	}
	else
	{
		Message = FString::Printf(TEXT("Created at %s (%d behaviors)"), *AssetPath, AddedCount);
	}

	LogGeneration(FString::Printf(TEXT("Created Schedule: %s"), *Definition.Name));

	Result = FGenerationResult(Definition.Name, EGenerationStatus::Created, Message);
	Result.GeneratedAsset = Schedule;
	Result.DetermineCategory();

	return Result;
}
```

---

## PART 5: YAML PARSING (GasAbilityGeneratorParser.cpp)

Add in `ParseManifest()` function:

### 5.1 Parse Goals

```cpp
// Parse goals
if (RootObject->HasField(TEXT("goals")))
{
	const TArray<TSharedPtr<FJsonValue>>* GoalsArray;
	if (RootObject->TryGetArrayField(TEXT("goals"), GoalsArray))
	{
		for (const TSharedPtr<FJsonValue>& GoalValue : *GoalsArray)
		{
			const TSharedPtr<FJsonObject>* GoalObject;
			if (GoalValue->TryGetObject(GoalObject))
			{
				FManifestGoalDefinition GoalDef;

				(*GoalObject)->TryGetStringField(TEXT("name"), GoalDef.Name);
				(*GoalObject)->TryGetStringField(TEXT("folder"), GoalDef.Folder);
				(*GoalObject)->TryGetStringField(TEXT("parent_class"), GoalDef.ParentClass);

				// Parse tag arrays
				ParseStringArray(*GoalObject, TEXT("owned_tags"), GoalDef.OwnedTags);
				ParseStringArray(*GoalObject, TEXT("block_tags"), GoalDef.BlockTags);
				ParseStringArray(*GoalObject, TEXT("require_tags"), GoalDef.RequireTags);

				// Parse numeric/bool values with defaults
				double TempScore = GoalDef.DefaultScore;
				if ((*GoalObject)->TryGetNumberField(TEXT("default_score"), TempScore))
				{
					GoalDef.DefaultScore = static_cast<float>(TempScore);
				}

				(*GoalObject)->TryGetBoolField(TEXT("remove_on_succeeded"), GoalDef.bRemoveOnSucceeded);

				double TempLifetime = GoalDef.GoalLifetime;
				if ((*GoalObject)->TryGetNumberField(TEXT("goal_lifetime"), TempLifetime))
				{
					GoalDef.GoalLifetime = static_cast<float>(TempLifetime);
				}

				(*GoalObject)->TryGetBoolField(TEXT("save_goal"), GoalDef.bSaveGoal);

				OutManifest.Goals.Add(GoalDef);
			}
		}
	}
}
```

### 5.2 Parse Schedules

```cpp
// Parse schedules
if (RootObject->HasField(TEXT("schedules")))
{
	const TArray<TSharedPtr<FJsonValue>>* SchedulesArray;
	if (RootObject->TryGetArrayField(TEXT("schedules"), SchedulesArray))
	{
		for (const TSharedPtr<FJsonValue>& ScheduleValue : *SchedulesArray)
		{
			const TSharedPtr<FJsonObject>* ScheduleObject;
			if (ScheduleValue->TryGetObject(ScheduleObject))
			{
				FManifestScheduleDefinition ScheduleDef;

				(*ScheduleObject)->TryGetStringField(TEXT("name"), ScheduleDef.Name);
				(*ScheduleObject)->TryGetStringField(TEXT("folder"), ScheduleDef.Folder);

				// Parse activities array
				const TArray<TSharedPtr<FJsonValue>>* ActivitiesArray;
				if ((*ScheduleObject)->TryGetArrayField(TEXT("activities"), ActivitiesArray))
				{
					for (const TSharedPtr<FJsonValue>& ActivityValue : *ActivitiesArray)
					{
						const TSharedPtr<FJsonObject>* ActivityObject;
						if (ActivityValue->TryGetObject(ActivityObject))
						{
							FManifestScheduledBehaviorEntry Entry;

							(*ActivityObject)->TryGetStringField(TEXT("type"), Entry.Type);

							double TempStart = Entry.StartTime;
							if ((*ActivityObject)->TryGetNumberField(TEXT("start_time"), TempStart))
							{
								Entry.StartTime = static_cast<float>(TempStart);
							}

							double TempEnd = Entry.EndTime;
							if ((*ActivityObject)->TryGetNumberField(TEXT("end_time"), TempEnd))
							{
								Entry.EndTime = static_cast<float>(TempEnd);
							}

							(*ActivityObject)->TryGetBoolField(TEXT("disabled"), Entry.bDisabled);

							double TempScore = Entry.ScoreOverride;
							if ((*ActivityObject)->TryGetNumberField(TEXT("score_override"), TempScore))
							{
								Entry.ScoreOverride = static_cast<float>(TempScore);
							}

							(*ActivityObject)->TryGetBoolField(TEXT("reselect"), Entry.bReselect);

							ScheduleDef.Activities.Add(Entry);
						}
					}
				}

				OutManifest.Schedules.Add(ScheduleDef);
			}
		}
	}
}
```

---

## PART 6: GENERATION CALLS (Commandlet/Window)

Add to the generation loop:

```cpp
// Generate Goals (before Activities that might reference them)
for (const FManifestGoalDefinition& Def : Manifest.Goals)
{
	FGenerationResult Result = FGoalGenerator::Generate(Def);
	Results.Add(Result);
	HandleDeferredResult(Result, DeferredAssets);
}

// Generate Schedules (after behavior BPs exist)
for (const FManifestScheduleDefinition& Def : Manifest.Schedules)
{
	FGenerationResult Result = FScheduleGenerator::Generate(Def);
	Results.Add(Result);
	HandleDeferredResult(Result, DeferredAssets);
}
```

**Generation Order** (add Goals after Enumerations, Schedules after Activities):
1. Tags
2. Enumerations
3. **Goals** ← NEW
4. GameplayEffects
5. GameplayAbilities
6. Activities
7. **Schedules** ← NEW
8. NPCDefinitions

---

## PART 7: YAML EXAMPLES

### 7.1 Complete Goal Examples

```yaml
goals:
  # Simple patrol goal - low priority, never expires
  - name: Goal_PatrolArea
    folder: AI/Goals
    owned_tags:
      - State.Patrolling
    default_score: 20.0
    remove_on_succeeded: false
    goal_lifetime: -1.0
    save_goal: false

  # High-priority defense goal
  - name: Goal_DefendPosition
    folder: AI/Goals
    owned_tags:
      - State.Defending
      - AI.Goal.Combat
    block_tags:
      - State.Incapacitated
      - State.Dead
    require_tags:
      - State.Alive
    default_score: 80.0
    remove_on_succeeded: true
    save_goal: true

  # Temporary investigation goal
  - name: Goal_Investigate
    folder: AI/Goals
    owned_tags:
      - State.Investigating
    default_score: 50.0
    goal_lifetime: 60.0        # Expires after 60 game-seconds
    remove_on_succeeded: true

  # Custom parent class example
  - name: Goal_FatherFollow
    folder: AI/Goals/Father
    parent_class: Goal_FollowCharacter    # Inherit from Narrative Pro goal
    owned_tags:
      - Father.State.Following
    default_score: 30.0
```

### 7.2 Complete Schedule Examples

```yaml
schedules:
  # Blacksmith daily routine
  - name: Schedule_BlacksmithDay
    folder: NPCs/Schedules
    activities:
      # Morning work at forge
      - type: ScheduledBehavior_WorkAtForge
        start_time: 800
        end_time: 1200
        score_override: 50.0
        reselect: true

      # Lunch at tavern
      - type: ScheduledBehavior_GoToTavern
        start_time: 1200
        end_time: 1300
        score_override: 60.0

      # Afternoon work
      - type: ScheduledBehavior_WorkAtForge
        start_time: 1300
        end_time: 2000
        score_override: 50.0

      # Night - go home
      - type: ScheduledBehavior_GoHome
        start_time: 2000
        end_time: 800
        score_override: 70.0

  # Guard patrol - different behavior day/night
  - name: Schedule_GuardPatrol
    folder: NPCs/Schedules
    activities:
      # Day patrol - relaxed
      - type: ScheduledBehavior_AddPatrolGoal
        start_time: 600
        end_time: 1800
        score_override: 40.0
        reselect: true

      # Night patrol - vigilant
      - type: ScheduledBehavior_AddPatrolGoal
        start_time: 1800
        end_time: 600
        score_override: 60.0      # Higher priority at night

  # Simple idle schedule (disabled entry example)
  - name: Schedule_IdleNPC
    folder: NPCs/Schedules
    activities:
      - type: ScheduledBehavior_Idle
        start_time: 0
        end_time: 2400
        disabled: false
```

---

## PART 8: IMPORTANT WARNINGS

### 8.1 Schedule Abstract Class Issue

`UScheduledBehavior_AddNPCGoal` is **ABSTRACT**. It requires `ProvideGoal()` to be overridden in Blueprint.

**Solution:** Users must create concrete Blueprint classes like:
- `ScheduledBehavior_AddPatrolGoal` (overrides ProvideGoal to return Goal_Patrol)
- `ScheduledBehavior_AddIdleGoal` (overrides ProvideGoal to return Goal_Idle)

The generator references these by name. If not found, it logs for manual setup.

### 8.2 Tag Registration

Tags in `owned_tags`, `block_tags`, `require_tags` MUST exist in DefaultGameplayTags.ini before Goals are generated. Otherwise `FGameplayTag::RequestGameplayTag` returns invalid.

**Solution:** Generate Tags first, or warn if tag not found.

### 8.3 Schedule Time Wraparound

When `EndTime < StartTime`, it means overnight (e.g., 2000-0800 = 8PM to 8AM).
Narrative Pro handles this internally - no special code needed.

---

## PART 9: FILES TO MODIFY (CHECKLIST)

```
□ GasAbilityGeneratorTypes.h
  □ Add FManifestGoalDefinition struct
  □ Add FManifestScheduledBehaviorEntry struct
  □ Add FManifestScheduleDefinition struct
  □ Add TArray<FManifestGoalDefinition> Goals to FManifestData
  □ Add TArray<FManifestScheduleDefinition> Schedules to FManifestData
  □ Update BuildAssetWhitelist()

□ GasAbilityGeneratorGenerators.h
  □ Add FGoalGenerator class declaration
  □ Add FScheduleGenerator class declaration

□ GasAbilityGeneratorGenerators.cpp
  □ Add #include "AI/Activities/NPCGoalItem.h"
  □ Add #include "AI/Activities/NPCActivitySchedule.h"
  □ Add FGoalGenerator::Generate() implementation
  □ Add FScheduleGenerator::Generate() implementation

□ GasAbilityGeneratorParser.cpp
  □ Add goals parsing section
  □ Add schedules parsing section

□ GasAbilityGeneratorCommandlet.cpp
  □ Add Goal generation loop
  □ Add Schedule generation loop

□ GasAbilityGeneratorWindow.cpp (if UI generation)
  □ Add Goal generation loop
  □ Add Schedule generation loop
```

---

## PART 10: VERIFICATION

### After Building

1. Build succeeds without errors
2. `UNPCGoalItem` and `UNPCActivitySchedule` classes found

### After Generation

**Goals:**
- [ ] Goal_ asset created in `{ProjectRoot}/AI/Goals/`
- [ ] Blueprint opens without errors
- [ ] Parent class is UNPCGoalItem (or specified)
- [ ] CDO shows correct OwnedTags populated
- [ ] CDO shows correct DefaultScore value
- [ ] CDO shows correct bRemoveOnSucceeded value

**Schedules:**
- [ ] Schedule_ DataAsset created in `{ProjectRoot}/AI/Schedules/`
- [ ] DataAsset opens showing Activities array
- [ ] Each entry shows correct behavior type
- [ ] Each entry shows correct StartTime/EndTime
- [ ] NPCDefinition can reference via `activity_schedules`

### Runtime Test

- [ ] NPC with schedule changes behavior at correct times
- [ ] Goals can be added to NPC via Activity system
- [ ] Activity correctly uses Goal's tags and score

---

**END OF IMPLEMENTATION DOCUMENT**

*This single file contains everything needed to implement Goal_ and Schedule_ generators.*
