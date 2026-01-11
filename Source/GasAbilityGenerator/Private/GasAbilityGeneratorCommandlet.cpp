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
	LogMessage(TEXT("GasAbilityGenerator Commandlet v2.5.7"));
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

void UGasAbilityGeneratorCommandlet::GenerateAssets(const FManifestData& ManifestData)
{
	LogMessage(TEXT("--- Generating Assets ---"));

	FGeneratorBase::SetActiveManifest(&ManifestData);
	FGenerationSummary Summary;

	// PHASE 1: No Dependencies - Enumerations
	for (const auto& Definition : ManifestData.Enumerations)
	{
		FGenerationResult Result = FEnumerationGenerator::Generate(Definition);
		Summary.AddResult(Result);
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
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 3: Blueprint Assets - Actor Blueprints (before abilities that reference them)
	for (const auto& Definition : ManifestData.ActorBlueprints)
	{
		FGenerationResult Result = FActorBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
		if (Result.Status == EGenerationStatus::Failed)
		{
			LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
		}
	}

	// Gameplay Abilities (after actor blueprints they may reference)
	for (const auto& Definition : ManifestData.GameplayAbilities)
	{
		FGenerationResult Result = FGameplayAbilityGenerator::Generate(Definition, ManifestData.ProjectRoot);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
		if (Result.Status == EGenerationStatus::Failed)
		{
			LogError(FString::Printf(TEXT("  Error: %s"), *Result.Message));
		}
	}

	// Widget Blueprints
	for (const auto& Definition : ManifestData.WidgetBlueprints)
	{
		FGenerationResult Result = FWidgetBlueprintGenerator::Generate(Definition, &ManifestData);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Blackboards
	for (const auto& Definition : ManifestData.Blackboards)
	{
		FGenerationResult Result = FBlackboardGenerator::Generate(Definition);
		Summary.AddResult(Result);
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
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Equippable Items
	for (const auto& Definition : ManifestData.EquippableItems)
	{
		FGenerationResult Result = FEquippableItemGenerator::Generate(Definition);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Activities
	for (const auto& Definition : ManifestData.Activities)
	{
		FGenerationResult Result = FActivityGenerator::Generate(Definition);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Ability Configurations
	for (const auto& Definition : ManifestData.AbilityConfigurations)
	{
		FGenerationResult Result = FAbilityConfigurationGenerator::Generate(Definition);
		Summary.AddResult(Result);
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
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: NPC Definitions
	for (const auto& Definition : ManifestData.NPCDefinitions)
	{
		FGenerationResult Result = FNPCDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// v2.6.0: Character Definitions
	for (const auto& Definition : ManifestData.CharacterDefinitions)
	{
		FGenerationResult Result = FCharacterDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
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
		LogMessage(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	FGeneratorBase::ClearActiveManifest();

	LogMessage(TEXT(""));
	LogMessage(TEXT("--- Summary ---"));
	LogMessage(FString::Printf(TEXT("New: %d"), Summary.NewCount));
	LogMessage(FString::Printf(TEXT("Skipped: %d"), Summary.SkippedCount));
	LogMessage(FString::Printf(TEXT("Failed: %d"), Summary.FailedCount));
	LogMessage(FString::Printf(TEXT("Total: %d"), Summary.NewCount + Summary.SkippedCount + Summary.FailedCount));
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
