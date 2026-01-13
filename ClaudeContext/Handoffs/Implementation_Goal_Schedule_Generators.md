# Implementation Handoff: Goal_ and Schedule_ Generators

## For Terminal 1 (Writer Terminal)
**Date:** 2026-01-13
**From:** Terminal 3 (AI Research)

---

## PART 1: GOAL GENERATOR (Goal_)

### 1.1 Add to GasAbilityGeneratorTypes.h

Add this struct after `FManifestActivityDefinition` (around line 1768):

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

### 1.2 Add to FManifestData struct (around line 2806)

```cpp
TArray<FManifestGoalDefinition> Goals;
```

And in `BuildAssetWhitelist()`:

```cpp
for (const auto& Def : Goals) AssetWhitelist.Add(Def.Name);
```

### 1.3 Add to GasAbilityGeneratorGenerators.h

Add class declaration (after FActivityGenerator around line 627):

```cpp
class GASABILITYGENERATOR_API FGoalGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestGoalDefinition& Definition);
};
```

### 1.4 Add to GasAbilityGeneratorGenerators.cpp

Add include at top:

```cpp
#include "AI/Activities/NPCGoalItem.h"
```

Add generator implementation (after FActivityGenerator::Generate):

```cpp
// ============================================================================
// GOAL GENERATOR (Goal_)
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
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("UNPCGoalItem class not found"));
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

	// Compile blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Set CDO properties via reflection
	if (Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			// OwnedTags
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
						}
					}
				}
			}

			// BlockTags
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

			// RequireTags
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

			// DefaultScore
			FFloatProperty* ScoreProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DefaultScore")));
			if (ScoreProp)
			{
				ScoreProp->SetPropertyValue_InContainer(CDO, Definition.DefaultScore);
				LogGeneration(FString::Printf(TEXT("  Set DefaultScore: %.2f"), Definition.DefaultScore));
			}

			// bRemoveOnSucceeded
			FBoolProperty* RemoveProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bRemoveOnSucceeded")));
			if (RemoveProp)
			{
				RemoveProp->SetPropertyValue_InContainer(CDO, Definition.bRemoveOnSucceeded);
				LogGeneration(FString::Printf(TEXT("  Set bRemoveOnSucceeded: %s"), Definition.bRemoveOnSucceeded ? TEXT("true") : TEXT("false")));
			}

			// GoalLifetime
			FFloatProperty* LifetimeProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("GoalLifetime")));
			if (LifetimeProp)
			{
				LifetimeProp->SetPropertyValue_InContainer(CDO, Definition.GoalLifetime);
				LogGeneration(FString::Printf(TEXT("  Set GoalLifetime: %.2f"), Definition.GoalLifetime));
			}

			// bSaveGoal
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

### 1.5 Add YAML Parsing in GasAbilityGeneratorParser.cpp

In `ParseManifest()` function, add after activities parsing:

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

				// Parse numeric/bool values
				(*GoalObject)->TryGetNumberField(TEXT("default_score"), GoalDef.DefaultScore);
				(*GoalObject)->TryGetBoolField(TEXT("remove_on_succeeded"), GoalDef.bRemoveOnSucceeded);
				(*GoalObject)->TryGetNumberField(TEXT("goal_lifetime"), GoalDef.GoalLifetime);
				(*GoalObject)->TryGetBoolField(TEXT("save_goal"), GoalDef.bSaveGoal);

				OutManifest.Goals.Add(GoalDef);
			}
		}
	}
}
```

### 1.6 Add Generation Call in Commandlet/Window

In the generation loop (both commandlet and window), add:

```cpp
// Generate Goals
for (const FManifestGoalDefinition& Def : Manifest.Goals)
{
	FGenerationResult Result = FGoalGenerator::Generate(Def);
	Results.Add(Result);
	// Handle deferred if needed
}
```

---

## PART 2: SCHEDULE GENERATOR (Schedule_)

### 2.1 Add Structs to GasAbilityGeneratorTypes.h

```cpp
/**
 * Scheduled behavior entry for NPC schedules
 */
struct FManifestScheduledBehaviorEntry
{
	FString Type;                        // Behavior class name (e.g., "ScheduledBehavior_AddNPCGoal_Patrol")
	float StartTime = 0.0f;              // Time of day to start (0-2400, where 100 = 1 hour)
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

### 2.2 Add to FManifestData struct

```cpp
TArray<FManifestScheduleDefinition> Schedules;
```

And in `BuildAssetWhitelist()`:

```cpp
for (const auto& Def : Schedules) AssetWhitelist.Add(Def.Name);
```

### 2.3 Add to GasAbilityGeneratorGenerators.h

```cpp
class GASABILITYGENERATOR_API FScheduleGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestScheduleDefinition& Definition);
};
```

### 2.4 Add to GasAbilityGeneratorGenerators.cpp

Add include:

```cpp
#include "AI/Activities/NPCActivitySchedule.h"
```

Add generator:

```cpp
// ============================================================================
// SCHEDULE GENERATOR (Schedule_)
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
	for (const FManifestScheduledBehaviorEntry& Entry : Definition.Activities)
	{
		// Find the behavior class
		UClass* BehaviorClass = nullptr;

		// Try to find the class
		BehaviorClass = FindObject<UClass>(ANY_PACKAGE, *Entry.Type);

		if (!BehaviorClass)
		{
			// Try with full path patterns
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("/Game/%s/%s.%s_C"), *GetProjectName(), *Entry.Type, *Entry.Type),
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
			LogGeneration(FString::Printf(TEXT("  Warning: Behavior class '%s' not found - entry will be logged for manual setup"), *Entry.Type));
			LogGeneration(FString::Printf(TEXT("    Manual setup needed: %s from %.0f to %.0f"), *Entry.Type, Entry.StartTime, Entry.EndTime));
			continue;
		}

		// Ensure it's a valid subclass
		if (!BehaviorClass->IsChildOf(UScheduledBehavior_NPC::StaticClass()))
		{
			LogGeneration(FString::Printf(TEXT("  Warning: '%s' is not a UScheduledBehavior_NPC subclass"), *Entry.Type));
			continue;
		}

		// Skip abstract classes
		if (BehaviorClass->HasAnyClassFlags(CLASS_Abstract))
		{
			LogGeneration(FString::Printf(TEXT("  Warning: '%s' is abstract - need concrete Blueprint subclass"), *Entry.Type));
			continue;
		}

		// Create instanced subobject
		UScheduledBehavior_NPC* Behavior = NewObject<UScheduledBehavior_NPC>(
			Schedule,
			BehaviorClass,
			NAME_None,
			RF_Transactional);

		if (!Behavior)
		{
			LogGeneration(FString::Printf(TEXT("  Failed to create behavior instance: %s"), *Entry.Type));
			continue;
		}

		// Set base properties
		Behavior->bDisabled = Entry.bDisabled;
		Behavior->StartTime = Entry.StartTime;
		Behavior->EndTime = Entry.EndTime;

		// Try to set AddNPCGoal-specific properties via reflection
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

		// Add to schedule
		Schedule->Activities.Add(Behavior);
		AddedCount++;

		LogGeneration(FString::Printf(TEXT("  Added behavior: %s (%.0f - %.0f)"),
			*Entry.Type, Entry.StartTime, Entry.EndTime));
	}

	// Mark dirty and register
	Schedule->MarkPackageDirty();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Schedule);

	// Store metadata
	StoreDataAssetMetadata(Schedule, Definition.Name, Definition.ComputeHash());

	LogGeneration(FString::Printf(TEXT("Created Schedule: %s with %d/%d behaviors"),
		*Definition.Name, AddedCount, Definition.Activities.Num()));

	Result = FGenerationResult(Definition.Name, EGenerationStatus::Created,
		FString::Printf(TEXT("Created at %s (%d behaviors)"), *AssetPath, AddedCount));
	Result.GeneratedAsset = Schedule;
	Result.DetermineCategory();

	return Result;
}
```

### 2.5 Add YAML Parsing

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
							(*ActivityObject)->TryGetNumberField(TEXT("start_time"), Entry.StartTime);
							(*ActivityObject)->TryGetNumberField(TEXT("end_time"), Entry.EndTime);
							(*ActivityObject)->TryGetBoolField(TEXT("disabled"), Entry.bDisabled);
							(*ActivityObject)->TryGetNumberField(TEXT("score_override"), Entry.ScoreOverride);
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

## PART 3: YAML EXAMPLES

### 3.1 Goal Examples

```yaml
goals:
  # Simple patrol goal
  - name: Goal_PatrolArea
    folder: AI/Goals
    owned_tags:
      - State.Patrolling
    default_score: 20.0
    remove_on_succeeded: false
    goal_lifetime: -1.0

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

  # Alert/investigate goal
  - name: Goal_Investigate
    folder: AI/Goals
    owned_tags:
      - State.Investigating
    default_score: 50.0
    goal_lifetime: 60.0          # Expires after 60 seconds
    remove_on_succeeded: true
```

### 3.2 Schedule Examples

```yaml
schedules:
  # Blacksmith daily routine
  - name: Schedule_BlacksmithDay
    folder: NPCs/Schedules
    activities:
      # Morning work
      - type: ScheduledBehavior_StayAtWorkstation
        start_time: 800
        end_time: 1200
        score_override: 50.0
        reselect: true

      # Lunch break
      - type: ScheduledBehavior_GoToTavern
        start_time: 1200
        end_time: 1300
        score_override: 60.0

      # Afternoon work
      - type: ScheduledBehavior_StayAtWorkstation
        start_time: 1300
        end_time: 2000
        score_override: 50.0

      # Night - go home
      - type: ScheduledBehavior_GoHome
        start_time: 2000
        end_time: 800
        score_override: 70.0

  # Guard patrol schedule
  - name: Schedule_GuardPatrol
    folder: NPCs/Schedules
    activities:
      - type: ScheduledBehavior_AddPatrolGoal
        start_time: 600
        end_time: 1800
        score_override: 40.0
        reselect: true

      - type: ScheduledBehavior_AddPatrolGoal
        start_time: 1800
        end_time: 600
        score_override: 60.0     # More vigilant at night
```

---

## PART 4: IMPORTANT NOTES

### 4.1 UNPCGoalItem Properties (from NPCGoalItem.h)

```cpp
// Key properties to set via reflection:
UPROPERTY() FGameplayTagContainer OwnedTags;      // EditAnywhere, BlueprintReadOnly
UPROPERTY() FGameplayTagContainer BlockTags;      // EditAnywhere, BlueprintReadOnly
UPROPERTY() FGameplayTagContainer RequireTags;    // EditAnywhere, BlueprintReadOnly
UPROPERTY() bool bRemoveOnSucceeded;              // EditAnywhere, BlueprintReadWrite, SaveGame
UPROPERTY() float DefaultScore;                   // EditAnywhere, BlueprintReadWrite, SaveGame
UPROPERTY() bool bSaveGoal;                       // EditAnywhere, BlueprintReadWrite, SaveGame
UPROPERTY() float GoalLifetime;                   // EditAnywhere, BlueprintReadWrite, SaveGame
```

### 4.2 UScheduledBehavior Properties (from NarrativeGameState.h)

```cpp
UPROPERTY(EditDefaultsOnly) bool bDisabled;
UPROPERTY(EditDefaultsOnly) float StartTime;      // 0-2400 (100 = 1 hour)
UPROPERTY(EditDefaultsOnly) float EndTime;
```

### 4.3 UScheduledBehavior_AddNPCGoal Properties (from NPCActivitySchedule.h)

```cpp
UPROPERTY(EditDefaultsOnly) float ScoreOverride;  // >0 to override goal score
UPROPERTY(EditDefaultsOnly) bool bReselect;       // Trigger activity reselection
```

### 4.4 Schedule Complexity Warning

`UScheduledBehavior_AddNPCGoal` is **ABSTRACT** - it requires `ProvideGoal()` override. Users must create Blueprint subclasses like:
- `ScheduledBehavior_AddPatrolGoal`
- `ScheduledBehavior_AddIdleGoal`
- etc.

The generator references these by name. If not found, it logs for manual setup.

### 4.5 Generation Order

Add to generation order in commandlet/window:
1. Tags (existing)
2. Enumerations (existing)
3. **Goals** (new) - before Activities that reference them
4. Activities (existing)
5. **Schedules** (new) - after behavior BPs exist
6. NPCDefinitions (existing) - references schedules

---

## PART 5: VERIFICATION CHECKLIST

### Goals
- [ ] Goal_ asset created in Content Browser
- [ ] Blueprint opens without errors
- [ ] CDO shows correct OwnedTags
- [ ] CDO shows correct DefaultScore
- [ ] Parent class is UNPCGoalItem or subclass

### Schedules
- [ ] Schedule_ DataAsset created
- [ ] Activities array populated
- [ ] Each entry shows correct StartTime/EndTime
- [ ] NPCDefinition can reference via activity_schedules
- [ ] In-game: NPC behavior changes at scheduled times

---

## PART 6: FILES TO MODIFY

1. **GasAbilityGeneratorTypes.h**
   - Add `FManifestGoalDefinition`
   - Add `FManifestScheduledBehaviorEntry`
   - Add `FManifestScheduleDefinition`
   - Add arrays to `FManifestData`
   - Update `BuildAssetWhitelist()`

2. **GasAbilityGeneratorGenerators.h**
   - Add `FGoalGenerator` class declaration
   - Add `FScheduleGenerator` class declaration

3. **GasAbilityGeneratorGenerators.cpp**
   - Add includes for NPCGoalItem.h, NPCActivitySchedule.h
   - Add `FGoalGenerator::Generate()`
   - Add `FScheduleGenerator::Generate()`

4. **GasAbilityGeneratorParser.cpp**
   - Add goals parsing
   - Add schedules parsing

5. **GasAbilityGeneratorCommandlet.cpp** (or Window.cpp)
   - Add generation calls for Goals and Schedules

---

**END OF IMPLEMENTATION HANDOFF**
