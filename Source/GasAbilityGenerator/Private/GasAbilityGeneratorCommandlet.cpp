// GasAbilityGeneratorCommandlet.cpp
// Commandlet for automated asset generation from command line

#include "GasAbilityGeneratorCommandlet.h"
#include "GasAbilityGeneratorParser.h"
#include "GasAbilityGeneratorGenerators.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

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
	LogMessage(TEXT("GasAbilityGenerator Commandlet v2.8.4"));
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
	LogMessage(TEXT(""));

	// Read manifest file
	LogMessage(TEXT("Reading manifest file..."));
	FString ManifestContent;
	if (!FFileHelper::LoadFileToString(ManifestContent, *ManifestPath))
	{
		LogError(FString::Printf(TEXT("ERROR: Failed to read manifest file: %s"), *ManifestPath));
		return 1;
	}

	// Parse manifest
	LogMessage(TEXT("Parsing manifest..."));
	FManifestData ManifestData;

	if (!FGasAbilityGeneratorParser::ParseManifest(ManifestContent, ManifestData))
	{
		LogError(TEXT("ERROR: Failed to parse manifest file"));
		return 1;
	}

	LogMessage(FString::Printf(TEXT("Parsed manifest with %d tags, %d enumerations, %d abilities, %d effects, %d blueprints"),
		ManifestData.Tags.Num(),
		ManifestData.Enumerations.Num(),
		ManifestData.GameplayAbilities.Num(),
		ManifestData.GameplayEffects.Num(),
		ManifestData.ActorBlueprints.Num()));
	LogMessage(TEXT(""));

	// Generate tags
	if (bGenerateTags)
	{
		GenerateTags(ManifestData);
	}

	// Generate assets
	if (bGenerateAssets)
	{
		GenerateAssets(ManifestData);
	}

	// Save output log if specified
	if (!OutputLogPath.IsEmpty())
	{
		FString LogContent = FString::Join(LogMessages, TEXT("\n"));
		FFileHelper::SaveStringToFile(LogContent, *OutputLogPath);
		UE_LOG(LogTemp, Log, TEXT("Log saved to: %s"), *OutputLogPath);
	}

	LogMessage(TEXT(""));
	LogMessage(TEXT("========================================"));
	LogMessage(TEXT("Generation Complete"));
	LogMessage(TEXT("========================================"));

	return 0;
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
		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] DUPLICATE GENERATION: %s"), *Result.AssetName);
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

	UE_LOG(LogTemp, Display, TEXT("[GasAbilityGenerator] [%s] %s"), StatusStr, *Result.AssetName);
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

	// v2.8.4: Helper lambda to track processed assets with duplicate detection
	auto TrackProcessedAsset = [this](const FString& AssetName) {
		if (ProcessedAssets.Contains(AssetName))
		{
			GenerationDuplicates.AddUnique(AssetName);
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] DUPLICATE GENERATION: %s"), *AssetName);
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

	// v2.6.0: Animation Notifies
	for (const auto& Definition : ManifestData.AnimationNotifies)
	{
		FGenerationResult Result = FAnimationNotifyGenerator::Generate(Definition);
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

	// v2.6.5: Niagara Systems
	for (const auto& Definition : ManifestData.NiagaraSystems)
	{
		FGenerationResult Result = FNiagaraSystemGenerator::Generate(Definition);
		Summary.AddResult(Result);
		TrackProcessedAsset(Result.AssetName);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
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

	// v2.8.4: Verification - compare actual vs expected using whitelist from start
	VerifyGenerationComplete(ExpectedAssets, ExpectedCount, ActualCount);
}

void UGasAbilityGeneratorCommandlet::LogMessage(const FString& Message)
{
	UE_LOG(LogTemp, Display, TEXT("[GasAbilityGenerator] %s"), *Message);
	LogMessages.Add(Message);
}

void UGasAbilityGeneratorCommandlet::LogError(const FString& Message)
{
	UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] %s"), *Message);
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
