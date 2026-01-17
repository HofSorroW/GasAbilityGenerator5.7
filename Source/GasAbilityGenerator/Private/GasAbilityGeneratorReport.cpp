// GasAbilityGeneratorReport.cpp
// v4.7: Machine-readable generation report implementation

#include "GasAbilityGeneratorReport.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#define GENERATOR_VERSION TEXT("4.7")

// ============================================================================
// FGenerationReportItem Implementation
// ============================================================================

FGenerationReportItem FGenerationReportItem::FromGenerationResult(const FGenerationResult& Result)
{
	FGenerationReportItem Item;
	Item.AssetPath = Result.AssetPath;
	Item.AssetName = Result.AssetName;
	Item.GeneratorId = Result.GeneratorId;
	Item.ExecutedStatus = UGenerationReport::GetStatusString(Result.Status);
	Item.bHasPlannedStatus = false;
	Item.Reason = Result.Message;
	return Item;
}

FGenerationReportItem FGenerationReportItem::FromDryRunResult(const FDryRunResult& Result)
{
	FGenerationReportItem Item;
	Item.AssetPath = Result.AssetPath;
	Item.AssetName = Result.AssetName;
	Item.GeneratorId = Result.GeneratorId;
	Item.PlannedStatus = UGenerationReport::GetDryRunStatusString(Result.Status);
	Item.bHasPlannedStatus = true;
	Item.Reason = Result.Reason;

	// Convert manifest/asset changes to warnings
	for (const FString& Change : Result.ManifestChanges)
	{
		Item.Warnings.Add(FString::Printf(TEXT("Manifest change: %s"), *Change));
	}
	for (const FString& Change : Result.AssetChanges)
	{
		Item.Warnings.Add(FString::Printf(TEXT("Asset change: %s"), *Change));
	}

	return Item;
}

// ============================================================================
// UGenerationReport Implementation
// ============================================================================

void UGenerationReport::Initialize(const FString& InManifestPath, int64 InManifestHash, bool bInDryRun, bool bInForce)
{
	RunId = FGuid::NewGuid();
	Timestamp = FDateTime::Now();
	GeneratorVersion = GENERATOR_VERSION;
	ManifestFilePath = InManifestPath;
	ManifestHash = InManifestHash;
	bIsDryRun = bInDryRun;
	bIsForceRun = bInForce;
}

void UGenerationReport::AddItem(const FGenerationResult& Result)
{
	FGenerationReportItem Item = FGenerationReportItem::FromGenerationResult(Result);

	// Add structured error for failed items
	if (Result.Status == EGenerationStatus::Failed && !Result.Message.IsEmpty())
	{
		FGenerationError Error;
		Error.ErrorCode = TEXT("E_GENERATION_FAILED");
		Error.ContextPath = Result.AssetPath.IsEmpty() ? Result.AssetName : Result.AssetPath;
		Error.Message = Result.Message;
		Error.SuggestedFix = TEXT("Check manifest definition and dependencies");
		Item.Errors.Add(Error);
	}

	Items.Add(Item);
}

void UGenerationReport::AddDryRunItem(const FDryRunResult& Result)
{
	FGenerationReportItem Item = FGenerationReportItem::FromDryRunResult(Result);

	// Add structured error for conflicted items
	if (Result.Status == EDryRunStatus::Conflicted)
	{
		FGenerationError Error;
		Error.ErrorCode = TEXT("E_CONFLICT");
		Error.ContextPath = Result.AssetPath.IsEmpty() ? Result.AssetName : Result.AssetPath;
		Error.Message = FString::Printf(TEXT("Manifest and asset both changed. Input hash: stored=%llu, current=%llu. Output hash: stored=%llu, current=%llu"),
			Result.StoredInputHash, Result.CurrentInputHash, Result.StoredOutputHash, Result.CurrentOutputHash);
		Error.SuggestedFix = TEXT("Use --force to override or resolve manually");
		Item.Errors.Add(Error);
	}

	Items.Add(Item);
}

void UGenerationReport::Finalize()
{
	// Reset counts
	CountNew = 0;
	CountSkipped = 0;
	CountFailed = 0;
	CountDeferred = 0;
	CountWillCreate = 0;
	CountWillModify = 0;
	CountWillSkip = 0;
	CountConflicted = 0;

	for (const FGenerationReportItem& Item : Items)
	{
		if (Item.bHasPlannedStatus)
		{
			// Dry-run counts
			if (Item.PlannedStatus == TEXT("WillCreate")) CountWillCreate++;
			else if (Item.PlannedStatus == TEXT("WillModify")) CountWillModify++;
			else if (Item.PlannedStatus == TEXT("WillSkip")) CountWillSkip++;
			else if (Item.PlannedStatus == TEXT("Conflicted")) CountConflicted++;
		}
		else
		{
			// Real-run counts
			if (Item.ExecutedStatus == TEXT("New")) CountNew++;
			else if (Item.ExecutedStatus == TEXT("Skipped")) CountSkipped++;
			else if (Item.ExecutedStatus == TEXT("Failed")) CountFailed++;
			else if (Item.ExecutedStatus == TEXT("Deferred")) CountDeferred++;
		}
	}
}

bool UGenerationReport::SaveAsJson(const FString& OutputPath) const
{
	// Convert to JSON using FJsonObjectConverter
	FString JsonString;
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	// Add header fields
	JsonObject->SetStringField(TEXT("runId"), RunId.ToString());
	JsonObject->SetStringField(TEXT("timestamp"), Timestamp.ToIso8601());
	JsonObject->SetStringField(TEXT("generatorVersion"), GeneratorVersion);
	JsonObject->SetStringField(TEXT("manifestFilePath"), ManifestFilePath);
	JsonObject->SetNumberField(TEXT("manifestHash"), static_cast<double>(ManifestHash));
	JsonObject->SetBoolField(TEXT("isDryRun"), bIsDryRun);
	JsonObject->SetBoolField(TEXT("isForceRun"), bIsForceRun);

	// Add counts
	JsonObject->SetNumberField(TEXT("countNew"), CountNew);
	JsonObject->SetNumberField(TEXT("countSkipped"), CountSkipped);
	JsonObject->SetNumberField(TEXT("countFailed"), CountFailed);
	JsonObject->SetNumberField(TEXT("countDeferred"), CountDeferred);
	JsonObject->SetNumberField(TEXT("countWillCreate"), CountWillCreate);
	JsonObject->SetNumberField(TEXT("countWillModify"), CountWillModify);
	JsonObject->SetNumberField(TEXT("countWillSkip"), CountWillSkip);
	JsonObject->SetNumberField(TEXT("countConflicted"), CountConflicted);

	// Add items array
	TArray<TSharedPtr<FJsonValue>> ItemsArray;
	for (const FGenerationReportItem& Item : Items)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		ItemObject->SetStringField(TEXT("assetPath"), Item.AssetPath);
		ItemObject->SetStringField(TEXT("assetName"), Item.AssetName);
		ItemObject->SetStringField(TEXT("generatorId"), Item.GeneratorId);
		ItemObject->SetStringField(TEXT("executedStatus"), Item.ExecutedStatus);
		ItemObject->SetStringField(TEXT("plannedStatus"), Item.PlannedStatus);
		ItemObject->SetBoolField(TEXT("hasPlannedStatus"), Item.bHasPlannedStatus);
		ItemObject->SetBoolField(TEXT("assetExistedBeforeRun"), Item.bAssetExistedBeforeRun);
		ItemObject->SetStringField(TEXT("reason"), Item.Reason);

		// Add errors
		TArray<TSharedPtr<FJsonValue>> ErrorsArray;
		for (const FGenerationError& Error : Item.Errors)
		{
			TSharedRef<FJsonObject> ErrorObject = MakeShared<FJsonObject>();
			ErrorObject->SetStringField(TEXT("errorCode"), Error.ErrorCode);
			ErrorObject->SetStringField(TEXT("contextPath"), Error.ContextPath);
			ErrorObject->SetStringField(TEXT("message"), Error.Message);
			ErrorObject->SetStringField(TEXT("suggestedFix"), Error.SuggestedFix);
			ErrorsArray.Add(MakeShared<FJsonValueObject>(ErrorObject));
		}
		ItemObject->SetArrayField(TEXT("errors"), ErrorsArray);

		// Add warnings
		TArray<TSharedPtr<FJsonValue>> WarningsArray;
		for (const FString& Warning : Item.Warnings)
		{
			WarningsArray.Add(MakeShared<FJsonValueString>(Warning));
		}
		ItemObject->SetArrayField(TEXT("warnings"), WarningsArray);

		ItemsArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}
	JsonObject->SetArrayField(TEXT("items"), ItemsArray);

	// Serialize to string
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(JsonObject, Writer))
	{
		return false;
	}

	// Ensure directory exists
	FString Directory = FPaths::GetPath(OutputPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	// Write to file
	return FFileHelper::SaveStringToFile(JsonString, *OutputPath);
}

FString UGenerationReport::GetStatusString(EGenerationStatus Status)
{
	switch (Status)
	{
	case EGenerationStatus::New: return TEXT("New");
	case EGenerationStatus::Skipped: return TEXT("Skipped");
	case EGenerationStatus::Failed: return TEXT("Failed");
	case EGenerationStatus::Deferred: return TEXT("Deferred");
	default: return TEXT("Unknown");
	}
}

FString UGenerationReport::GetDryRunStatusString(EDryRunStatus Status)
{
	switch (Status)
	{
	case EDryRunStatus::WillCreate: return TEXT("WillCreate");
	case EDryRunStatus::WillModify: return TEXT("WillModify");
	case EDryRunStatus::WillSkip: return TEXT("WillSkip");
	case EDryRunStatus::Conflicted: return TEXT("Conflicted");
	case EDryRunStatus::PolicySkip: return TEXT("PolicySkip");
	default: return TEXT("Unknown");
	}
}

// ============================================================================
// FGenerationReportHelper Implementation
// ============================================================================

UGenerationReport* FGenerationReportHelper::CreateAndSaveReport(
	const FString& ManifestPath,
	int64 ManifestHash,
	const TArray<FGenerationResult>& Results,
	bool bForce)
{
	// Create report object
	FGuid RunId = FGuid::NewGuid();
	FString ReportName = GenerateReportName(RunId, false);
	FString PackagePath = GetReportAssetPath() / ReportName;

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for report: %s"), *PackagePath);
		return nullptr;
	}

	// Create report object
	UGenerationReport* Report = NewObject<UGenerationReport>(Package, *ReportName, RF_Public | RF_Standalone);
	if (!Report)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create report object"));
		return nullptr;
	}

	// Initialize
	Report->RunId = RunId;
	Report->Initialize(ManifestPath, ManifestHash, false, bForce);

	// Add items
	for (const FGenerationResult& Result : Results)
	{
		Report->AddItem(Result);
	}

	// Finalize counts
	Report->Finalize();

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(Report);

	// Mark dirty and save
	Report->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Report, *PackageFileName, SaveArgs);

	// Save JSON mirror
	FString JsonPath = GetReportJsonPath() / ReportName + TEXT(".json");
	Report->SaveAsJson(JsonPath);

	UE_LOG(LogTemp, Display, TEXT("Generation report saved: %s"), *PackagePath);
	UE_LOG(LogTemp, Display, TEXT("JSON report saved: %s"), *JsonPath);

	return Report;
}

UGenerationReport* FGenerationReportHelper::CreateAndSaveDryRunReport(
	const FString& ManifestPath,
	int64 ManifestHash,
	const FDryRunSummary& Summary,
	bool bForce)
{
	// Create report object
	FGuid RunId = FGuid::NewGuid();
	FString ReportName = GenerateReportName(RunId, true);
	FString PackagePath = GetReportAssetPath() / ReportName;

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for dry-run report: %s"), *PackagePath);
		return nullptr;
	}

	// Create report object
	UGenerationReport* Report = NewObject<UGenerationReport>(Package, *ReportName, RF_Public | RF_Standalone);
	if (!Report)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create dry-run report object"));
		return nullptr;
	}

	// Initialize
	Report->RunId = RunId;
	Report->Initialize(ManifestPath, ManifestHash, true, bForce);

	// Add items
	for (const FDryRunResult& Result : Summary.Results)
	{
		Report->AddDryRunItem(Result);
	}

	// Finalize counts
	Report->Finalize();

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(Report);

	// Mark dirty and save
	Report->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Report, *PackageFileName, SaveArgs);

	// Save JSON mirror
	FString JsonPath = GetReportJsonPath() / ReportName + TEXT(".json");
	Report->SaveAsJson(JsonPath);

	UE_LOG(LogTemp, Display, TEXT("Dry-run report saved: %s"), *PackagePath);
	UE_LOG(LogTemp, Display, TEXT("JSON report saved: %s"), *JsonPath);

	return Report;
}

FString FGenerationReportHelper::GetReportAssetPath()
{
	return TEXT("/Game/Generated/Reports");
}

FString FGenerationReportHelper::GetReportJsonPath()
{
	return FPaths::ProjectSavedDir() / TEXT("GasAbilityGenerator/Reports");
}

FString FGenerationReportHelper::GenerateReportName(const FGuid& RunId, bool bDryRun)
{
	FString Prefix = bDryRun ? TEXT("GR_DryRun_") : TEXT("GR_");
	FString TimestampStr = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	FString ShortId = RunId.ToString().Left(8);
	return Prefix + TimestampStr + TEXT("_") + ShortId;
}
