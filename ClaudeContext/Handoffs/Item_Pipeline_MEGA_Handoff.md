# Item Pipeline MEGA Handoff Document

**Version:** 1.0
**Date:** January 2026
**Purpose:** Complete specification for implementing Mesh-to-Item Pipeline in GasAbilityGenerator

---

## Overview

This document specifies a complete pipeline that transforms raw meshes into fully playable items:

```
Mesh → Blueprint Item (BI_) → Item Collection (IC_) → NPC Loadout
```

### Three Input Methods
1. **Batch YAML** - File-based bulk processing
2. **Editor UI** - Slate window in Unreal Editor
3. **Claude Code** - Conversational interface

---

## Part 1: Phase-by-Phase Implementation Specs

### Phase 1: Core Infrastructure

#### New Enums

```cpp
// In GasAbilityGeneratorTypes.h

UENUM()
enum class EPipelineItemType : uint8
{
    Clothing,           // UEquippableItem_Clothing
    Weapon_Melee,       // UMeleeWeaponItem
    Weapon_Ranged,      // URangedWeaponItem
    Consumable,         // UConsumableItem
    Generic             // UNarrativeItem
};

UENUM()
enum class EPipelineSlotInference : uint8
{
    Automatic,          // Infer from mesh name/path
    Manual,             // Explicitly specified
    Interactive         // Ask user via Claude Code
};
```

#### Core Structs

```cpp
// Pipeline input definition
USTRUCT()
struct FPipelineMeshInput
{
    GENERATED_BODY()

    // Source mesh path (SK_* or SM_*)
    UPROPERTY()
    FString MeshPath;

    // Optional: Override inferred item type
    UPROPERTY()
    EPipelineItemType ItemType = EPipelineItemType::Clothing;

    // Optional: Override inferred slot
    UPROPERTY()
    FGameplayTag EquipmentSlot;

    // Optional: Display name override
    UPROPERTY()
    FString DisplayName;

    // Optional: Target collection to add to
    UPROPERTY()
    FString TargetCollection;

    // Optional: Target NPC loadout
    UPROPERTY()
    FString TargetLoadout;
};

// Pipeline processing result
USTRUCT()
struct FPipelineProcessResult
{
    GENERATED_BODY()

    bool bSuccess = false;
    FString GeneratedItemPath;      // /Game/.../BI_*
    FString GeneratedCollectionPath; // /Game/.../IC_*
    FString ModifiedLoadoutPath;    // DataTable path
    TArray<FString> Warnings;
    TArray<FString> Errors;
};

// Mesh analysis result
USTRUCT()
struct FMeshAnalysisResult
{
    GENERATED_BODY()

    EPipelineItemType InferredType;
    FGameplayTag InferredSlot;
    FString SuggestedDisplayName;
    FString SuggestedAssetName;

    // Related assets found
    TArray<FString> RelatedMaterials;
    TArray<FString> RelatedTextures;
    TArray<FString> RelatedAnimations;

    // Confidence scores (0.0 - 1.0)
    float SlotConfidence = 0.0f;
    float TypeConfidence = 0.0f;
};
```

#### FPipelineMeshAnalyzer Class

```cpp
// In GasAbilityGeneratorPipeline.h

class GASABILITYGENERATOR_API FPipelineMeshAnalyzer
{
public:
    // Analyze a single mesh
    static FMeshAnalysisResult AnalyzeMesh(const FString& MeshPath);

    // Batch analyze multiple meshes
    static TArray<FMeshAnalysisResult> AnalyzeMeshes(const TArray<FString>& MeshPaths);

private:
    // Slot inference from naming conventions
    static FGameplayTag InferSlotFromName(const FString& MeshName);

    // Type inference from mesh properties
    static EPipelineItemType InferTypeFromMesh(UObject* MeshAsset);

    // Find related assets in same directory
    static TArray<FString> FindRelatedAssets(const FString& MeshPath);

    // Generate display name from asset name
    static FString GenerateDisplayName(const FString& AssetName);
};
```

#### FPipelineProcessor Class

```cpp
// Main pipeline processor

class GASABILITYGENERATOR_API FPipelineProcessor
{
public:
    // Process single mesh through full pipeline
    static FPipelineProcessResult ProcessMesh(const FPipelineMeshInput& Input);

    // Process multiple meshes (batch mode)
    static TArray<FPipelineProcessResult> ProcessMeshes(const TArray<FPipelineMeshInput>& Inputs);

    // Stage 1: Generate BI_ item from mesh
    static FString GenerateItemFromMesh(const FPipelineMeshInput& Input, const FMeshAnalysisResult& Analysis);

    // Stage 2: Add item to or create IC_ collection
    static FString AddToCollection(const FString& ItemPath, const FString& CollectionName);

    // Stage 3: Add collection to NPC loadout
    static bool AddToLoadout(const FString& CollectionPath, const FString& LoadoutPath);

private:
    // Create manifest definition from analysis
    static FManifestEquippableItemDefinition CreateItemDefinition(
        const FPipelineMeshInput& Input,
        const FMeshAnalysisResult& Analysis);
};
```

---

### Phase 2: Enhanced EI_ Generator for Clothing

#### New Manifest Structs

```cpp
// Material color binding for character creator integration
USTRUCT()
struct FManifestMaterialColorBinding
{
    GENERATED_BODY()

    // Character creator color tag (e.g., "Narrative.CharacterCreator.Vectors.Torso.PrimaryColor")
    UPROPERTY()
    FString ColorTag;

    // Material parameter name to bind (e.g., "BaseColor")
    UPROPERTY()
    FString ParameterName;
};

// Material definition for clothing mesh
USTRUCT()
struct FManifestClothingMaterial
{
    GENERATED_BODY()

    // Material asset path
    UPROPERTY()
    FString Material;

    // Vector parameter bindings
    UPROPERTY()
    TArray<FManifestMaterialColorBinding> VectorParams;

    // Scalar parameter bindings
    UPROPERTY()
    TArray<FManifestMaterialColorBinding> ScalarParams;
};

// Complete clothing mesh configuration
USTRUCT()
struct FManifestClothingMeshConfig
{
    GENERATED_BODY()

    // Skeletal mesh path (SK_*)
    UPROPERTY()
    FString Mesh;

    // Use leader pose (follow character skeleton)
    UPROPERTY()
    bool bUseLeaderPose = true;

    // Use static mesh instead
    UPROPERTY()
    bool bIsStaticMesh = false;

    // Static mesh path (SM_*) if bIsStaticMesh
    UPROPERTY()
    FString StaticMesh;

    // Attach socket name
    UPROPERTY()
    FString AttachSocket;

    // Attach offset transform
    UPROPERTY()
    FVector AttachLocation = FVector::ZeroVector;

    UPROPERTY()
    FRotator AttachRotation = FRotator::ZeroRotator;

    UPROPERTY()
    FVector AttachScale = FVector::OneVector;

    // AnimBP for non-leader-pose meshes
    UPROPERTY()
    FString MeshAnimBP;

    // Materials with color bindings
    UPROPERTY()
    TArray<FManifestClothingMaterial> Materials;
};
```

#### Enhanced FManifestEquippableItemDefinition

Add to existing struct in GasAbilityGeneratorTypes.h:

```cpp
// Add these members to FManifestEquippableItemDefinition

// Clothing mesh configuration (for EquippableItem_Clothing parent)
UPROPERTY()
FManifestClothingMeshConfig ClothingMesh;

// Equipment effect values (TMap<FGameplayTag, float>)
UPROPERTY()
TMap<FString, float> EquipmentEffectValues;
```

#### YAML Parsing Updates

Add to FGasAbilityGeneratorParser::ParseEquippableItems():

```cpp
// Parse clothing mesh config
if (ItemNode["clothing_mesh"])
{
    auto& CM = Def.ClothingMesh;
    auto CMNode = ItemNode["clothing_mesh"];

    if (CMNode["mesh"]) CM.Mesh = CMNode["mesh"].as<std::string>();
    if (CMNode["use_leader_pose"]) CM.bUseLeaderPose = CMNode["use_leader_pose"].as<bool>();
    if (CMNode["is_static_mesh"]) CM.bIsStaticMesh = CMNode["is_static_mesh"].as<bool>();
    if (CMNode["static_mesh"]) CM.StaticMesh = CMNode["static_mesh"].as<std::string>();
    if (CMNode["attach_socket"]) CM.AttachSocket = CMNode["attach_socket"].as<std::string>();

    // Parse attach offset
    if (CMNode["attach_location"])
    {
        auto Loc = CMNode["attach_location"];
        CM.AttachLocation.X = Loc["x"] ? Loc["x"].as<float>() : 0.f;
        CM.AttachLocation.Y = Loc["y"] ? Loc["y"].as<float>() : 0.f;
        CM.AttachLocation.Z = Loc["z"] ? Loc["z"].as<float>() : 0.f;
    }

    // Parse materials array
    if (CMNode["materials"])
    {
        for (auto& MatNode : CMNode["materials"])
        {
            FManifestClothingMaterial Mat;
            if (MatNode["material"]) Mat.Material = MatNode["material"].as<std::string>();

            // Parse vector params
            if (MatNode["vector_params"])
            {
                for (auto& VP : MatNode["vector_params"])
                {
                    FManifestMaterialColorBinding Binding;
                    Binding.ColorTag = VP["color_tag"].as<std::string>();
                    Binding.ParameterName = VP["parameter_name"].as<std::string>();
                    Mat.VectorParams.Add(Binding);
                }
            }
            CM.Materials.Add(Mat);
        }
    }
}

// Parse equipment effect values
if (ItemNode["equipment_effect_values"])
{
    for (auto& EV : ItemNode["equipment_effect_values"])
    {
        FString Tag = UTF8_TO_TCHAR(EV.first.as<std::string>().c_str());
        float Value = EV.second.as<float>();
        Def.EquipmentEffectValues.Add(Tag, Value);
    }
}
```

#### Generator Updates

In FEquippableItemGenerator::Generate(), add after existing property population:

```cpp
// Handle EquippableItem_Clothing parent class
if (Definition.ParentClass.Contains(TEXT("Clothing")))
{
    // Get ClothingMeshData property
    FStructProperty* ClothingMeshProp = CastField<FStructProperty>(
        ItemCDO->GetClass()->FindPropertyByName(TEXT("ClothingMeshData")));

    if (ClothingMeshProp)
    {
        void* ClothingMeshPtr = ClothingMeshProp->ContainerPtrToValuePtr<void>(ItemCDO);
        UScriptStruct* MeshStruct = ClothingMeshProp->Struct;

        // Set Mesh property
        if (!Definition.ClothingMesh.Mesh.IsEmpty())
        {
            FSoftObjectProperty* MeshProp = CastField<FSoftObjectProperty>(
                MeshStruct->FindPropertyByName(TEXT("Mesh")));
            if (MeshProp)
            {
                FSoftObjectPath MeshPath(Definition.ClothingMesh.Mesh);
                MeshProp->SetPropertyValue_InContainer(ClothingMeshPtr, MeshPath);
            }
        }

        // Set bUseLeaderPose
        FBoolProperty* LeaderPoseProp = CastField<FBoolProperty>(
            MeshStruct->FindPropertyByName(TEXT("bUseLeaderPose")));
        if (LeaderPoseProp)
        {
            LeaderPoseProp->SetPropertyValue_InContainer(
                ClothingMeshPtr, Definition.ClothingMesh.bUseLeaderPose);
        }

        // Set MeshAttachSocket
        if (!Definition.ClothingMesh.AttachSocket.IsEmpty())
        {
            FNameProperty* SocketProp = CastField<FNameProperty>(
                MeshStruct->FindPropertyByName(TEXT("MeshAttachSocket")));
            if (SocketProp)
            {
                SocketProp->SetPropertyValue_InContainer(
                    ClothingMeshPtr, FName(*Definition.ClothingMesh.AttachSocket));
            }
        }

        // Set MeshAttachOffset transform
        FStructProperty* OffsetProp = CastField<FStructProperty>(
            MeshStruct->FindPropertyByName(TEXT("MeshAttachOffset")));
        if (OffsetProp)
        {
            FTransform* OffsetPtr = OffsetProp->ContainerPtrToValuePtr<FTransform>(ClothingMeshPtr);
            OffsetPtr->SetLocation(Definition.ClothingMesh.AttachLocation);
            OffsetPtr->SetRotation(FQuat(Definition.ClothingMesh.AttachRotation));
            OffsetPtr->SetScale3D(Definition.ClothingMesh.AttachScale);
        }

        // Populate MeshMaterials array
        if (Definition.ClothingMesh.Materials.Num() > 0)
        {
            FArrayProperty* MaterialsArrayProp = CastField<FArrayProperty>(
                MeshStruct->FindPropertyByName(TEXT("MeshMaterials")));

            if (MaterialsArrayProp)
            {
                FScriptArrayHelper ArrayHelper(MaterialsArrayProp,
                    MaterialsArrayProp->ContainerPtrToValuePtr<void>(ClothingMeshPtr));

                for (const auto& MatDef : Definition.ClothingMesh.Materials)
                {
                    int32 NewIndex = ArrayHelper.AddValue();
                    void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);

                    // Set Material soft reference
                    FSoftObjectProperty* MatProp = CastField<FSoftObjectProperty>(
                        MaterialsArrayProp->Inner->Struct->FindPropertyByName(TEXT("Material")));
                    if (MatProp && !MatDef.Material.IsEmpty())
                    {
                        FSoftObjectPath MatPath(MatDef.Material);
                        MatProp->SetPropertyValue_InContainer(ElementPtr, MatPath);
                    }

                    // Populate VectorParams array
                    // ... (similar FScriptArrayHelper pattern)
                }
            }
        }
    }
}

// Handle EquipmentEffectValues TMap<FGameplayTag, float>
if (Definition.EquipmentEffectValues.Num() > 0)
{
    FMapProperty* EffectValuesMapProp = CastField<FMapProperty>(
        ItemCDO->GetClass()->FindPropertyByName(TEXT("EquipmentEffectValues")));

    if (EffectValuesMapProp)
    {
        void* MapPtr = EffectValuesMapProp->ContainerPtrToValuePtr<void>(ItemCDO);
        FScriptMapHelper MapHelper(EffectValuesMapProp, MapPtr);

        for (const auto& Pair : Definition.EquipmentEffectValues)
        {
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Pair.Key));
            float Value = Pair.Value;

            int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
            uint8* KeyPtr = MapHelper.GetKeyPtr(NewIndex);
            uint8* ValuePtr = MapHelper.GetValuePtr(NewIndex);

            // Set key (FGameplayTag)
            FStructProperty* KeyProp = CastField<FStructProperty>(EffectValuesMapProp->KeyProp);
            KeyProp->CopySingleValue(KeyPtr, &Tag);

            // Set value (float)
            FFloatProperty* ValueProp = CastField<FFloatProperty>(EffectValuesMapProp->ValueProp);
            ValueProp->SetFloatingPointPropertyValue(ValuePtr, Value);
        }

        MapHelper.Rehash();
    }
}
```

---

### Phase 3: YAML Configuration Format

#### Full Pipeline YAML Example

```yaml
# item_pipeline.yaml - Batch item generation

pipeline_config:
  default_folder: Items/Generated
  auto_create_collections: true
  auto_add_to_loadouts: true

# Full mesh-to-item definitions
items:
  - mesh: /Game/Meshes/Armor/SK_IronChestplate
    type: Clothing
    slot: Narrative.Equipment.Slot.Mesh.Torso
    display_name: "Iron Chestplate"
    description: "A sturdy iron chestplate."
    folder: Items/Armor

    clothing_mesh:
      mesh: /Game/Meshes/Armor/SK_IronChestplate
      use_leader_pose: true
      materials:
        - material: /Game/Materials/M_Armor_Base
          vector_params:
            - color_tag: Narrative.CharacterCreator.Vectors.Torso.PrimaryColor
              parameter_name: BaseColor
            - color_tag: Narrative.CharacterCreator.Vectors.Torso.SecondaryColor
              parameter_name: AccentColor

    equipment_effect_values:
      SetByCaller.Armor: 15.0
      SetByCaller.Weight: 8.0

    stats:
      armor_rating: 15.0
      stealth_rating: -5.0

    item_tags:
      - Item.Armor.Medium
      - Item.Material.Iron

    target_collection: IC_IronArmorSet
    target_loadout: DT_GuardLoadout

  - mesh: /Game/Meshes/Armor/SK_IronHelmet
    type: Clothing
    slot: Narrative.Equipment.Slot.Mesh.Head
    display_name: "Iron Helmet"
    folder: Items/Armor
    target_collection: IC_IronArmorSet

# Collection definitions
collections:
  - name: IC_IronArmorSet
    folder: Items/Collections
    items:
      - item: BI_C_IronChestplate
        quantity: 1
      - item: BI_C_IronHelmet
        quantity: 1

# Loadout assignments
loadouts:
  - npc_definition: NPCDef_TownGuard
    collections:
      - IC_IronArmorSet
      - IC_GuardWeapons
```

---

## Part 2: Mesh Analysis Heuristics

### Slot Inference from Naming

```cpp
// Naming convention patterns for slot inference

static TMap<FString, FGameplayTag> SlotPatterns = {
    // Head slots
    {TEXT("helmet"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("helm"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("hat"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("hood"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("crown"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("mask"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},
    {TEXT("circlet"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"))},

    // Torso slots
    {TEXT("chest"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("chestplate"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("cuirass"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("shirt"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("robe"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("tunic"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("vest"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},
    {TEXT("jacket"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"))},

    // Leg slots
    {TEXT("leg"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},
    {TEXT("greave"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},
    {TEXT("pants"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},
    {TEXT("trousers"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},
    {TEXT("legging"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},
    {TEXT("platelegs"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"))},

    // Feet slots
    {TEXT("boot"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Feet"))},
    {TEXT("shoe"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Feet"))},
    {TEXT("sabatons"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Feet"))},
    {TEXT("sandal"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Feet"))},

    // Hand slots
    {TEXT("glove"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Hands"))},
    {TEXT("gauntlet"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Hands"))},
    {TEXT("bracer"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Hands"))},
    {TEXT("vambrace"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Hands"))},

    // Back slots
    {TEXT("cape"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Back"))},
    {TEXT("cloak"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Back"))},
    {TEXT("backpack"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Back"))},
    {TEXT("quiver"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Back"))},

    // Weapon slots
    {TEXT("sword"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("axe"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("mace"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("bow"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("staff"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("dagger"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("rifle"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},
    {TEXT("pistol"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"))},

    // Shield
    {TEXT("shield"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Offhand"))},
    {TEXT("buckler"), FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Offhand"))},
};
```

### Slot Inference Implementation

```cpp
FGameplayTag FPipelineMeshAnalyzer::InferSlotFromName(const FString& MeshName)
{
    FString LowerName = MeshName.ToLower();

    // Remove common prefixes
    LowerName.RemoveFromStart(TEXT("sk_"));
    LowerName.RemoveFromStart(TEXT("sm_"));

    // Check each pattern
    float BestConfidence = 0.0f;
    FGameplayTag BestSlot;

    for (const auto& Pattern : SlotPatterns)
    {
        if (LowerName.Contains(Pattern.Key))
        {
            // Weight by how much of the name the pattern matches
            float Confidence = (float)Pattern.Key.Len() / (float)LowerName.Len();

            // Bonus for exact word boundary matches
            int32 FoundIndex = LowerName.Find(Pattern.Key);
            if (FoundIndex == 0 || LowerName[FoundIndex - 1] == '_')
            {
                Confidence += 0.2f;
            }

            if (Confidence > BestConfidence)
            {
                BestConfidence = Confidence;
                BestSlot = Pattern.Value;
            }
        }
    }

    return BestSlot;
}
```

### Weapon Detection Heuristics

```cpp
EPipelineItemType FPipelineMeshAnalyzer::InferTypeFromMesh(UObject* MeshAsset)
{
    FString AssetName = MeshAsset->GetName().ToLower();

    // Weapon keywords
    static TArray<FString> MeleeWeaponKeywords = {
        TEXT("sword"), TEXT("axe"), TEXT("mace"), TEXT("hammer"),
        TEXT("dagger"), TEXT("knife"), TEXT("spear"), TEXT("halberd"),
        TEXT("club"), TEXT("staff"), TEXT("blade")
    };

    static TArray<FString> RangedWeaponKeywords = {
        TEXT("bow"), TEXT("crossbow"), TEXT("rifle"), TEXT("pistol"),
        TEXT("gun"), TEXT("blaster"), TEXT("launcher")
    };

    // Check melee weapons
    for (const FString& Keyword : MeleeWeaponKeywords)
    {
        if (AssetName.Contains(Keyword))
        {
            return EPipelineItemType::Weapon_Melee;
        }
    }

    // Check ranged weapons
    for (const FString& Keyword : RangedWeaponKeywords)
    {
        if (AssetName.Contains(Keyword))
        {
            return EPipelineItemType::Weapon_Ranged;
        }
    }

    // Check for skeletal mesh (likely clothing) vs static mesh (likely prop/weapon)
    if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(MeshAsset))
    {
        // Check bone count - high bone count likely character clothing
        if (SkelMesh->GetRefSkeleton().GetNum() > 10)
        {
            return EPipelineItemType::Clothing;
        }
    }

    // Default to clothing for skeletal, generic for static
    if (Cast<USkeletalMesh>(MeshAsset))
    {
        return EPipelineItemType::Clothing;
    }

    return EPipelineItemType::Generic;
}
```

### Related Asset Discovery

```cpp
TArray<FString> FPipelineMeshAnalyzer::FindRelatedAssets(const FString& MeshPath)
{
    TArray<FString> RelatedAssets;

    // Get directory and base name
    FString Directory = FPaths::GetPath(MeshPath);
    FString BaseName = FPaths::GetBaseFilename(MeshPath);

    // Remove SK_/SM_ prefix
    BaseName.RemoveFromStart(TEXT("SK_"));
    BaseName.RemoveFromStart(TEXT("SM_"));

    // Search for related materials
    TArray<FString> MaterialPaths;
    IFileManager::Get().FindFilesRecursive(
        MaterialPaths,
        *Directory,
        TEXT("M_*.uasset"),
        true, false, false);

    for (const FString& MatPath : MaterialPaths)
    {
        FString MatName = FPaths::GetBaseFilename(MatPath);
        if (MatName.Contains(BaseName))
        {
            RelatedAssets.Add(MatPath);
        }
    }

    // Search for related textures
    TArray<FString> TexturePaths;
    IFileManager::Get().FindFilesRecursive(
        TexturePaths,
        *Directory,
        TEXT("T_*.uasset"),
        true, false, false);

    for (const FString& TexPath : TexturePaths)
    {
        FString TexName = FPaths::GetBaseFilename(TexPath);
        if (TexName.Contains(BaseName))
        {
            RelatedAssets.Add(TexPath);
        }
    }

    // Search for animations (if weapon)
    TArray<FString> AnimPaths;
    IFileManager::Get().FindFilesRecursive(
        AnimPaths,
        *Directory,
        TEXT("A_*.uasset"),
        true, false, false);

    for (const FString& AnimPath : AnimPaths)
    {
        FString AnimName = FPaths::GetBaseFilename(AnimPath);
        if (AnimName.Contains(BaseName))
        {
            RelatedAssets.Add(AnimPath);
        }
    }

    return RelatedAssets;
}
```

### Display Name Generation

```cpp
FString FPipelineMeshAnalyzer::GenerateDisplayName(const FString& AssetName)
{
    FString DisplayName = AssetName;

    // Remove prefixes
    DisplayName.RemoveFromStart(TEXT("SK_"));
    DisplayName.RemoveFromStart(TEXT("SM_"));
    DisplayName.RemoveFromStart(TEXT("BI_"));
    DisplayName.RemoveFromStart(TEXT("BI_C_"));

    // Replace underscores with spaces
    DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

    // Title case each word
    TArray<FString> Words;
    DisplayName.ParseIntoArray(Words, TEXT(" "), true);

    DisplayName.Empty();
    for (int32 i = 0; i < Words.Num(); i++)
    {
        if (Words[i].Len() > 0)
        {
            Words[i] = Words[i].Left(1).ToUpper() + Words[i].Mid(1).ToLower();
            DisplayName += Words[i];
            if (i < Words.Num() - 1)
            {
                DisplayName += TEXT(" ");
            }
        }
    }

    return DisplayName;
}
```

---

## Part 3: Claude Code Conversation Patterns

### Intent Detection

```cpp
// Intent types for Claude Code conversation
enum class EClaudeCodeIntent
{
    CreateItem,           // "create an item from SK_IronHelmet"
    CreateCollection,     // "create a collection with these items"
    AddToLoadout,         // "add IC_ArmorSet to guard loadout"
    AnalyzeMesh,          // "what slot should SK_Boots use?"
    ModifyItem,           // "change the armor rating to 20"
    BatchProcess,         // "process all meshes in /Game/Armor/"
    Query,                // "show me all helmets"
    Unknown
};

// Intent detection patterns
struct FClaudeCodeIntentPattern
{
    TArray<FString> Keywords;
    EClaudeCodeIntent Intent;
    float Confidence;
};

static TArray<FClaudeCodeIntentPattern> IntentPatterns = {
    // CreateItem patterns
    {{TEXT("create"), TEXT("item"), TEXT("from")}, EClaudeCodeIntent::CreateItem, 0.9f},
    {{TEXT("make"), TEXT("equippable")}, EClaudeCodeIntent::CreateItem, 0.8f},
    {{TEXT("turn"), TEXT("mesh"), TEXT("into")}, EClaudeCodeIntent::CreateItem, 0.85f},
    {{TEXT("generate"), TEXT("BI_")}, EClaudeCodeIntent::CreateItem, 0.9f},

    // CreateCollection patterns
    {{TEXT("create"), TEXT("collection")}, EClaudeCodeIntent::CreateCollection, 0.9f},
    {{TEXT("group"), TEXT("items")}, EClaudeCodeIntent::CreateCollection, 0.8f},
    {{TEXT("make"), TEXT("IC_")}, EClaudeCodeIntent::CreateCollection, 0.9f},

    // AddToLoadout patterns
    {{TEXT("add"), TEXT("loadout")}, EClaudeCodeIntent::AddToLoadout, 0.85f},
    {{TEXT("assign"), TEXT("npc")}, EClaudeCodeIntent::AddToLoadout, 0.8f},
    {{TEXT("give"), TEXT("guard")}, EClaudeCodeIntent::AddToLoadout, 0.75f},

    // AnalyzeMesh patterns
    {{TEXT("what"), TEXT("slot")}, EClaudeCodeIntent::AnalyzeMesh, 0.9f},
    {{TEXT("analyze"), TEXT("mesh")}, EClaudeCodeIntent::AnalyzeMesh, 0.9f},
    {{TEXT("which"), TEXT("type")}, EClaudeCodeIntent::AnalyzeMesh, 0.8f},

    // BatchProcess patterns
    {{TEXT("process"), TEXT("all")}, EClaudeCodeIntent::BatchProcess, 0.9f},
    {{TEXT("batch"), TEXT("generate")}, EClaudeCodeIntent::BatchProcess, 0.9f},
    {{TEXT("convert"), TEXT("folder")}, EClaudeCodeIntent::BatchProcess, 0.85f},
};
```

### NLP Extraction Helpers

```cpp
// Extract mesh path from natural language
FString ExtractMeshPath(const FString& UserInput)
{
    // Pattern: /Game/.../*.uasset or SK_* or SM_*
    FRegexPattern PathPattern(TEXT("(/Game/[\\w/]+)|(SK_\\w+)|(SM_\\w+)"));
    FRegexMatcher Matcher(PathPattern, UserInput);

    if (Matcher.FindNext())
    {
        return Matcher.GetCaptureGroup(0);
    }

    return FString();
}

// Extract slot from natural language
FGameplayTag ExtractSlot(const FString& UserInput)
{
    FString LowerInput = UserInput.ToLower();

    // Direct slot mentions
    if (LowerInput.Contains(TEXT("head")) || LowerInput.Contains(TEXT("helmet")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Head"));
    if (LowerInput.Contains(TEXT("chest")) || LowerInput.Contains(TEXT("torso")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Torso"));
    if (LowerInput.Contains(TEXT("leg")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Legs"));
    if (LowerInput.Contains(TEXT("feet")) || LowerInput.Contains(TEXT("boot")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Feet"));
    if (LowerInput.Contains(TEXT("hand")) || LowerInput.Contains(TEXT("glove")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Mesh.Hands"));
    if (LowerInput.Contains(TEXT("weapon")))
        return FGameplayTag::RequestGameplayTag(TEXT("Narrative.Equipment.Slot.Weapon"));

    return FGameplayTag();
}

// Extract numeric values
float ExtractNumericValue(const FString& UserInput, const FString& PropertyName)
{
    // Pattern: "armor rating of 20" or "armor: 20" or "20 armor"
    FString Pattern = FString::Printf(TEXT("%s[:\\s]+(\\d+\\.?\\d*)|(\\d+\\.?\\d*)\\s*%s"),
        *PropertyName, *PropertyName);

    FRegexPattern NumPattern(Pattern);
    FRegexMatcher Matcher(NumPattern, UserInput.ToLower());

    if (Matcher.FindNext())
    {
        FString NumStr = Matcher.GetCaptureGroup(1);
        if (NumStr.IsEmpty()) NumStr = Matcher.GetCaptureGroup(2);
        return FCString::Atof(*NumStr);
    }

    return 0.0f;
}

// Extract item name
FString ExtractItemName(const FString& UserInput)
{
    // Look for quoted strings first
    FRegexPattern QuotedPattern(TEXT("\"([^\"]+)\"|'([^']+)'"));
    FRegexMatcher Matcher(QuotedPattern, UserInput);

    if (Matcher.FindNext())
    {
        FString Result = Matcher.GetCaptureGroup(1);
        if (Result.IsEmpty()) Result = Matcher.GetCaptureGroup(2);
        return Result;
    }

    // Look for "called X" or "named X"
    FRegexPattern NamedPattern(TEXT("(?:called|named)\\s+(\\w+)"));
    FRegexMatcher NamedMatcher(NamedPattern, UserInput);

    if (NamedMatcher.FindNext())
    {
        return NamedMatcher.GetCaptureGroup(1);
    }

    return FString();
}
```

### Response Templates

```cpp
// Response templates for Claude Code

// Successful item creation
FString FormatItemCreatedResponse(const FString& ItemPath, const FMeshAnalysisResult& Analysis)
{
    return FString::Printf(
        TEXT("Created item at %s\n")
        TEXT("- Slot: %s (%.0f%% confidence)\n")
        TEXT("- Type: %s\n")
        TEXT("- Display Name: %s"),
        *ItemPath,
        *Analysis.InferredSlot.ToString(),
        Analysis.SlotConfidence * 100.0f,
        *UEnum::GetValueAsString(Analysis.InferredType),
        *Analysis.SuggestedDisplayName
    );
}

// Ambiguous slot prompt
FString FormatSlotPrompt(const FString& MeshName, const TArray<TPair<FGameplayTag, float>>& Candidates)
{
    FString Response = FString::Printf(
        TEXT("I detected the mesh '%s' but I'm not sure which slot it should use.\n")
        TEXT("Here are my guesses:\n"),
        *MeshName
    );

    for (int32 i = 0; i < Candidates.Num(); i++)
    {
        Response += FString::Printf(
            TEXT("%d. %s (%.0f%% confidence)\n"),
            i + 1,
            *Candidates[i].Key.ToString(),
            Candidates[i].Value * 100.0f
        );
    }

    Response += TEXT("\nWhich slot should I use? (enter number or type slot name)");
    return Response;
}

// Batch processing summary
FString FormatBatchSummary(const TArray<FPipelineProcessResult>& Results)
{
    int32 Succeeded = 0;
    int32 Failed = 0;

    for (const auto& Result : Results)
    {
        if (Result.bSuccess) Succeeded++;
        else Failed++;
    }

    FString Response = FString::Printf(
        TEXT("Batch processing complete:\n")
        TEXT("- Succeeded: %d\n")
        TEXT("- Failed: %d\n"),
        Succeeded, Failed
    );

    if (Failed > 0)
    {
        Response += TEXT("\nFailures:\n");
        for (const auto& Result : Results)
        {
            if (!Result.bSuccess)
            {
                for (const FString& Error : Result.Errors)
                {
                    Response += FString::Printf(TEXT("  - %s\n"), *Error);
                }
            }
        }
    }

    return Response;
}

// Collection creation confirmation
FString FormatCollectionCreated(const FString& CollectionPath, const TArray<FString>& Items)
{
    FString Response = FString::Printf(
        TEXT("Created collection at %s with %d items:\n"),
        *CollectionPath, Items.Num()
    );

    for (const FString& Item : Items)
    {
        Response += FString::Printf(TEXT("  - %s\n"), *Item);
    }

    return Response;
}
```

### Example Conversation Flows

#### Flow 1: Simple Item Creation

```
User: create an item from SK_IronHelmet for the head slot

Claude: Analyzing mesh SK_IronHelmet...

Created item at /Game/Items/Generated/BI_C_IronHelmet
- Slot: Narrative.Equipment.Slot.Mesh.Head (95% confidence)
- Type: Clothing
- Display Name: Iron Helmet

Would you like me to:
1. Add this to an existing collection
2. Create a new collection
3. Assign it to an NPC loadout
```

#### Flow 2: Ambiguous Slot

```
User: make SK_LeatherArmor into an equippable

Claude: Analyzing mesh SK_LeatherArmor...

I detected the mesh 'SK_LeatherArmor' but I'm not sure which slot it should use.
Here are my guesses:
1. Narrative.Equipment.Slot.Mesh.Torso (65% confidence)
2. Narrative.Equipment.Slot.Mesh.Legs (20% confidence)

Which slot should I use? (enter number or type slot name)

User: torso

Claude: Created item at /Game/Items/Generated/BI_C_LeatherArmor
- Slot: Narrative.Equipment.Slot.Mesh.Torso
- Type: Clothing
- Display Name: Leather Armor
```

#### Flow 3: Batch Processing

```
User: process all meshes in /Game/Meshes/ArmorSets/Iron/

Claude: Found 6 meshes in /Game/Meshes/ArmorSets/Iron/:
- SK_IronHelmet (Head)
- SK_IronChestplate (Torso)
- SK_IronGauntlets (Hands)
- SK_IronGreaves (Legs)
- SK_IronBoots (Feet)
- SK_IronCape (Back)

Processing...

Batch processing complete:
- Succeeded: 6
- Failed: 0

Items created:
- BI_C_IronHelmet
- BI_C_IronChestplate
- BI_C_IronGauntlets
- BI_C_IronGreaves
- BI_C_IronBoots
- BI_C_IronCape

Would you like me to create a collection with these items?

User: yes call it IC_IronArmorSet

Claude: Created collection at /Game/Items/Collections/IC_IronArmorSet with 6 items.
```

#### Flow 4: Full Pipeline

```
User: I have SK_GuardArmor meshes in /Game/Meshes/Guards/. Turn them into
items, create a collection, and assign it to the town guard NPC

Claude: Found 4 meshes:
- SK_GuardHelmet
- SK_GuardChestplate
- SK_GuardGloves
- SK_GuardBoots

Processing full pipeline...

Stage 1 - Items Created:
- BI_C_GuardHelmet
- BI_C_GuardChestplate
- BI_C_GuardGloves
- BI_C_GuardBoots

Stage 2 - Collection Created:
- IC_GuardArmorSet with 4 items

Stage 3 - Loadout Updated:
- Added IC_GuardArmorSet to NPCDef_TownGuard default_item_loadout

Pipeline complete! The town guard NPCs will now spawn with this armor.
```

---

## Part 4: Editor UI Specification

### Slate Window Design

```cpp
// SGasAbilityGeneratorPipelineWindow.h

class SGasAbilityGeneratorPipelineWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGasAbilityGeneratorPipelineWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Input section
    TSharedPtr<SEditableTextBox> MeshPathInput;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> SlotDropdown;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> TypeDropdown;
    TSharedPtr<SEditableTextBox> DisplayNameInput;

    // Analysis results display
    TSharedPtr<STextBlock> AnalysisResultText;
    TSharedPtr<SListView<TSharedPtr<FString>>> RelatedAssetsListView;

    // Collection section
    TSharedPtr<SEditableTextBox> CollectionNameInput;
    TSharedPtr<SCheckBox> CreateNewCollectionCheckbox;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> ExistingCollectionDropdown;

    // Loadout section
    TSharedPtr<SComboBox<TSharedPtr<FString>>> NPCDropdown;
    TSharedPtr<SCheckBox> AddToLoadoutCheckbox;

    // Batch mode
    TSharedPtr<SMultiLineEditableTextBox> BatchYAMLInput;
    TSharedPtr<SButton> ProcessBatchButton;

    // Output log
    TSharedPtr<SMultiLineEditableTextBox> OutputLog;

    // Button handlers
    FReply OnAnalyzeMeshClicked();
    FReply OnGenerateItemClicked();
    FReply OnProcessBatchClicked();
    FReply OnBrowseMeshClicked();

    // State
    FMeshAnalysisResult CurrentAnalysis;
    TArray<FPipelineProcessResult> BatchResults;
};
```

### UI Layout

```
+----------------------------------------------------------+
| Item Pipeline Generator                           [X]     |
+----------------------------------------------------------+
| [Single Item] [Batch Mode]                               |
+----------------------------------------------------------+
| Mesh Path: [________________________] [Browse]           |
|                                                          |
| [Analyze Mesh]                                           |
|                                                          |
| Analysis Results:                                        |
| +------------------------------------------------------+ |
| | Inferred Slot: Head (95% confidence)                 | |
| | Inferred Type: Clothing                              | |
| | Suggested Name: Iron Helmet                          | |
| | Related Assets:                                      | |
| |   - M_IronArmor_Base                                | |
| |   - T_IronArmor_Normal                              | |
| +------------------------------------------------------+ |
|                                                          |
| Override Settings:                                       |
| Slot: [Head              v]  Type: [Clothing         v] |
| Display Name: [Iron Helmet_______]                      |
|                                                          |
| Collection:                                              |
| [x] Add to collection                                   |
| ( ) Create new: [IC_IronArmorSet___]                   |
| (o) Add to existing: [IC_ArmorSets       v]            |
|                                                          |
| Loadout:                                                 |
| [x] Add to NPC loadout                                  |
| NPC: [NPCDef_TownGuard       v]                        |
|                                                          |
| [Generate Item]                                          |
|                                                          |
| Output Log:                                              |
| +------------------------------------------------------+ |
| | [12:34:56] Analyzing SK_IronHelmet...               | |
| | [12:34:56] Slot inference: Head (95%)               | |
| | [12:34:57] Created BI_C_IronHelmet                  | |
| | [12:34:57] Added to IC_IronArmorSet                 | |
| +------------------------------------------------------+ |
+----------------------------------------------------------+
```

---

## Implementation Priority

### Phase 1 (Core - Required)
1. `FPipelineMeshAnalyzer` class with slot/type inference
2. `FManifestClothingMeshConfig` struct and YAML parsing
3. Enhanced `FEquippableItemGenerator` for clothing meshes
4. Basic pipeline YAML format support

### Phase 2 (Collections)
1. Auto-creation of IC_ collections from pipeline
2. Collection YAML section parsing
3. Integration with existing `FItemCollectionGenerator`

### Phase 3 (Loadouts)
1. Auto-add collections to NPC loadouts
2. Loadout YAML section parsing
3. Integration with NPCDefinition generator

### Phase 4 (Editor UI)
1. `SGasAbilityGeneratorPipelineWindow` Slate UI
2. Mesh browser integration
3. Live analysis preview
4. Batch processing UI

### Phase 5 (Claude Code)
1. Intent detection system
2. NLP extraction helpers
3. Response template system
4. Conversation state management

---

## Testing Checklist

- [ ] Single clothing mesh to BI_ item
- [ ] EquipmentEffectValues TMap population
- [ ] ClothingMeshData.Materials array with color bindings
- [ ] Slot inference accuracy (test 20+ mesh names)
- [ ] Weapon type detection
- [ ] Related asset discovery
- [ ] Collection auto-creation
- [ ] Loadout auto-assignment
- [ ] Batch YAML processing
- [ ] Editor UI single item flow
- [ ] Editor UI batch mode
- [ ] Claude Code conversation flows

---

## References

### Narrative Pro Headers
- `NarrativeArsenal/Public/Items/EquippableItem.h` - UEquippableItem_Clothing
- `NarrativeArsenal/Public/CharacterCreator/CharacterCreatorAttributes.h` - FCharacterCreatorAttribute_Mesh
- `NarrativeArsenal/Public/Items/NarrativeItem.h` - Base item properties

### Existing Generator Code
- `GasAbilityGeneratorTypes.h:1585+` - FManifestEquippableItemDefinition
- `GasAbilityGeneratorGenerators.cpp` - FEquippableItemGenerator::Generate

### Screenshots Analyzed
- BI_C_ExampleChestplate - ClothingMeshData, EquipmentEffectValues, Stats
- IC_ExampleArmorSet - Item collection structure
- DT_Armor - DataTable with FLootTableRow
- ItemLoadout_Seth - NPC loadout configuration
