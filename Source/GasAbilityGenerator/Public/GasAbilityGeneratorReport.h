// GasAbilityGeneratorReport.h
// v4.7: Machine-readable generation report system for CI/CD and debugging
// Reports are saved as UDataAsset + JSON mirror for maximum accessibility

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorReport.generated.h"

/**
 * v4.7: Structured error for machine-readable debugging
 * Every failure must include at least one structured error
 */
USTRUCT(BlueprintType)
struct FGenerationError
{
	GENERATED_BODY()

	// Error code for programmatic handling (e.g., "E_PARENT_CLASS_NOT_FOUND")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ErrorCode;

	// Context path for locating the error (e.g., "Abilities/GA_FatherAttack/parent_class")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ContextPath;

	// Human-readable error message
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Message;

	// Actionable fix suggestion
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString SuggestedFix;

	FGenerationError() = default;

	FGenerationError(const FString& InCode, const FString& InContext, const FString& InMessage, const FString& InFix)
		: ErrorCode(InCode)
		, ContextPath(InContext)
		, Message(InMessage)
		, SuggestedFix(InFix)
	{}
};

/**
 * v4.7: Single item in generation report
 * Represents the outcome of one asset generation attempt
 */
USTRUCT(BlueprintType)
struct FGenerationReportItem
{
	GENERATED_BODY()

	// Full asset path (e.g., "/Game/FatherCompanion/Abilities/GA_FatherAttack")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString AssetPath;

	// Asset name for readability
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString AssetName;

	// Generator that processed this asset (e.g., "GameplayAbility")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString GeneratorId;

	// Execution status (New, Skipped, Failed, Deferred)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ExecutedStatus;

	// For dry-run: planned status (WillCreate, WillModify, WillSkip, Conflicted)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlannedStatus;

	// Whether planned status is set (for dry-run reports)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasPlannedStatus = false;

	// Whether asset existed before this run (distinguishes Create vs Modify)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAssetExistedBeforeRun = false;

	// Reason/message for this outcome
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Reason;

	// Structured errors (for Failed items)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FGenerationError> Errors;

	// Warnings (non-fatal issues)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FString> Warnings;

	FGenerationReportItem() = default;

	// Create from FGenerationResult
	static FGenerationReportItem FromGenerationResult(const FGenerationResult& Result);

	// Create from FDryRunResult
	static FGenerationReportItem FromDryRunResult(const struct FDryRunResult& Result);
};

/**
 * v4.7: Generation report - machine-readable audit trail
 * Saved as UDataAsset for in-editor inspection and JSON for CI/CD
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UGenerationReport : public UDataAsset
{
	GENERATED_BODY()

public:
	// Unique identifier for this run
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid RunId;

	// When this report was generated
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime Timestamp;

	// Generator plugin version
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString GeneratorVersion;

	// Path to manifest file used
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ManifestFilePath;

	// Hash of manifest content for change detection
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 ManifestHash = 0;

	// Whether this was a dry-run
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsDryRun = false;

	// Whether --force was used
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsForceRun = false;

	// Individual asset results
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FGenerationReportItem> Items;

	// Summary counts
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountNew = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountSkipped = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountFailed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountDeferred = 0;

	// Dry-run specific counts
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountWillCreate = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountWillModify = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountWillSkip = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CountConflicted = 0;

public:
	/** Initialize report with run metadata */
	void Initialize(const FString& InManifestPath, int64 InManifestHash, bool bInDryRun, bool bInForce);

	/** Add item from real-run result */
	void AddItem(const FGenerationResult& Result);

	/** Add item from dry-run result */
	void AddDryRunItem(const struct FDryRunResult& Result);

	/** Finalize counts after all items added */
	void Finalize();

	/** Save report as JSON to Saved/ directory */
	bool SaveAsJson(const FString& OutputPath) const;

	/** Get status string for serialization */
	static FString GetStatusString(EGenerationStatus Status);

	/** Get dry-run status string for serialization */
	static FString GetDryRunStatusString(EDryRunStatus Status);
};

/**
 * v4.7: Helper class for report creation and saving
 */
class GASABILITYGENERATOR_API FGenerationReportHelper
{
public:
	/** Create and save a report from real-run results */
	static UGenerationReport* CreateAndSaveReport(
		const FString& ManifestPath,
		int64 ManifestHash,
		const TArray<FGenerationResult>& Results,
		bool bForce);

	/** Create and save a report from dry-run results */
	static UGenerationReport* CreateAndSaveDryRunReport(
		const FString& ManifestPath,
		int64 ManifestHash,
		const struct FDryRunSummary& Summary,
		bool bForce);

	/** Get the report output directory for UDataAsset */
	static FString GetReportAssetPath();

	/** Get the report output directory for JSON */
	static FString GetReportJsonPath();

	/** Generate unique report name based on timestamp and run ID */
	static FString GenerateReportName(const FGuid& RunId, bool bDryRun);
};
