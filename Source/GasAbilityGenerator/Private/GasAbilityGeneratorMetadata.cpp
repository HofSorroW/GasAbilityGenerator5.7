// GasAbilityGenerator v3.1
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.1: Added UGeneratorMetadataRegistry for assets that don't support AssetUserData

#include "GasAbilityGeneratorMetadata.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/App.h"

// v3.1: Dedicated log category
DEFINE_LOG_CATEGORY_STATIC(LogGeneratorMetadata, Log, All);

// v3.1: Static registry cache
TWeakObjectPtr<UGeneratorMetadataRegistry> UGeneratorMetadataRegistry::CachedRegistry;

UGeneratorAssetMetadata::UGeneratorAssetMetadata()
	: InputHash(0)
	, OutputHash(0)
	, Timestamp(FDateTime::Now())
	, bIsGenerated(false)
{
}

FGeneratorMetadata UGeneratorAssetMetadata::ToMetadata() const
{
	FGeneratorMetadata Meta;
	Meta.GeneratorId = GeneratorId;
	Meta.ManifestPath = ManifestPath;
	Meta.ManifestAssetKey = ManifestAssetKey;
	Meta.InputHash = static_cast<uint64>(InputHash);
	Meta.OutputHash = static_cast<uint64>(OutputHash);
	Meta.GeneratorVersion = GeneratorVersion;
	Meta.Timestamp = Timestamp;
	Meta.Dependencies = Dependencies;
	Meta.bIsGenerated = bIsGenerated;
	return Meta;
}

void UGeneratorAssetMetadata::FromMetadata(const FGeneratorMetadata& InMetadata)
{
	GeneratorId = InMetadata.GeneratorId;
	ManifestPath = InMetadata.ManifestPath;
	ManifestAssetKey = InMetadata.ManifestAssetKey;
	InputHash = static_cast<int64>(InMetadata.InputHash);
	OutputHash = static_cast<int64>(InMetadata.OutputHash);
	GeneratorVersion = InMetadata.GeneratorVersion;
	Timestamp = InMetadata.Timestamp;
	Dependencies = InMetadata.Dependencies;
	bIsGenerated = InMetadata.bIsGenerated;
}

bool UGeneratorAssetMetadata::HasInputChanged(uint64 NewInputHash) const
{
	return static_cast<uint64>(InputHash) != NewInputHash;
}

bool UGeneratorAssetMetadata::HasOutputChanged(uint64 CurrentOutputHash) const
{
	return static_cast<uint64>(OutputHash) != CurrentOutputHash;
}

FString UGeneratorAssetMetadata::GetSummary() const
{
	return FString::Printf(TEXT("[%s] %s - InHash:%llu OutHash:%llu Ver:%s Gen:%s"),
		*GeneratorId,
		*ManifestAssetKey,
		static_cast<uint64>(InputHash),
		static_cast<uint64>(OutputHash),
		*GeneratorVersion,
		bIsGenerated ? TEXT("Yes") : TEXT("No"));
}

// ============================================================================
// v3.1: FGeneratorMetadataRecord Implementation
// ============================================================================

FGeneratorMetadata FGeneratorMetadataRecord::ToMetadata() const
{
	FGeneratorMetadata Meta;
	Meta.GeneratorId = GeneratorId;
	Meta.ManifestPath = ManifestPath;
	Meta.ManifestAssetKey = ManifestAssetKey;
	Meta.InputHash = static_cast<uint64>(InputHash);
	Meta.OutputHash = static_cast<uint64>(OutputHash);
	Meta.GeneratorVersion = GeneratorVersion;
	Meta.Timestamp = Timestamp;
	Meta.Dependencies = Dependencies;
	Meta.bIsGenerated = bIsGenerated;
	return Meta;
}

void FGeneratorMetadataRecord::FromMetadata(const FGeneratorMetadata& InMetadata)
{
	GeneratorId = InMetadata.GeneratorId;
	ManifestPath = InMetadata.ManifestPath;
	ManifestAssetKey = InMetadata.ManifestAssetKey;
	InputHash = static_cast<int64>(InMetadata.InputHash);
	OutputHash = static_cast<int64>(InMetadata.OutputHash);
	GeneratorVersion = InMetadata.GeneratorVersion;
	Timestamp = InMetadata.Timestamp;
	Dependencies = InMetadata.Dependencies;
	bIsGenerated = InMetadata.bIsGenerated;
}

// ============================================================================
// v3.1: UGeneratorMetadataRegistry Implementation
// ============================================================================

UGeneratorMetadataRegistry::UGeneratorMetadataRegistry()
{
}

FGeneratorMetadataRecord* UGeneratorMetadataRegistry::GetRecord(const FString& AssetPath)
{
	return Records.Find(AssetPath);
}

void UGeneratorMetadataRegistry::SetRecord(const FString& AssetPath, const FGeneratorMetadataRecord& Record)
{
	Records.Add(AssetPath, Record);
}

void UGeneratorMetadataRegistry::RemoveRecord(const FString& AssetPath)
{
	Records.Remove(AssetPath);
}

bool UGeneratorMetadataRegistry::HasRecord(const FString& AssetPath) const
{
	return Records.Contains(AssetPath);
}

TArray<FString> UGeneratorMetadataRegistry::GetAllAssetPaths() const
{
	TArray<FString> Paths;
	Records.GetKeys(Paths);
	return Paths;
}

void UGeneratorMetadataRegistry::ClearAllRecords()
{
	Records.Empty();
}

UGeneratorMetadataRegistry* UGeneratorMetadataRegistry::GetOrCreateRegistry()
{
	// Return cached if valid
	if (CachedRegistry.IsValid())
	{
		return CachedRegistry.Get();
	}

	// Determine registry path based on project
	FString ProjectName = FApp::GetProjectName();
	if (ProjectName.IsEmpty())
	{
		ProjectName = TEXT("Default");
	}

	FString RegistryPath = FString::Printf(TEXT("/Game/%s/GeneratorMetadataRegistry"), *ProjectName);

	// Try to load existing
	UGeneratorMetadataRegistry* Registry = LoadObject<UGeneratorMetadataRegistry>(nullptr, *RegistryPath);

	// Create new if not found
	if (!Registry)
	{
		UPackage* Package = CreatePackage(*RegistryPath);
		if (Package)
		{
			Registry = NewObject<UGeneratorMetadataRegistry>(Package, TEXT("GeneratorMetadataRegistry"), RF_Public | RF_Standalone);
			if (Registry)
			{
				Package->MarkPackageDirty();
				FAssetRegistryModule::AssetCreated(Registry);

				// Save immediately
				FString PackageFileName = FPackageName::LongPackageNameToFilename(RegistryPath, FPackageName::GetAssetPackageExtension());
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
				UPackage::SavePackage(Package, Registry, *PackageFileName, SaveArgs);

				UE_LOG(LogGeneratorMetadata, Display, TEXT("Created new metadata registry at %s"), *RegistryPath);
			}
		}
	}

	CachedRegistry = Registry;
	return Registry;
}

UGeneratorMetadataRegistry* UGeneratorMetadataRegistry::GetRegistry()
{
	if (CachedRegistry.IsValid())
	{
		return CachedRegistry.Get();
	}

	FString ProjectName = FApp::GetProjectName();
	if (ProjectName.IsEmpty())
	{
		ProjectName = TEXT("Default");
	}

	FString RegistryPath = FString::Printf(TEXT("/Game/%s/GeneratorMetadataRegistry"), *ProjectName);
	UGeneratorMetadataRegistry* Registry = LoadObject<UGeneratorMetadataRegistry>(nullptr, *RegistryPath);
	CachedRegistry = Registry;
	return Registry;
}

void UGeneratorMetadataRegistry::SaveRegistry()
{
	UGeneratorMetadataRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return;
	}

	UPackage* Package = Registry->GetOutermost();
	if (!Package)
	{
		return;
	}

	Package->MarkPackageDirty();

	FString PackagePath = Package->GetPathName();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Registry, *PackageFileName, SaveArgs);

	UE_LOG(LogGeneratorMetadata, Display, TEXT("Saved metadata registry with %d records"), Registry->Records.Num());
}

void UGeneratorMetadataRegistry::ClearRegistryCache()
{
	CachedRegistry.Reset();
}

// ============================================================================
// Helper Functions
// v3.1: Updated to use registry fallback for unsupported asset types
// ============================================================================

namespace GeneratorMetadataHelpers
{
	// v3.1: Helper to get asset path for registry lookup
	static FString GetAssetPathForRegistry(UObject* Asset)
	{
		if (!Asset)
		{
			return FString();
		}

		// For assets, get the package path
		UPackage* Package = Asset->GetOutermost();
		if (Package)
		{
			return Package->GetPathName();
		}

		return Asset->GetPathName();
	}

	UGeneratorAssetMetadata* GetMetadata(UObject* Asset)
	{
		if (!Asset)
		{
			return nullptr;
		}

		// v3.1: First try IInterface_AssetUserData (works for UAnimationAsset, etc.)
		if (IInterface_AssetUserData* AssetUserDataInterface = Cast<IInterface_AssetUserData>(Asset))
		{
			UGeneratorAssetMetadata* Meta = Cast<UGeneratorAssetMetadata>(
				AssetUserDataInterface->GetAssetUserDataOfClass(UGeneratorAssetMetadata::StaticClass()));
			if (Meta)
			{
				return Meta;
			}
		}

		// v3.1: Fallback - AssetUserData not supported, return nullptr
		// The caller should use GetMetadataFromRegistry instead
		return nullptr;
	}

	// v3.1: New function to get metadata, checking both AssetUserData and registry
	FGeneratorMetadata GetMetadataEx(UObject* Asset)
	{
		if (!Asset)
		{
			return FGeneratorMetadata();
		}

		// First try AssetUserData
		UGeneratorAssetMetadata* Meta = GetMetadata(Asset);
		if (Meta)
		{
			return Meta->ToMetadata();
		}

		// v3.1: Fallback to registry
		FString AssetPath = GetAssetPathForRegistry(Asset);
		if (!AssetPath.IsEmpty())
		{
			UGeneratorMetadataRegistry* Registry = UGeneratorMetadataRegistry::GetRegistry();
			if (Registry)
			{
				FGeneratorMetadataRecord* Record = Registry->GetRecord(AssetPath);
				if (Record)
				{
					return Record->ToMetadata();
				}
			}
		}

		return FGeneratorMetadata();
	}

	// v3.1: Check if we have metadata (from either source)
	bool HasMetadataEx(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		// Check AssetUserData first
		if (GetMetadata(Asset))
		{
			return true;
		}

		// Check registry
		FString AssetPath = GetAssetPathForRegistry(Asset);
		if (!AssetPath.IsEmpty())
		{
			UGeneratorMetadataRegistry* Registry = UGeneratorMetadataRegistry::GetRegistry();
			if (Registry && Registry->HasRecord(AssetPath))
			{
				return true;
			}
		}

		return false;
	}

	void SetMetadata(UObject* Asset, const FGeneratorMetadata& Metadata)
	{
		if (!Asset)
		{
			return;
		}

		// v3.1: Try IInterface_AssetUserData first
		IInterface_AssetUserData* AssetUserDataInterface = Cast<IInterface_AssetUserData>(Asset);
		if (AssetUserDataInterface)
		{
			UGeneratorAssetMetadata* MetaData = GetMetadata(Asset);

			// Create new if doesn't exist
			if (!MetaData)
			{
				MetaData = NewObject<UGeneratorAssetMetadata>(Asset);
				AssetUserDataInterface->AddAssetUserData(MetaData);
			}

			// Update metadata
			MetaData->FromMetadata(Metadata);
			Asset->MarkPackageDirty();
			return;
		}

		// v3.1: Fallback to registry for assets that don't support AssetUserData
		FString AssetPath = GetAssetPathForRegistry(Asset);
		if (!AssetPath.IsEmpty())
		{
			UGeneratorMetadataRegistry* Registry = UGeneratorMetadataRegistry::GetOrCreateRegistry();
			if (Registry)
			{
				FGeneratorMetadataRecord Record;
				Record.FromMetadata(Metadata);
				Registry->SetRecord(AssetPath, Record);
				// Note: Registry save is deferred to end of generation for efficiency
				UE_LOG(LogGeneratorMetadata, Verbose, TEXT("Stored metadata in registry for: %s"), *AssetPath);
			}
		}
	}

	void RemoveMetadata(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		// v3.1: Try IInterface_AssetUserData first
		if (IInterface_AssetUserData* AssetUserDataInterface = Cast<IInterface_AssetUserData>(Asset))
		{
			UGeneratorAssetMetadata* MetaData = GetMetadata(Asset);
			if (MetaData)
			{
				AssetUserDataInterface->RemoveUserDataOfClass(UGeneratorAssetMetadata::StaticClass());
				Asset->MarkPackageDirty();
				return;
			}
		}

		// v3.1: Also remove from registry
		FString AssetPath = GetAssetPathForRegistry(Asset);
		if (!AssetPath.IsEmpty())
		{
			UGeneratorMetadataRegistry* Registry = UGeneratorMetadataRegistry::GetRegistry();
			if (Registry && Registry->HasRecord(AssetPath))
			{
				Registry->RemoveRecord(AssetPath);
			}
		}
	}

	bool HasMetadata(UObject* Asset)
	{
		// v3.1: Use HasMetadataEx which checks both sources
		return HasMetadataEx(Asset);
	}

	EDryRunStatus ComputeDryRunStatus(
		UObject* Asset,
		uint64 NewInputHash,
		TFunction<uint64(UObject*)> ComputeCurrentOutputHash)
	{
		// No existing asset - will create new
		if (!Asset)
		{
			return EDryRunStatus::WillCreate;
		}

		// v3.1: Get existing metadata from either AssetUserData or registry
		FGeneratorMetadata Metadata = GetMetadataEx(Asset);

		// No metadata - asset exists but wasn't generated by us
		// Treat as skip to avoid overwriting manual assets
		if (!Metadata.bIsGenerated)
		{
			return EDryRunStatus::WillSkip;
		}

		// Check if manifest definition changed
		bool bInputChanged = (Metadata.InputHash != NewInputHash);

		// Check if asset was manually edited (if we have output hash computation)
		bool bOutputChanged = false;
		if (ComputeCurrentOutputHash)
		{
			uint64 CurrentOutputHash = ComputeCurrentOutputHash(Asset);
			bOutputChanged = (Metadata.OutputHash != CurrentOutputHash);
		}

		// Determine status based on changes
		if (!bInputChanged && !bOutputChanged)
		{
			// Nothing changed - skip
			return EDryRunStatus::WillSkip;
		}
		else if (bInputChanged && !bOutputChanged)
		{
			// Only manifest changed - safe to regenerate
			return EDryRunStatus::WillModify;
		}
		else if (bInputChanged && bOutputChanged)
		{
			// Both changed - conflict requiring user decision
			return EDryRunStatus::Conflicted;
		}
		else // !bInputChanged && bOutputChanged
		{
			// Only asset edited, manifest unchanged - skip to preserve edits
			return EDryRunStatus::WillSkip;
		}
	}

	// v3.1: Save the registry after generation completes
	void SaveRegistryIfNeeded()
	{
		UGeneratorMetadataRegistry::SaveRegistry();
	}
}
