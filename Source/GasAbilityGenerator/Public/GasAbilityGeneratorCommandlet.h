// GasAbilityGeneratorCommandlet.h
// Commandlet for automated asset generation from command line

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GasAbilityGeneratorCommandlet.generated.h"

/**
 * Commandlet to generate assets from a YAML manifest without opening the editor UI.
 *
 * Usage:
 *   UnrealEditor.exe ProjectName.uproject -run=GasAbilityGenerator -manifest="path/to/manifest.yaml"
 *
 * Parameters:
 *   -manifest=<path>  : Path to the YAML manifest file (required)
 *   -tags             : Generate gameplay tags
 *   -assets           : Generate assets
 *   -all              : Generate both tags and assets (default)
 *   -output=<path>    : Output log file path (optional)
 */
UCLASS()
class GASABILITYGENERATOR_API UGasAbilityGeneratorCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGasAbilityGeneratorCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	void GenerateTags(const struct FManifestData& ManifestData);
	void GenerateAssets(const struct FManifestData& ManifestData);
	void LogMessage(const FString& Message);
	void LogError(const FString& Message);

	FString OutputLogPath;
	TArray<FString> LogMessages;
};
