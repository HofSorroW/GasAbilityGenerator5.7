// GasAbilityGeneratorCommandlet.cpp
// Commandlet for automated asset generation from command line
// v3.1: Fixed exit code, added dedicated log category, exception handling

#include "GasAbilityGeneratorCommandlet.h"
#include "GasAbilityGeneratorParser.h"
#include "GasAbilityGeneratorGenerators.h"
#include "Locked/GasAbilityGeneratorMetadata.h"  // v3.1: For metadata registry
#include "GasAbilityGeneratorDialogueCSVParser.h"  // v4.0: CSV dialogue parsing
#include "GasAbilityGeneratorReport.h"  // v4.7: Generation report system
#include "GasAbilityGeneratorPipeline.h"  // v4.12: Mesh-to-Item Pipeline
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"  // v4.11: For FApp::CanEverRender() in headless detection
#include "HAL/PlatformFilemanager.h"
#include <exception>  // v3.1: For std::exception in try/catch

// v3.9.9: World loading and saving for level actor placement
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EditorWorldUtils.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Navigation/POIActor.h"
#include "Spawners/NPCSpawner.h"

// v3.1: Dedicated log category for filtering
DEFINE_LOG_CATEGORY_STATIC(LogGasAbilityGenerator, Log, All);

UGasAbilityGeneratorCommandlet::UGasAbilityGeneratorCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UGasAbilityGeneratorCommandlet::Main(const FString& Params)
{
	LogMessage(TEXT("========================================"));
	LogMessage(TEXT("GasAbilityGenerator Commandlet v3.0"));
	LogMessage(TEXT("========================================"));

	// Parse command line parameters
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParamVals);

	// Get manifest path
	FString ManifestPath;
	if (!FParse::Value(*Params, TEXT("-manifest="), ManifestPath))
	{
		// Try without the = sign
		const FString* ManifestVal = ParamVals.Find(TEXT("manifest"));
		if (ManifestVal)
		{
			ManifestPath = *ManifestVal;
		}
	}

	// Remove quotes if present
	ManifestPath = ManifestPath.TrimQuotes();

	if (ManifestPath.IsEmpty())
	{
		LogError(TEXT("ERROR: No manifest path specified. Use -manifest=\"path/to/manifest.yaml\""));
		return 1;
	}

	// Make path absolute if relative
	if (FPaths::IsRelative(ManifestPath))
	{
		ManifestPath = FPaths::Combine(FPaths::ProjectDir(), ManifestPath);
	}
	FPaths::NormalizeFilename(ManifestPath);

	LogMessage(FString::Printf(TEXT("Manifest: %s"), *ManifestPath));

	// Check if manifest exists
	if (!FPaths::FileExists(ManifestPath))
	{
		LogError(FString::Printf(TEXT("ERROR: Manifest file not found: %s"), *ManifestPath));
		return 1;
	}

	// Get output log path
	FParse::Value(*Params, TEXT("-output="), OutputLogPath);
	if (!OutputLogPath.IsEmpty())
	{
		OutputLogPath = OutputLogPath.TrimQuotes();
		LogMessage(FString::Printf(TEXT("Output log: %s"), *OutputLogPath));
	}

	// v3.0: Dry run and force mode flags
	bool bDryRun = Switches.Contains(TEXT("dryrun")) || Switches.Contains(TEXT("dry-run"));
	bool bForce = Switches.Contains(TEXT("force"));

	if (bDryRun)
	{
		FGeneratorBase::SetDryRunMode(true);
		LogMessage(TEXT("MODE: Dry Run (preview only, no changes will be made)"));
	}

	if (bForce)
	{
		FGeneratorBase::SetForceMode(true);
		LogMessage(TEXT("MODE: Force (will overwrite even on conflicts)"));
	}

	// v3.9.9: Parse -level parameter for World Partition level loading
	FString LevelPath;
	if (FParse::Value(*Params, TEXT("-level="), LevelPath))
	{
		LevelPath = LevelPath.TrimQuotes();
	}
	else
	{
		const FString* LevelVal = ParamVals.Find(TEXT("level"));
		if (LevelVal)
		{
			LevelPath = LevelVal->TrimQuotes();
		}
	}

	if (!LevelPath.IsEmpty())
	{
		LogMessage(FString::Printf(TEXT("Level: %s"), *LevelPath));
	}

	// v3.0: Set manifest path for metadata tracking
	FGeneratorBase::SetManifestPath(ManifestPath);

	// Determine what to generate
	bool bGenerateTags = Switches.Contains(TEXT("tags")) || Switches.Contains(TEXT("all"));
	bool bGenerateAssets = Switches.Contains(TEXT("assets")) || Switches.Contains(TEXT("all"));

	// Default to all if nothing specified
	if (!bGenerateTags && !bGenerateAssets)
	{
		bGenerateTags = true;
		bGenerateAssets = true;
	}

	LogMessage(FString::Printf(TEXT("Generate Tags: %s"), bGenerateTags ? TEXT("YES") : TEXT("NO")));
	LogMessage(FString::Printf(TEXT("Generate Assets: %s"), bGenerateAssets ? TEXT("YES") : TEXT("NO")));
	LogMessage(FString::Printf(TEXT("Dry Run: %s"), bDryRun ? TEXT("YES") : TEXT("NO")));
	LogMessage(FString::Printf(TEXT("Force: %s"), bForce ? TEXT("YES") : TEXT("NO")));
	LogMessage(TEXT(""));

	// Read manifest file
	LogMessage(TEXT("Reading manifest file..."));
	FString ManifestContent;
	if (!FFileHelper::LoadFileToString(ManifestContent, *ManifestPath))
	{
		LogError(FString::Printf(TEXT("ERROR: Failed to read manifest file: %s"), *ManifestPath));
		return 1;
	}

	// v4.7: Compute manifest hash for report tracking
	int64 ManifestHash = static_cast<int64>(GetTypeHash(ManifestContent));

	// v4.7: Cache values for report generation in GenerateAssets
	CachedManifestPath = ManifestPath;
	CachedManifestHash = ManifestHash;
	bCachedForceMode = bForce;

	// Parse manifest with exception handling
	LogMessage(TEXT("Parsing manifest..."));
	FManifestData ManifestData;

	// v3.1: Wrap parsing in try/catch for safety
	try
	{
		if (!FGasAbilityGeneratorParser::ParseManifest(ManifestContent, ManifestData))
		{
			LogError(TEXT("ERROR: Failed to parse manifest file"));
			bHadParseError = true;
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		LogError(FString::Printf(TEXT("ERROR: Manifest parse exception: %hs"), e.what()));
		bHadParseError = true;
		return 1;
	}
	catch (...)
	{
		LogError(TEXT("ERROR: Unknown manifest parse exception"));
		bHadParseError = true;
		return 1;
	}

	// v4.13: Category C - Expand FormStateEffects to GameplayEffects (P1.1)
	if (ManifestData.FormStateEffects.Num() > 0)
	{
		LogMessage(FString::Printf(TEXT("Expanding %d form_state_effects to gameplay_effects"), ManifestData.FormStateEffects.Num()));
		for (const auto& FormState : ManifestData.FormStateEffects)
		{
			FManifestGameplayEffectDefinition ExpandedGE = FormState.ToGameplayEffectDefinition();
			// Check for duplicates (don't overwrite explicit GE definitions)
			bool bExists = ManifestData.GameplayEffects.ContainsByPredicate([&](const FManifestGameplayEffectDefinition& Existing) {
				return Existing.Name == ExpandedGE.Name;
			});
			if (!bExists)
			{
				ManifestData.GameplayEffects.Add(ExpandedGE);
				LogMessage(FString::Printf(TEXT("  Expanded: %s -> %s%s"),
					*FormState.Form, *ExpandedGE.Name,
					FormState.bInvulnerable ? TEXT(" (invulnerable)") : TEXT("")));
			}
			else
			{
				LogMessage(FString::Printf(TEXT("  Skipped: %s (explicit GE definition exists)"), *ExpandedGE.Name));
			}
		}
	}

	LogMessage(FString::Printf(TEXT("Parsed manifest with %d tags, %d enumerations, %d abilities, %d effects, %d blueprints, %d MICs"),
		ManifestData.Tags.Num(),
		ManifestData.Enumerations.Num(),
		ManifestData.GameplayAbilities.Num(),
		ManifestData.GameplayEffects.Num(),
		ManifestData.ActorBlueprints.Num(),
		ManifestData.MaterialInstances.Num()));

	// v4.0: Parse dialogue CSV if provided
	FString DialogueCSVPath;
	if (FParse::Value(*Params, TEXT("-dialoguecsv="), DialogueCSVPath))
	{
		DialogueCSVPath = DialogueCSVPath.TrimQuotes();
	}
	else
	{
		const FString* DialogueCSVVal = ParamVals.Find(TEXT("dialoguecsv"));
		if (DialogueCSVVal)
		{
			DialogueCSVPath = DialogueCSVVal->TrimQuotes();
		}
	}

	if (!DialogueCSVPath.IsEmpty())
	{
		// Resolve relative path
		if (FPaths::IsRelative(DialogueCSVPath))
		{
			DialogueCSVPath = FPaths::GetPath(ManifestPath) / DialogueCSVPath;
		}
		FPaths::NormalizeFilename(DialogueCSVPath);

		if (FPaths::FileExists(DialogueCSVPath))
		{
			LogMessage(FString::Printf(TEXT("Parsing dialogue CSV: %s"), *DialogueCSVPath));

			TArray<FManifestDialogueBlueprintDefinition> CSVDialogues;
			if (FDialogueCSVParser::ParseCSVFile(DialogueCSVPath, CSVDialogues))
			{
				LogMessage(FString::Printf(TEXT("Loaded %d dialogues from CSV"), CSVDialogues.Num()));

				// Append to manifest data (CSV dialogues take precedence over YAML)
				for (const auto& Dialogue : CSVDialogues)
				{
					// Remove existing definition with same name (CSV overrides YAML)
					ManifestData.DialogueBlueprints.RemoveAll([&](const FManifestDialogueBlueprintDefinition& Existing) {
						return Existing.Name == Dialogue.Name;
					});
					ManifestData.DialogueBlueprints.Add(Dialogue);
				}
			}
			else
			{
				LogError(FString::Printf(TEXT("WARNING: Failed to parse dialogue CSV: %s"), *DialogueCSVPath));
			}
		}
		else
		{
			LogError(FString::Printf(TEXT("WARNING: Dialogue CSV not found: %s"), *DialogueCSVPath));
		}
	}

	// v3.9.9: Log POI and Spawner counts
	if (ManifestData.POIPlacements.Num() > 0 || ManifestData.NPCSpawnerPlacements.Num() > 0)
	{
		LogMessage(FString::Printf(TEXT("Level actors: %d POIs, %d NPC Spawners"),
			ManifestData.POIPlacements.Num(),
			ManifestData.NPCSpawnerPlacements.Num()));
	}
	LogMessage(TEXT(""));

	// v3.9.9: Load world if level path specified and we have level actors to place
	UWorld* TargetWorld = nullptr;
	bool bNeedsLevelPlacement = ManifestData.POIPlacements.Num() > 0 || ManifestData.NPCSpawnerPlacements.Num() > 0;

	if (!LevelPath.IsEmpty() && bNeedsLevelPlacement)
	{
		LogMessage(TEXT("--- Loading Level for Actor Placement ---"));

		// Resolve the level path
		FString LongPackageName;
		if (!FPackageName::SearchForPackageOnDisk(LevelPath, &LongPackageName))
		{
			// Try as content path directly
			LongPackageName = LevelPath;
		}

		LogMessage(FString::Printf(TEXT("Loading level package: %s"), *LongPackageName));

		// Load the world package
		UPackage* WorldPackage = LoadPackage(nullptr, *LongPackageName, LOAD_None);
		if (WorldPackage)
		{
			// Find the world in the package
			TargetWorld = UWorld::FindWorldInPackage(WorldPackage);
			if (TargetWorld)
			{
				LoadedWorld = TargetWorld;
				LogMessage(FString::Printf(TEXT("Successfully loaded world: %s"), *TargetWorld->GetName()));
			}
			else
			{
				LogError(TEXT("Failed to find world in package"));
			}
		}
		else
		{
			LogError(FString::Printf(TEXT("Failed to load level package: %s"), *LongPackageName));
		}
		LogMessage(TEXT(""));
	}
	else if (bNeedsLevelPlacement && LevelPath.IsEmpty())
	{
		LogMessage(TEXT("WARNING: POI/Spawner placements defined but no -level parameter specified. Level actors will NOT be placed."));
		LogMessage(TEXT("Use -level=\"/Game/Maps/YourLevel\" to enable level actor placement."));
		LogMessage(TEXT(""));
	}

	// Generate tags
	if (bGenerateTags)
	{
		GenerateTags(ManifestData);
	}

	// Generate assets
	if (bGenerateAssets)
	{
		// v4.16.1: Clear hash collision map at start of generation session
		UGeneratorMetadataRegistry* Registry = UGeneratorMetadataRegistry::GetOrCreateRegistry();
		if (Registry)
		{
			Registry->ClearCollisionMap();
		}

		GenerateAssets(ManifestData);
	}

	// v3.9.9: Generate level actors if world is loaded
	if (TargetWorld && bNeedsLevelPlacement)
	{
		GenerateLevelActors(ManifestData, TargetWorld);
	}

	// v3.0: Print dry run summary if in dry run mode
	if (FGeneratorBase::IsDryRunMode())
	{
		LogMessage(TEXT(""));
		LogMessage(TEXT("========================================"));
		LogMessage(TEXT("DRY RUN REPORT"));
		LogMessage(TEXT("========================================"));

		const FDryRunSummary& Summary = FGeneratorBase::GetDryRunSummary();
		LogMessage(Summary.GetSummary());

		if (Summary.CreateCount > 0)
		{
			LogMessage(TEXT(""));
			LogMessage(FString::Printf(TEXT("--- CREATE (%d new assets) ---"), Summary.CreateCount));
			for (const FDryRunResult& Result : Summary.GetResultsByStatus(EDryRunStatus::WillCreate))
			{
				LogMessage(Result.ToString());
			}
		}

		if (Summary.ModifyCount > 0)
		{
			LogMessage(TEXT(""));
			LogMessage(FString::Printf(TEXT("--- MODIFY (%d manifest changes, no manual edits) ---"), Summary.ModifyCount));
			for (const FDryRunResult& Result : Summary.GetResultsByStatus(EDryRunStatus::WillModify))
			{
				LogMessage(Result.ToString());
			}
		}

		if (Summary.ConflictCount > 0)
		{
			LogMessage(TEXT(""));
			LogMessage(FString::Printf(TEXT("--- CONFLICTS (%d require attention) ---"), Summary.ConflictCount));
			for (const FDryRunResult& Result : Summary.GetResultsByStatus(EDryRunStatus::Conflicted))
			{
				LogMessage(Result.ToString());
				LogMessage(FString::Printf(TEXT("  Input hash: stored=%llu, current=%llu"),
					Result.StoredInputHash, Result.CurrentInputHash));
				LogMessage(FString::Printf(TEXT("  Output hash: stored=%llu, current=%llu"),
					Result.StoredOutputHash, Result.CurrentOutputHash));
				LogMessage(TEXT("  Action: Use --force to override, or resolve manually"));
			}
		}

		if (Summary.SkipCount > 0)
		{
			LogMessage(TEXT(""));
			LogMessage(FString::Printf(TEXT("--- SKIP (%d unchanged) ---"), Summary.SkipCount));
			// Only show first 10 skipped to avoid cluttering output
			int32 SkipShown = 0;
			for (const FDryRunResult& Result : Summary.GetResultsByStatus(EDryRunStatus::WillSkip))
			{
				if (SkipShown++ < 10)
				{
					LogMessage(Result.ToString());
				}
			}
			if (Summary.SkipCount > 10)
			{
				LogMessage(FString::Printf(TEXT("... and %d more skipped assets"), Summary.SkipCount - 10));
			}
		}
	}

	// v4.7: Create and save dry-run report
	if (FGeneratorBase::IsDryRunMode())
	{
		const FDryRunSummary& DryRunSummary = FGeneratorBase::GetDryRunSummary();
		FGenerationReportHelper::CreateAndSaveDryRunReport(CachedManifestPath, CachedManifestHash, DryRunSummary, bCachedForceMode);
	}

	// v3.0: Cleanup dry run and force modes
	FGeneratorBase::SetDryRunMode(false);
	FGeneratorBase::SetForceMode(false);
	FGeneratorBase::ClearDryRunSummary();

	// Save output log if specified
	if (!OutputLogPath.IsEmpty())
	{
		FString LogContent = FString::Join(LogMessages, TEXT("\n"));
		FFileHelper::SaveStringToFile(LogContent, *OutputLogPath);
		UE_LOG(LogGasAbilityGenerator, Display, TEXT("Log saved to: %s"), *OutputLogPath);
	}

	LogMessage(TEXT(""));
	LogMessage(TEXT("========================================"));
	LogMessage(TEXT("Generation Complete"));
	LogMessage(TEXT("========================================"));

	// v3.1: Return non-zero exit code if any failures occurred
	int32 ExitCode = 0;
	if (!FGeneratorBase::IsDryRunMode())
	{
		if (LastFailedCount > 0 || LastValidationErrorCount > 0 || bHadParseError)
		{
			ExitCode = 1;
			LogError(FString::Printf(TEXT("Exiting with code 1 (Failed: %d, Validation Errors: %d, Parse Errors: %s)"),
				LastFailedCount, LastValidationErrorCount, bHadParseError ? TEXT("Yes") : TEXT("No")));
		}
	}

	return ExitCode;
}

void UGasAbilityGeneratorCommandlet::GenerateTags(const FManifestData& ManifestData)
{
	LogMessage(TEXT("--- Generating Tags ---"));

	if (ManifestData.Tags.Num() == 0)
	{
		LogMessage(TEXT("No tags to generate"));
		return;
	}

	FString TagsIniPath = ManifestData.TagsIniPath;
	if (TagsIniPath.IsEmpty())
	{
		TagsIniPath = TEXT("Config/DefaultGameplayTags.ini");
	}

	// Make path absolute
	if (FPaths::IsRelative(TagsIniPath))
	{
		TagsIniPath = FPaths::Combine(FPaths::ProjectDir(), TagsIniPath);
	}
	FPaths::NormalizeFilename(TagsIniPath);

	LogMessage(FString::Printf(TEXT("Tags INI path: %s"), *TagsIniPath));
	LogMessage(FString::Printf(TEXT("Generating %d tags..."), ManifestData.Tags.Num()));

	FGenerationSummary Summary = FTagGenerator::GenerateTags(ManifestData.Tags, TagsIniPath);

	LogMessage(FString::Printf(TEXT("Tags: %d new, %d skipped, %d failed"),
		Summary.NewCount, Summary.SkippedCount, Summary.FailedCount));
	LogMessage(TEXT(""));
}

// v2.6.7: Helper lambda to process generation results
// v2.8.4: Also tracks ProcessedAssets for verification and detects duplicates
static auto ProcessResult = [](UGasAbilityGeneratorCommandlet* Self, FGenerationResult& Result,
	FGenerationSummary& Summary, TSet<FString>& GeneratedAssets, TSet<FString>& ProcessedAssets,
	TArray<FString>& GenerationDuplicates)
{
	Summary.AddResult(Result);

	// v2.8.4: Track ALL processed assets for verification, detect duplicates
	if (ProcessedAssets.Contains(Result.AssetName))
	{
		GenerationDuplicates.AddUnique(Result.AssetName);
		UE_LOG(LogGasAbilityGenerator, Error, TEXT("DUPLICATE GENERATION: %s"), *Result.AssetName);
	}
	else
	{
		ProcessedAssets.Add(Result.AssetName);
	}

	// Track generated/existing assets for dependency resolution
	if (Result.Status == EGenerationStatus::New || Result.Status == EGenerationStatus::Skipped)
	{
		GeneratedAssets.Add(Result.AssetName);
	}

	const TCHAR* StatusStr = TEXT("FAIL");
	switch (Result.Status)
	{
	case EGenerationStatus::New: StatusStr = TEXT("NEW"); break;
	case EGenerationStatus::Skipped: StatusStr = TEXT("SKIP"); break;
	case EGenerationStatus::Deferred: StatusStr = TEXT("DEFER"); break;
	case EGenerationStatus::Failed: StatusStr = TEXT("FAIL"); break;
	}

	UE_LOG(LogGasAbilityGenerator, Display, TEXT("[%s] %s"), StatusStr, *Result.AssetName);
};

void UGasAbilityGeneratorCommandlet::GenerateAssets(const FManifestData& ManifestData)
{
	LogMessage(TEXT("--- Generating Assets ---"));

	FGeneratorBase::SetActiveManifest(&ManifestData);
	FGenerationSummary Summary;

	// v2.6.7: Clear tracking for this run
	GeneratedAssets.Empty();
	DeferredAssets.Empty();
	// v2.8.4: Clear processed assets and duplicates for verification
	ProcessedAssets.Empty();
	GenerationDuplicates.Empty();
	// v4.9.1: Clear session cache for MIC parent material lookup
	FMaterialGenerator::ClearGeneratedMaterialsCache();
	// v4.14: Clear session cache for BT blackboard lookup
	FBlackboardGenerator::ClearGeneratedBlackboardsCache();

	// v2.8.4: Helper lambda to track processed assets with duplicate detection
	auto TrackProcessedAsset = [this](const FString& AssetName) {
		if (ProcessedAssets.Contains(AssetName))
		{
			GenerationDuplicates.AddUnique(AssetName);
			UE_LOG(LogGasAbilityGenerator, Error, TEXT("DUPLICATE GENERATION: %s"), *AssetName);
		}
		else
		{
			ProcessedAssets.Add(AssetName);
		}
	};

	// v2.8.4: Build expected asset whitelist at START for verification
	TArray<FString> DuplicateAssets;
	const TSet<FString> ExpectedAssets = ManifestData.GetExpectedAssetNames(&DuplicateAssets);
	const int32 ExpectedCount = ExpectedAssets.Num();

	// Check for duplicates in manifest
	if (DuplicateAssets.Num() > 0)
	{
		LogError(FString::Printf(TEXT("WARNING: Found %d duplicate asset names in manifest:"), DuplicateAssets.Num()));
		for (const FString& DupeName : DuplicateAssets)
		{
			LogError(FString::Printf(TEXT("  DUPLICATE: %s"), *DupeName));
		}
		LogMessage(TEXT(""));
	}

	LogMessage(FString::Printf(TEXT("Expected unique asset count from manifest: %d"), ExpectedCount));
	LogMessage(TEXT("Expected assets from manifest whitelist:"));
	for (const FString& AssetName : ExpectedAssets)
	{
		LogMessage(FString::Printf(TEXT("  - %s"), *AssetName));
	}
	LogMessage(TEXT(""));

	// PHASE 1: No Dependencies - Enumerations
	for (const auto& Definition : ManifestData.Enumerations)
	{
		FGenerationResult Result = FEnumerationGenerator::Generate(Definition);
		ProcessResult(this, Result, Summary, GeneratedAssets, ProcessedAssets, GenerationDuplicates);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
		if (Result.Status == EGenerationStatus::Failed)
		{
			LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
		}
	}

	// Float Curves
	for (const auto& Definition : ManifestData.FloatCurves)
	{
		FGenerationResult Result = FFloatCurveGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Input Actions
	for (const auto& Definition : ManifestData.InputActions)
	{
		FGenerationResult Result = FInputActionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Input Mapping Contexts
	for (const auto& Definition : ManifestData.InputMappingContexts)
	{
		FGenerationResult Result = FInputMappingContextGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 2: Base Assets - Gameplay Effects
	for (const auto& Definition : ManifestData.GameplayEffects)
	{
		FGenerationResult Result = FGameplayEffectGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 3: Blueprint Assets - Actor Blueprints (before abilities that reference them)
	for (int32 i = 0; i < ManifestData.ActorBlueprints.Num(); ++i)
	{
		const auto& Definition = ManifestData.ActorBlueprints[i];
		FGenerationResult Result = FActorBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.7: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("ActorBlueprint");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// Gameplay Abilities (after actor blueprints they may reference)
	for (int32 i = 0; i < ManifestData.GameplayAbilities.Num(); ++i)
	{
		const auto& Definition = ManifestData.GameplayAbilities[i];
		FGenerationResult Result = FGameplayAbilityGenerator::Generate(Definition, ManifestData.ProjectRoot);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.7: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("GameplayAbility");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// Widget Blueprints
	for (int32 i = 0; i < ManifestData.WidgetBlueprints.Num(); ++i)
	{
		const auto& Definition = ManifestData.WidgetBlueprints[i];
		FGenerationResult Result = FWidgetBlueprintGenerator::Generate(Definition, &ManifestData);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.7: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("WidgetBlueprint");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// Blackboards
	for (const auto& Definition : ManifestData.Blackboards)
	{
		FGenerationResult Result = FBlackboardGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Behavior Trees
	for (const auto& Definition : ManifestData.BehaviorTrees)
	{
		FGenerationResult Result = FBehaviorTreeGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Materials
	for (const auto& Definition : ManifestData.Materials)
	{
		FGenerationResult Result = FMaterialGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.12: Material Functions
	for (const auto& Definition : ManifestData.MaterialFunctions)
	{
		FGenerationResult Result = FMaterialFunctionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v4.9: Material Instances
	for (const auto& Definition : ManifestData.MaterialInstances)
	{
		FGenerationResult Result = FMaterialInstanceGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Tagged Dialogue Sets
	for (const auto& Definition : ManifestData.TaggedDialogueSets)
	{
		FGenerationResult Result = FTaggedDialogueSetGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Animation Montages
	for (const auto& Definition : ManifestData.AnimationMontages)
	{
		FGenerationResult Result = FAnimationMontageGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Animation Notifies (v4.0: Pass ManifestData for event graph reference lookup)
	for (const auto& Definition : ManifestData.AnimationNotifies)
	{
		FGenerationResult Result = FAnimationNotifyGenerator::Generate(Definition, &ManifestData);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Dialogue Blueprints
	for (const auto& Definition : ManifestData.DialogueBlueprints)
	{
		FGenerationResult Result = FDialogueBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Equippable Items (v2.6.9: with deferred handling)
	for (int32 i = 0; i < ManifestData.EquippableItems.Num(); ++i)
	{
		const auto& Definition = ManifestData.EquippableItems[i];
		FGenerationResult Result = FEquippableItemGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.9: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("EquippableItem");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// v2.6.0: Activities (v2.6.9: with deferred handling)
	for (int32 i = 0; i < ManifestData.Activities.Num(); ++i)
	{
		const auto& Definition = ManifestData.Activities[i];
		FGenerationResult Result = FActivityGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.9: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("Activity");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// v2.6.0: Ability Configurations
	for (const auto& Definition : ManifestData.AbilityConfigurations)
	{
		FGenerationResult Result = FAbilityConfigurationGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Activity Configurations
	for (const auto& Definition : ManifestData.ActivityConfigurations)
	{
		FGenerationResult Result = FActivityConfigurationGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Item Collections
	for (const auto& Definition : ManifestData.ItemCollections)
	{
		FGenerationResult Result = FItemCollectionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Narrative Events
	for (const auto& Definition : ManifestData.NarrativeEvents)
	{
		FGenerationResult Result = FNarrativeEventGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v4.0: Gameplay Cues
	for (const auto& Definition : ManifestData.GameplayCues)
	{
		FGenerationResult Result = FGameplayCueGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: NPC Definitions (v2.6.9: with deferred handling)
	for (int32 i = 0; i < ManifestData.NPCDefinitions.Num(); ++i)
	{
		const auto& Definition = ManifestData.NPCDefinitions[i];
		FGenerationResult Result = FNPCDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);

		// v2.6.9: Handle deferred assets
		if (Result.Status == EGenerationStatus::Deferred && Result.CanRetry())
		{
			FDeferredAsset Deferred;
			Deferred.AssetName = Definition.Name;
			Deferred.AssetType = TEXT("NPCDefinition");
			Deferred.MissingDependency = Result.MissingDependency;
			Deferred.MissingDependencyType = Result.MissingDependencyType;
			Deferred.DefinitionIndex = i;
			DeferredAssets.Add(Deferred);
			LogMessage(FString::Printf(TEXT("[DEFER] %s (waiting for %s)"), *Result.AssetName, *Result.MissingDependency));
		}
		else
		{
			LogMessage(FString::Printf(TEXT("[%s] %s"),
				Result.Status == EGenerationStatus::New ? TEXT("NEW") :
				Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
				*Result.AssetName));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
			if (Result.Status == EGenerationStatus::New)
			{
				GeneratedAssets.Add(Definition.Name);
			}
		}
	}

	// v2.6.0: Character Definitions
	for (const auto& Definition : ManifestData.CharacterDefinitions)
	{
		FGenerationResult Result = FCharacterDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v4.8.3: Character Appearances
	for (const auto& Definition : ManifestData.CharacterAppearances)
	{
		FGenerationResult Result = FCharacterAppearanceGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v4.9: TriggerSets (event-driven NPC behaviors)
	for (const auto& Definition : ManifestData.TriggerSets)
	{
		FGenerationResult Result = FTriggerSetGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.5: Niagara Systems
	for (const auto& Definition : ManifestData.NiagaraSystems)
	{
		// v4.9: Resolve FX Preset parameters before generation
		FManifestNiagaraSystemDefinition ResolvedDefinition = Definition;
		if (!Definition.Preset.IsEmpty())
		{
			// Build merged parameters from preset (with inheritance support)
			TMap<FString, FString> MergedParams;
			TSet<FString> ProcessedPresets;  // Prevent infinite loops
			FString CurrentPreset = Definition.Preset;

			while (!CurrentPreset.IsEmpty() && !ProcessedPresets.Contains(CurrentPreset))
			{
				ProcessedPresets.Add(CurrentPreset);
				const FManifestFXPresetDefinition* PresetDef = nullptr;
				for (const auto& Preset : ManifestData.FXPresets)
				{
					if (Preset.Name == CurrentPreset)
					{
						PresetDef = &Preset;
						break;
					}
				}

				if (PresetDef)
				{
					// Apply parameters (child presets override parent)
					for (const auto& Param : PresetDef->Parameters)
					{
						if (!MergedParams.Contains(Param.Key))
						{
							MergedParams.Add(Param.Key, Param.Value);
						}
					}
					CurrentPreset = PresetDef->BasePreset;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FX Preset '%s' not found for Niagara system '%s'"), *CurrentPreset, *Definition.Name);
					break;
				}
			}

			// Convert merged params to UserParameters (prepend so manifest user_parameters override)
			TArray<FManifestNiagaraUserParameter> PresetParams;
			for (const auto& Param : MergedParams)
			{
				// Check if this param is already in user_parameters - if so, skip (user overrides preset)
				bool bAlreadyDefined = false;
				for (const auto& UserParam : Definition.UserParameters)
				{
					if (UserParam.Name == Param.Key)
					{
						bAlreadyDefined = true;
						break;
					}
				}
				if (!bAlreadyDefined)
				{
					FManifestNiagaraUserParameter NewParam;
					NewParam.Name = Param.Key;
					NewParam.DefaultValue = Param.Value;
					// Infer type from value
					if (Param.Value.StartsWith(TEXT("[")) || Param.Value.Contains(TEXT(",")))
					{
						NewParam.Type = TEXT("LinearColor");  // Assume color/vector
					}
					else if (Param.Value.IsNumeric() || Param.Value.Contains(TEXT(".")))
					{
						NewParam.Type = TEXT("Float");
					}
					else
					{
						NewParam.Type = TEXT("Float");  // Default
					}
					PresetParams.Add(NewParam);
				}
			}
			// Prepend preset params to user params
			PresetParams.Append(ResolvedDefinition.UserParameters);
			ResolvedDefinition.UserParameters = PresetParams;
		}

		FGenerationResult Result = FNiagaraSystemGenerator::Generate(ResolvedDefinition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v3.9: Activity Schedules (NPC daily routines)
	for (const auto& Definition : ManifestData.ActivitySchedules)
	{
		FGenerationResult Result = FActivityScheduleGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v3.9: Goal Items (AI objectives)
	for (const auto& Definition : ManifestData.GoalItems)
	{
		FGenerationResult Result = FGoalItemGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v3.9: Quests (quest state machines)
	for (const auto& Definition : ManifestData.Quests)
	{
		FGenerationResult Result = FQuestGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v4.12: Mesh-to-Item Pipeline
	if (ManifestData.PipelineItems.Num() > 0)
	{
		LogMessage(TEXT(""));
		LogMessage(TEXT("--- Processing Pipeline Items ---"));

		FPipelineProcessor Pipeline;
		Pipeline.SetProjectRoot(ManifestData.ProjectRoot);

		for (const auto& ItemDef : ManifestData.PipelineItems)
		{
			// Convert manifest definition to pipeline input
			FPipelineMeshInput Input;
			Input.MeshPath = ItemDef.Mesh;
			Input.DisplayName = ItemDef.DisplayName;
			Input.EquipmentSlot = ItemDef.Slot;
			Input.TargetCollection = ItemDef.TargetCollection;

			// Convert type string to enum
			if (ItemDef.Type.Equals(TEXT("Weapon_Melee"), ESearchCase::IgnoreCase))
			{
				Input.ItemType = EPipelineItemType::Weapon_Melee;
			}
			else if (ItemDef.Type.Equals(TEXT("Weapon_Ranged"), ESearchCase::IgnoreCase))
			{
				Input.ItemType = EPipelineItemType::Weapon_Ranged;
			}
			else if (ItemDef.Type.Equals(TEXT("Consumable"), ESearchCase::IgnoreCase))
			{
				Input.ItemType = EPipelineItemType::Consumable;
			}
			else if (ItemDef.Type.Equals(TEXT("Generic"), ESearchCase::IgnoreCase))
			{
				Input.ItemType = EPipelineItemType::Generic;
			}
			else
			{
				Input.ItemType = EPipelineItemType::Clothing;  // Default
			}

			FPipelineProcessResult PipeResult = Pipeline.ProcessMesh(Input);

			// Create generation result from pipeline result
			FString ItemName = ItemDef.Name.IsEmpty() ? FPipelineMeshAnalyzer::GenerateItemAssetName(FPaths::GetBaseFilename(ItemDef.Mesh)) : ItemDef.Name;
			FGenerationResult Result(
				ItemName,
				PipeResult.bSuccess ? EGenerationStatus::New : EGenerationStatus::Failed,
				PipeResult.bSuccess ? TEXT("Pipeline item generated") : (PipeResult.Errors.Num() > 0 ? PipeResult.Errors[0] : TEXT("Pipeline failed"))
			);
			Result.GeneratorId = TEXT("Pipeline");
			Result.AssetPath = PipeResult.GeneratedItemPath;
			Result.DetermineCategory();

			Summary.AddResult(Result);
			if (!ItemName.IsEmpty())
			{
				TrackProcessedAsset(ItemName);
			}

			LogMessage(FString::Printf(TEXT("[%s] Pipeline: %s"),
				PipeResult.bSuccess ? TEXT("NEW") : TEXT("FAIL"),
				*ItemName));

			// Log warnings
			for (const FString& Warning : PipeResult.Warnings)
			{
				LogMessage(FString::Printf(TEXT("  [WARN] %s"), *Warning));
			}
		}

		LogMessage(FString::Printf(TEXT("Pipeline items processed: %d"), ManifestData.PipelineItems.Num()));
	}

	// v2.6.7: Process deferred assets (retry mechanism)
	if (DeferredAssets.Num() > 0)
	{
		ProcessDeferredAssets(ManifestData);
	}

	FGeneratorBase::ClearActiveManifest();

	// v2.8.4: Compute actual processed count
	const int32 ActualCount = Summary.NewCount + Summary.SkippedCount + Summary.FailedCount;

	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Summary ---"));
	LogMessage(FString::Printf(TEXT("New: %d"), Summary.NewCount));
	LogMessage(FString::Printf(TEXT("Skipped: %d"), Summary.SkippedCount));
	LogMessage(FString::Printf(TEXT("Failed: %d"), Summary.FailedCount));
	LogMessage(FString::Printf(TEXT("Deferred: %d"), Summary.DeferredCount));
	LogMessage(FString::Printf(TEXT("Total: %d"), ActualCount + Summary.DeferredCount));

	// v4.11: Machine-parseable RESULT footer for wrapper reliability
	// Schema v1: Keys in fixed order, all always present. Deferred = skipped due to missing prerequisite.
	// Format: RESULT: New=<N> Skipped=<N> Failed=<N> Deferred=<N> Total=<N> Headless=<true/false>
	const bool bIsHeadless = IsRunningCommandlet() && !FApp::CanEverRender();
	LogMessage(FString::Printf(TEXT("RESULT: New=%d Skipped=%d Failed=%d Deferred=%d Total=%d Headless=%s"),
		Summary.NewCount,
		Summary.SkippedCount,
		Summary.FailedCount,
		Summary.DeferredCount,
		ActualCount + Summary.DeferredCount,
		bIsHeadless ? TEXT("true") : TEXT("false")));

	// v4.11: RESULT_HEADLESS_SAVED footer for CI dashboards
	// Count of assets saved under headless escape hatch (subset of New, require editor verification)
	if (Summary.HeadlessSavedCount > 0)
	{
		LogMessage(FString::Printf(TEXT("RESULT_HEADLESS_SAVED: Count=%d"), Summary.HeadlessSavedCount));
	}

	// v2.8.4: Verification - compare actual vs expected using whitelist from start
	VerifyGenerationComplete(ExpectedAssets, ExpectedCount, ActualCount);

	// v3.1: Store results for exit code
	LastFailedCount = Summary.FailedCount;

	// v3.1: Save the metadata registry
	GeneratorMetadataHelpers::SaveRegistryIfNeeded();

	// v4.7: Create and save real-run report (only if not dry-run)
	if (!FGeneratorBase::IsDryRunMode())
	{
		FGenerationReportHelper::CreateAndSaveReport(CachedManifestPath, CachedManifestHash, Summary.Results, bCachedForceMode);
	}
}

void UGasAbilityGeneratorCommandlet::LogMessage(const FString& Message)
{
	UE_LOG(LogGasAbilityGenerator, Display, TEXT("%s"), *Message);
	LogMessages.Add(Message);
}

void UGasAbilityGeneratorCommandlet::LogError(const FString& Message)
{
	UE_LOG(LogGasAbilityGenerator, Error, TEXT("%s"), *Message);
	LogMessages.Add(FString::Printf(TEXT("ERROR: %s"), *Message));
}

// v2.6.7: Process deferred assets with retry mechanism
void UGasAbilityGeneratorCommandlet::ProcessDeferredAssets(const FManifestData& ManifestData)
{
	if (DeferredAssets.Num() == 0)
	{
		return;
	}

	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Processing Deferred Assets ---"));
	LogMessage(FString::Printf(TEXT("%d asset(s) deferred due to missing dependencies"), DeferredAssets.Num()));

	int32 Pass = 0;
	int32 ResolvedThisPass = 0;

	do
	{
		Pass++;
		ResolvedThisPass = 0;
		LogMessage(FString::Printf(TEXT("Retry pass %d..."), Pass));

		TArray<FDeferredAsset> StillDeferred;

		for (FDeferredAsset& Deferred : DeferredAssets)
		{
			// Check if dependency is now resolved
			if (IsDependencyResolved(Deferred.MissingDependency, Deferred.MissingDependencyType))
			{
				FGenerationResult Result;
				if (TryGenerateDeferredAsset(Deferred, ManifestData, Result))
				{
					LogMessage(FString::Printf(TEXT("[RETRY-OK] %s (dependency %s now available)"),
						*Deferred.AssetName, *Deferred.MissingDependency));
					GeneratedAssets.Add(Deferred.AssetName);
					ResolvedThisPass++;
				}
				else
				{
					// Still failed after retry
					Deferred.RetryCount++;
					if (Deferred.RetryCount < MaxRetryAttempts)
					{
						StillDeferred.Add(Deferred);
					}
					else
					{
						LogError(FString::Printf(TEXT("[RETRY-FAIL] %s - max retries exceeded: %s"),
							*Deferred.AssetName, *Result.Message));
					}
				}
			}
			else
			{
				// Dependency still not resolved
				Deferred.RetryCount++;
				if (Deferred.RetryCount < MaxRetryAttempts)
				{
					StillDeferred.Add(Deferred);
				}
				else
				{
					LogError(FString::Printf(TEXT("[UNRESOLVED] %s - dependency %s not found in manifest"),
						*Deferred.AssetName, *Deferred.MissingDependency));
				}
			}
		}

		DeferredAssets = StillDeferred;

	} while (ResolvedThisPass > 0 && DeferredAssets.Num() > 0 && Pass < MaxRetryAttempts);

	if (DeferredAssets.Num() > 0)
	{
		LogMessage(FString::Printf(TEXT("%d asset(s) could not be resolved after %d passes"),
			DeferredAssets.Num(), Pass));
	}
	else
	{
		LogMessage(TEXT("All deferred assets resolved successfully"));
	}
}

// v2.6.7: Check if a dependency has been generated
bool UGasAbilityGeneratorCommandlet::IsDependencyResolved(const FString& DependencyName, const FString& DependencyType) const
{
	// Check if it was generated in this session
	if (GeneratedAssets.Contains(DependencyName))
	{
		return true;
	}

	// Check if it exists on disk (may have been generated in a previous run)
	// This uses the generator base to check disk existence
	return false; // For now, only check session-generated assets
}

// v2.6.7: Try to generate a deferred asset
bool UGasAbilityGeneratorCommandlet::TryGenerateDeferredAsset(const FDeferredAsset& Deferred,
	const FManifestData& ManifestData, FGenerationResult& OutResult)
{
	// Route to appropriate generator based on asset type
	if (Deferred.AssetType == TEXT("ActorBlueprint"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.ActorBlueprints.Num())
		{
			OutResult = FActorBlueprintGenerator::Generate(
				ManifestData.ActorBlueprints[Deferred.DefinitionIndex],
				ManifestData.ProjectRoot, &ManifestData);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	else if (Deferred.AssetType == TEXT("GameplayAbility"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.GameplayAbilities.Num())
		{
			OutResult = FGameplayAbilityGenerator::Generate(
				ManifestData.GameplayAbilities[Deferred.DefinitionIndex],
				ManifestData.ProjectRoot);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	else if (Deferred.AssetType == TEXT("GameplayEffect"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.GameplayEffects.Num())
		{
			OutResult = FGameplayEffectGenerator::Generate(
				ManifestData.GameplayEffects[Deferred.DefinitionIndex]);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	else if (Deferred.AssetType == TEXT("WidgetBlueprint"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.WidgetBlueprints.Num())
		{
			OutResult = FWidgetBlueprintGenerator::Generate(
				ManifestData.WidgetBlueprints[Deferred.DefinitionIndex], &ManifestData);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	// v2.6.9: EquippableItem retry handling
	else if (Deferred.AssetType == TEXT("EquippableItem"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.EquippableItems.Num())
		{
			OutResult = FEquippableItemGenerator::Generate(
				ManifestData.EquippableItems[Deferred.DefinitionIndex]);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	// v2.6.9: Activity retry handling
	else if (Deferred.AssetType == TEXT("Activity"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.Activities.Num())
		{
			OutResult = FActivityGenerator::Generate(
				ManifestData.Activities[Deferred.DefinitionIndex]);
			return OutResult.Status == EGenerationStatus::New;
		}
	}
	// v2.6.9: NPCDefinition retry handling
	else if (Deferred.AssetType == TEXT("NPCDefinition"))
	{
		if (Deferred.DefinitionIndex >= 0 && Deferred.DefinitionIndex < ManifestData.NPCDefinitions.Num())
		{
			OutResult = FNPCDefinitionGenerator::Generate(
				ManifestData.NPCDefinitions[Deferred.DefinitionIndex]);
			return OutResult.Status == EGenerationStatus::New;
		}
	}

	OutResult = FGenerationResult(Deferred.AssetName, EGenerationStatus::Failed,
		FString::Printf(TEXT("Unknown asset type: %s"), *Deferred.AssetType));
	return false;
}

// v2.8.4: Verify generation completeness using whitelist from start
void UGasAbilityGeneratorCommandlet::VerifyGenerationComplete(const TSet<FString>& ExpectedAssets, int32 ExpectedCount, int32 ActualCount)
{
	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Verification (Whitelist Check) ---"));
	LogMessage(FString::Printf(TEXT("Expected (from whitelist): %d"), ExpectedCount));
	LogMessage(FString::Printf(TEXT("Processed: %d"), ActualCount));

	// Find missing assets (in whitelist but not processed)
	TArray<FString> MissingAssets;
	for (const FString& ExpectedName : ExpectedAssets)
	{
		if (!ProcessedAssets.Contains(ExpectedName))
		{
			MissingAssets.Add(ExpectedName);
		}
	}

	// Find unexpected assets (processed but not in whitelist)
	TArray<FString> UnexpectedAssets;
	for (const FString& ProcessedName : ProcessedAssets)
	{
		if (!ExpectedAssets.Contains(ProcessedName))
		{
			UnexpectedAssets.Add(ProcessedName);
		}
	}

	// Report results
	const bool bHasDuplicates = GenerationDuplicates.Num() > 0;
	if (MissingAssets.Num() == 0 && UnexpectedAssets.Num() == 0 && ActualCount == ExpectedCount && !bHasDuplicates)
	{
		LogMessage(TEXT(""));
		LogMessage(TEXT("VERIFICATION PASSED: All whitelist assets processed, counts match, no duplicates"));
		return;
	}

	// Verification failed
	LogError(TEXT("VERIFICATION FAILED"));

	if (MissingAssets.Num() > 0)
	{
		LogMessage(TEXT(""));
		LogError(FString::Printf(TEXT("Missing assets (%d) - in whitelist but NOT processed:"), MissingAssets.Num()));
		for (const FString& Name : MissingAssets)
		{
			LogError(FString::Printf(TEXT("  - %s"), *Name));
		}
	}

	if (UnexpectedAssets.Num() > 0)
	{
		LogMessage(TEXT(""));
		LogMessage(FString::Printf(TEXT("Unexpected assets (%d) - processed but NOT in whitelist:"), UnexpectedAssets.Num()));
		for (const FString& Name : UnexpectedAssets)
		{
			LogMessage(FString::Printf(TEXT("  + %s"), *Name));
		}
	}

	if (bHasDuplicates)
	{
		LogMessage(TEXT(""));
		LogError(FString::Printf(TEXT("Generation duplicates (%d) - processed more than once:"), GenerationDuplicates.Num()));
		for (const FString& Name : GenerationDuplicates)
		{
			LogError(FString::Printf(TEXT("  ! %s"), *Name));
		}
	}

	LogMessage(TEXT(""));
	LogMessage(FString::Printf(TEXT("Verification Summary: %d missing, %d unexpected, %d duplicates, count diff: %d"),
		MissingAssets.Num(), UnexpectedAssets.Num(), GenerationDuplicates.Num(), ExpectedCount - ActualCount));
}

// v3.9.9: Generate level actors (POIs and NPC Spawners)
void UGasAbilityGeneratorCommandlet::GenerateLevelActors(const FManifestData& ManifestData, UWorld* TargetWorld)
{
	if (!TargetWorld)
	{
		LogError(TEXT("Cannot generate level actors: No world loaded"));
		return;
	}

	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Generating Level Actors ---"));
	LogMessage(FString::Printf(TEXT("Target World: %s"), *TargetWorld->GetName()));

	int32 POINewCount = 0;
	int32 POISkipCount = 0;
	int32 POIFailCount = 0;
	int32 SpawnerNewCount = 0;
	int32 SpawnerSkipCount = 0;
	int32 SpawnerFailCount = 0;

	// Track placed POIs for NearPOI resolution and LinkedPOI second pass
	TMap<FString, APOIActor*> PlacedPOIs;

	// Phase 1: Place POI actors
	if (ManifestData.POIPlacements.Num() > 0)
	{
		LogMessage(TEXT(""));
		LogMessage(FString::Printf(TEXT("Placing %d POI actors..."), ManifestData.POIPlacements.Num()));

		for (const FManifestPOIPlacement& POIDef : ManifestData.POIPlacements)
		{
			FGenerationResult Result = FPOIPlacementGenerator::Generate(POIDef, TargetWorld, PlacedPOIs);

			const TCHAR* StatusStr = TEXT("FAIL");
			switch (Result.Status)
			{
			case EGenerationStatus::New:
				StatusStr = TEXT("NEW");
				POINewCount++;
				break;
			case EGenerationStatus::Skipped:
				StatusStr = TEXT("SKIP");
				POISkipCount++;
				break;
			case EGenerationStatus::Failed:
				StatusStr = TEXT("FAIL");
				POIFailCount++;
				break;
			default:
				break;
			}

			LogMessage(FString::Printf(TEXT("[%s] POI: %s"), StatusStr, *POIDef.POITag));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
		}

		// Second pass: Resolve LinkedPOIs
		LogMessage(TEXT("Resolving POI links..."));
		for (const FManifestPOIPlacement& POIDef : ManifestData.POIPlacements)
		{
			if (POIDef.LinkedPOIs.Num() == 0)
			{
				continue;
			}

			APOIActor** FoundPOI = PlacedPOIs.Find(POIDef.POITag);
			if (!FoundPOI || !*FoundPOI)
			{
				continue;
			}

			APOIActor* POIActor = *FoundPOI;

			// Get LinkedPOIs property via reflection
			FArrayProperty* LinkedPOIsProp = CastField<FArrayProperty>(APOIActor::StaticClass()->FindPropertyByName(TEXT("LinkedPOIs")));
			if (LinkedPOIsProp)
			{
				FScriptArrayHelper ArrayHelper(LinkedPOIsProp, LinkedPOIsProp->ContainerPtrToValuePtr<void>(POIActor));
				ArrayHelper.EmptyValues();

				for (const FString& LinkedTag : POIDef.LinkedPOIs)
				{
					APOIActor** LinkedPOI = PlacedPOIs.Find(LinkedTag);
					if (LinkedPOI && *LinkedPOI)
					{
						int32 Index = ArrayHelper.AddValue();
						FObjectProperty* InnerProp = CastField<FObjectProperty>(LinkedPOIsProp->Inner);
						if (InnerProp)
						{
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(Index), *LinkedPOI);
						}
						LogMessage(FString::Printf(TEXT("  Linked: %s -> %s"), *POIDef.POITag, *LinkedTag));
					}
					else
					{
						LogMessage(FString::Printf(TEXT("  Warning: Could not find linked POI: %s"), *LinkedTag));
					}
				}
			}
		}
	}

	// Phase 2: Place NPC Spawner actors
	if (ManifestData.NPCSpawnerPlacements.Num() > 0)
	{
		LogMessage(TEXT(""));
		LogMessage(FString::Printf(TEXT("Placing %d NPC Spawner actors..."), ManifestData.NPCSpawnerPlacements.Num()));

		for (const FManifestNPCSpawnerPlacement& SpawnerDef : ManifestData.NPCSpawnerPlacements)
		{
			FGenerationResult Result = FNPCSpawnerPlacementGenerator::Generate(SpawnerDef, TargetWorld, PlacedPOIs);

			const TCHAR* StatusStr = TEXT("FAIL");
			switch (Result.Status)
			{
			case EGenerationStatus::New:
				StatusStr = TEXT("NEW");
				SpawnerNewCount++;
				break;
			case EGenerationStatus::Skipped:
				StatusStr = TEXT("SKIP");
				SpawnerSkipCount++;
				break;
			case EGenerationStatus::Failed:
				StatusStr = TEXT("FAIL");
				SpawnerFailCount++;
				break;
			default:
				break;
			}

			LogMessage(FString::Printf(TEXT("[%s] Spawner: %s"), StatusStr, *SpawnerDef.Name));
			if (Result.Status == EGenerationStatus::Failed)
			{
				LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
			}
		}
	}

	// Summary
	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Level Actor Summary ---"));
	LogMessage(FString::Printf(TEXT("POIs: %d new, %d skipped, %d failed"), POINewCount, POISkipCount, POIFailCount));
	LogMessage(FString::Printf(TEXT("Spawners: %d new, %d skipped, %d failed"), SpawnerNewCount, SpawnerSkipCount, SpawnerFailCount));

	// Save the world if any actors were placed
	if (POINewCount > 0 || SpawnerNewCount > 0)
	{
		if (!FGeneratorBase::IsDryRunMode())
		{
			SaveWorldPackage(TargetWorld);
		}
		else
		{
			LogMessage(TEXT("Dry run: World would be saved"));
		}
	}
	else
	{
		LogMessage(TEXT("No new actors placed, world not modified"));
	}
}

// v3.9.9: Save the world package after actor placement
void UGasAbilityGeneratorCommandlet::SaveWorldPackage(UWorld* World)
{
	if (!World)
	{
		LogError(TEXT("Cannot save world: World is null"));
		return;
	}

	UPackage* Package = World->GetOutermost();
	if (!Package)
	{
		LogError(TEXT("Cannot save world: Package is null"));
		return;
	}

	LogMessage(TEXT(""));
	LogMessage(TEXT("Saving world package..."));

	// Mark the package dirty
	Package->MarkPackageDirty();

	// Get the package filename
	FString PackageFileName;
	if (FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFileName, FPackageName::GetMapPackageExtension()))
	{
		// Save the package
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.Error = GWarn;

		FSavePackageResultStruct Result = UPackage::Save(Package, World, *PackageFileName, SaveArgs);

		if (Result.Result == ESavePackageResult::Success)
		{
			LogMessage(FString::Printf(TEXT("World saved successfully: %s"), *PackageFileName));
		}
		else
		{
			LogError(FString::Printf(TEXT("Failed to save world: %s"), *PackageFileName));
		}
	}
	else
	{
		LogError(FString::Printf(TEXT("Failed to resolve package filename for: %s"), *Package->GetName()));
	}
}
