# GasAbilityGenerator v3.10 Automation Handoff

**Created:** 2026-01-14
**Purpose:** Close automation gaps for Narrative Pro asset types
**Target:** Increase EI_ from 92% to 98%, BPA_ from 88% to 95%

---

## Overview of Changes

| Asset Type | Current | Target | Changes Required |
|------------|---------|--------|------------------|
| EI_ (Equippable Items) | 92% | 98% | 10 RangedWeaponItem properties + TMap attachments |
| BPA_ (Activities) | 88% | 95% | Parent class flexibility |
| NPCDef_ | 90% | 90% | Already complete |
| CD_ | 85% | 85% | Already complete |
| TDS | 85% | 85% | Already complete |

---

## Part 1: Manifest Struct Additions (GasAbilityGeneratorTypes.h)

### 1.1 Add to FManifestEquippableItemDefinition

Find the struct `FManifestEquippableItemDefinition` and add these fields after the existing RangedWeaponItem properties:

```cpp
// =============================================================================
// v3.10: Additional RangedWeaponItem Properties
// =============================================================================

// Crosshair widget for aiming
UPROPERTY()
FString CrosshairWidget;

// Aim render properties
UPROPERTY()
float AimWeaponRenderFOV = 0.0f;

UPROPERTY()
float AimWeaponFStop = 0.0f;

// Additional spread properties
UPROPERTY()
float MoveSpeedAddDegrees = 0.0f;

UPROPERTY()
float CrouchSpreadMultiplier = 1.0f;

UPROPERTY()
float AimSpreadMultiplier = 1.0f;

// Recoil impulse vectors (aim down sights)
UPROPERTY()
FVector RecoilImpulseTranslationMin = FVector::ZeroVector;

UPROPERTY()
FVector RecoilImpulseTranslationMax = FVector::ZeroVector;

// Recoil impulse vectors (hip fire)
UPROPERTY()
FVector HipRecoilImpulseTranslationMin = FVector::ZeroVector;

UPROPERTY()
FVector HipRecoilImpulseTranslationMax = FVector::ZeroVector;

// =============================================================================
// v3.10: Weapon Attachment Slot Configuration (full TMap automation)
// =============================================================================

// Structure for a single attachment slot config
struct FManifestWeaponAttachmentSlot
{
    FString Slot;      // GameplayTag string e.g. "Narrative.Equipment.Weapon.AttachSlot.Sight"
    FString Socket;    // Socket name on mesh
    FVector Offset = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
};

UPROPERTY()
TArray<FManifestWeaponAttachmentSlot> WeaponAttachmentSlots;

// =============================================================================
// v3.10: Clothing Mesh Materials Automation
// =============================================================================

struct FManifestMaterialScalarParam
{
    FString Name;
    float Value = 0.0f;
};

struct FManifestMaterialVectorParam
{
    FString Name;
    FLinearColor Value = FLinearColor::White;
};

struct FManifestClothingMeshMaterial
{
    FString Material;  // Soft path to material
    TArray<FManifestMaterialScalarParam> ScalarParams;
    TArray<FManifestMaterialVectorParam> VectorParams;
};

UPROPERTY()
TArray<FManifestClothingMeshMaterial> ClothingMeshMaterials;

// Morph targets for clothing mesh
UPROPERTY()
TArray<FString> ClothingMeshMorphs;
```

### 1.2 Update ComputeHash() for FManifestEquippableItemDefinition

Add these lines to the `ComputeHash()` function:

```cpp
// v3.10: Hash new RangedWeaponItem properties
HashString += CrosshairWidget;
HashString += FString::Printf(TEXT("AimRenderFOV:%.2f,"), AimWeaponRenderFOV);
HashString += FString::Printf(TEXT("AimFStop:%.2f,"), AimWeaponFStop);
HashString += FString::Printf(TEXT("MoveSpeedAdd:%.2f,"), MoveSpeedAddDegrees);
HashString += FString::Printf(TEXT("CrouchSpreadMult:%.2f,"), CrouchSpreadMultiplier);
HashString += FString::Printf(TEXT("AimSpreadMult:%.2f,"), AimSpreadMultiplier);
HashString += FString::Printf(TEXT("RecoilMin:%s,"), *RecoilImpulseTranslationMin.ToString());
HashString += FString::Printf(TEXT("RecoilMax:%s,"), *RecoilImpulseTranslationMax.ToString());
HashString += FString::Printf(TEXT("HipRecoilMin:%s,"), *HipRecoilImpulseTranslationMin.ToString());
HashString += FString::Printf(TEXT("HipRecoilMax:%s,"), *HipRecoilImpulseTranslationMax.ToString());

// v3.10: Hash weapon attachment slots
for (const auto& Slot : WeaponAttachmentSlots)
{
    HashString += FString::Printf(TEXT("AttachSlot:%s:%s:%s:%s,"),
        *Slot.Slot, *Slot.Socket, *Slot.Offset.ToString(), *Slot.Rotation.ToString());
}

// v3.10: Hash clothing mesh materials
for (const auto& Mat : ClothingMeshMaterials)
{
    HashString += FString::Printf(TEXT("ClothMat:%s,"), *Mat.Material);
    for (const auto& Scalar : Mat.ScalarParams)
    {
        HashString += FString::Printf(TEXT("S:%s:%.2f,"), *Scalar.Name, Scalar.Value);
    }
    for (const auto& Vector : Mat.VectorParams)
    {
        HashString += FString::Printf(TEXT("V:%s:%s,"), *Vector.Name, *Vector.Value.ToString());
    }
}

// v3.10: Hash clothing mesh morphs
for (const FString& Morph : ClothingMeshMorphs)
{
    HashString += FString::Printf(TEXT("Morph:%s,"), *Morph);
}
```

---

## Part 2: Parser Additions (GasAbilityGeneratorParser.cpp)

### 2.1 Add to ParseEquippableItems() function

Find the section where RangedWeaponItem properties are parsed and add:

```cpp
// =============================================================================
// v3.10: Additional RangedWeaponItem Properties
// =============================================================================

// Crosshair widget
if (Line.Contains(TEXT("crosshair_widget:")))
{
    CurrentItem.CrosshairWidget = Line.Replace(TEXT("crosshair_widget:"), TEXT("")).TrimStartAndEnd();
}
// Aim render FOV
else if (Line.Contains(TEXT("aim_weapon_render_fov:")))
{
    CurrentItem.AimWeaponRenderFOV = FCString::Atof(*Line.Replace(TEXT("aim_weapon_render_fov:"), TEXT("")).TrimStartAndEnd());
}
// Aim F-Stop
else if (Line.Contains(TEXT("aim_weapon_fstop:")))
{
    CurrentItem.AimWeaponFStop = FCString::Atof(*Line.Replace(TEXT("aim_weapon_fstop:"), TEXT("")).TrimStartAndEnd());
}
// Move speed spread penalty
else if (Line.Contains(TEXT("move_speed_add_degrees:")))
{
    CurrentItem.MoveSpeedAddDegrees = FCString::Atof(*Line.Replace(TEXT("move_speed_add_degrees:"), TEXT("")).TrimStartAndEnd());
}
// Crouch spread multiplier
else if (Line.Contains(TEXT("crouch_spread_multiplier:")))
{
    CurrentItem.CrouchSpreadMultiplier = FCString::Atof(*Line.Replace(TEXT("crouch_spread_multiplier:"), TEXT("")).TrimStartAndEnd());
}
// Aim spread multiplier
else if (Line.Contains(TEXT("aim_spread_multiplier:")))
{
    CurrentItem.AimSpreadMultiplier = FCString::Atof(*Line.Replace(TEXT("aim_spread_multiplier:"), TEXT("")).TrimStartAndEnd());
}
// Recoil impulse vectors - parse as YAML arrays [x, y, z]
else if (Line.Contains(TEXT("recoil_impulse_translation_min:")))
{
    CurrentItem.RecoilImpulseTranslationMin = ParseVector(Line.Replace(TEXT("recoil_impulse_translation_min:"), TEXT("")).TrimStartAndEnd());
}
else if (Line.Contains(TEXT("recoil_impulse_translation_max:")))
{
    CurrentItem.RecoilImpulseTranslationMax = ParseVector(Line.Replace(TEXT("recoil_impulse_translation_max:"), TEXT("")).TrimStartAndEnd());
}
else if (Line.Contains(TEXT("hip_recoil_impulse_translation_min:")))
{
    CurrentItem.HipRecoilImpulseTranslationMin = ParseVector(Line.Replace(TEXT("hip_recoil_impulse_translation_min:"), TEXT("")).TrimStartAndEnd());
}
else if (Line.Contains(TEXT("hip_recoil_impulse_translation_max:")))
{
    CurrentItem.HipRecoilImpulseTranslationMax = ParseVector(Line.Replace(TEXT("hip_recoil_impulse_translation_max:"), TEXT("")).TrimStartAndEnd());
}
```

### 2.2 Add ParseVector() Helper Function

Add this helper function if it doesn't exist:

```cpp
// Parse YAML vector format [x, y, z] to FVector
FVector FGasAbilityGeneratorParser::ParseVector(const FString& VectorString)
{
    FVector Result = FVector::ZeroVector;

    // Remove brackets and split by comma
    FString CleanString = VectorString;
    CleanString.RemoveFromStart(TEXT("["));
    CleanString.RemoveFromEnd(TEXT("]"));

    TArray<FString> Components;
    CleanString.ParseIntoArray(Components, TEXT(","), true);

    if (Components.Num() >= 3)
    {
        Result.X = FCString::Atof(*Components[0].TrimStartAndEnd());
        Result.Y = FCString::Atof(*Components[1].TrimStartAndEnd());
        Result.Z = FCString::Atof(*Components[2].TrimStartAndEnd());
    }

    return Result;
}
```

### 2.3 Add Weapon Attachment Slot Parsing

Add state tracking for nested parsing:

```cpp
// State tracking for weapon_attachment_slots section
bool bInWeaponAttachmentSlots = false;
FManifestWeaponAttachmentSlot CurrentAttachmentSlot;

// In the main parsing loop, add:
else if (Line.Contains(TEXT("weapon_attachment_slots:")))
{
    bInWeaponAttachmentSlots = true;
}
else if (bInWeaponAttachmentSlots)
{
    // Check for new slot entry (starts with "- slot:")
    if (Line.TrimStart().StartsWith(TEXT("- slot:")))
    {
        // Save previous slot if valid
        if (!CurrentAttachmentSlot.Slot.IsEmpty())
        {
            CurrentItem.WeaponAttachmentSlots.Add(CurrentAttachmentSlot);
        }
        // Start new slot
        CurrentAttachmentSlot = FManifestWeaponAttachmentSlot();
        CurrentAttachmentSlot.Slot = Line.Replace(TEXT("- slot:"), TEXT("")).TrimStartAndEnd();
    }
    else if (Line.Contains(TEXT("socket:")))
    {
        CurrentAttachmentSlot.Socket = Line.Replace(TEXT("socket:"), TEXT("")).TrimStartAndEnd();
    }
    else if (Line.Contains(TEXT("offset:")))
    {
        CurrentAttachmentSlot.Offset = ParseVector(Line.Replace(TEXT("offset:"), TEXT("")).TrimStartAndEnd());
    }
    else if (Line.Contains(TEXT("rotation:")))
    {
        FVector RotVec = ParseVector(Line.Replace(TEXT("rotation:"), TEXT("")).TrimStartAndEnd());
        CurrentAttachmentSlot.Rotation = FRotator(RotVec.Y, RotVec.Z, RotVec.X); // Pitch, Yaw, Roll
    }
    // Exit section on next top-level property or new item
    else if (GetIndentLevel(Line) <= 2 && !Line.TrimStart().StartsWith(TEXT("-")))
    {
        // Save last slot and exit section
        if (!CurrentAttachmentSlot.Slot.IsEmpty())
        {
            CurrentItem.WeaponAttachmentSlots.Add(CurrentAttachmentSlot);
        }
        bInWeaponAttachmentSlots = false;
    }
}
```

### 2.4 Add Clothing Mesh Materials Parsing

```cpp
// State tracking for clothing_mesh_materials section
bool bInClothingMeshMaterials = false;
bool bInMaterialScalarParams = false;
bool bInMaterialVectorParams = false;
FManifestClothingMeshMaterial CurrentClothingMaterial;
FManifestMaterialScalarParam CurrentScalarParam;
FManifestMaterialVectorParam CurrentVectorParam;

// In the main parsing loop:
else if (Line.Contains(TEXT("clothing_mesh_materials:")))
{
    bInClothingMeshMaterials = true;
}
else if (bInClothingMeshMaterials)
{
    if (Line.TrimStart().StartsWith(TEXT("- material:")))
    {
        // Save previous material if valid
        if (!CurrentClothingMaterial.Material.IsEmpty())
        {
            CurrentItem.ClothingMeshMaterials.Add(CurrentClothingMaterial);
        }
        CurrentClothingMaterial = FManifestClothingMeshMaterial();
        CurrentClothingMaterial.Material = Line.Replace(TEXT("- material:"), TEXT("")).TrimStartAndEnd();
        bInMaterialScalarParams = false;
        bInMaterialVectorParams = false;
    }
    else if (Line.Contains(TEXT("scalar_params:")))
    {
        bInMaterialScalarParams = true;
        bInMaterialVectorParams = false;
    }
    else if (Line.Contains(TEXT("vector_params:")))
    {
        bInMaterialVectorParams = true;
        bInMaterialScalarParams = false;
    }
    else if (bInMaterialScalarParams && Line.TrimStart().StartsWith(TEXT("- name:")))
    {
        CurrentScalarParam = FManifestMaterialScalarParam();
        CurrentScalarParam.Name = Line.Replace(TEXT("- name:"), TEXT("")).TrimStartAndEnd();
    }
    else if (bInMaterialScalarParams && Line.Contains(TEXT("value:")) && !CurrentScalarParam.Name.IsEmpty())
    {
        CurrentScalarParam.Value = FCString::Atof(*Line.Replace(TEXT("value:"), TEXT("")).TrimStartAndEnd());
        CurrentClothingMaterial.ScalarParams.Add(CurrentScalarParam);
        CurrentScalarParam = FManifestMaterialScalarParam();
    }
    else if (bInMaterialVectorParams && Line.TrimStart().StartsWith(TEXT("- name:")))
    {
        CurrentVectorParam = FManifestMaterialVectorParam();
        CurrentVectorParam.Name = Line.Replace(TEXT("- name:"), TEXT("")).TrimStartAndEnd();
    }
    else if (bInMaterialVectorParams && Line.Contains(TEXT("value:")) && !CurrentVectorParam.Name.IsEmpty())
    {
        FVector4 Vec = ParseVector4(Line.Replace(TEXT("value:"), TEXT("")).TrimStartAndEnd());
        CurrentVectorParam.Value = FLinearColor(Vec.X, Vec.Y, Vec.Z, Vec.W);
        CurrentClothingMaterial.VectorParams.Add(CurrentVectorParam);
        CurrentVectorParam = FManifestMaterialVectorParam();
    }
    // Exit section
    else if (GetIndentLevel(Line) <= 2 && !Line.TrimStart().StartsWith(TEXT("-")))
    {
        if (!CurrentClothingMaterial.Material.IsEmpty())
        {
            CurrentItem.ClothingMeshMaterials.Add(CurrentClothingMaterial);
        }
        bInClothingMeshMaterials = false;
        bInMaterialScalarParams = false;
        bInMaterialVectorParams = false;
    }
}

// Clothing mesh morphs (simple string array)
else if (Line.Contains(TEXT("clothing_mesh_morphs:")))
{
    bInClothingMeshMorphs = true;
}
else if (bInClothingMeshMorphs)
{
    if (Line.TrimStart().StartsWith(TEXT("-")))
    {
        FString MorphName = Line.TrimStart();
        MorphName.RemoveFromStart(TEXT("-"));
        CurrentItem.ClothingMeshMorphs.Add(MorphName.TrimStartAndEnd());
    }
    else if (GetIndentLevel(Line) <= 2)
    {
        bInClothingMeshMorphs = false;
    }
}
```

### 2.5 Add ParseVector4() Helper

```cpp
// Parse YAML vector4 format [x, y, z, w] to FVector4
FVector4 FGasAbilityGeneratorParser::ParseVector4(const FString& VectorString)
{
    FVector4 Result(0, 0, 0, 1);

    FString CleanString = VectorString;
    CleanString.RemoveFromStart(TEXT("["));
    CleanString.RemoveFromEnd(TEXT("]"));

    TArray<FString> Components;
    CleanString.ParseIntoArray(Components, TEXT(","), true);

    if (Components.Num() >= 1) Result.X = FCString::Atof(*Components[0].TrimStartAndEnd());
    if (Components.Num() >= 2) Result.Y = FCString::Atof(*Components[1].TrimStartAndEnd());
    if (Components.Num() >= 3) Result.Z = FCString::Atof(*Components[2].TrimStartAndEnd());
    if (Components.Num() >= 4) Result.W = FCString::Atof(*Components[3].TrimStartAndEnd());

    return Result;
}
```

---

## Part 3: Generator Implementation (GasAbilityGeneratorGenerators.cpp)

### 3.1 FEquippableItemGenerator - Add v3.10 Properties

Find `FEquippableItemGenerator::Generate()` and add after the existing RangedWeaponItem property section:

```cpp
// =============================================================================
// v3.10: Additional RangedWeaponItem Properties
// =============================================================================

// Check if this is a RangedWeaponItem (or inherits from it)
bool bIsRangedWeapon = Definition.ParentClass.Contains(TEXT("RangedWeaponItem"));

if (bIsRangedWeapon)
{
    // CrosshairWidget (TSoftClassPtr<UCrosshairWidget>)
    if (!Definition.CrosshairWidget.IsEmpty())
    {
        if (FSoftClassProperty* CrosshairProp = CastField<FSoftClassProperty>(ItemClass->FindPropertyByName(TEXT("CrosshairWidget"))))
        {
            FString WidgetPath = BuildAssetPath(Definition.CrosshairWidget, Definition.Folder, TEXT("Widgets"));
            if (!WidgetPath.EndsWith(TEXT("_C")))
            {
                WidgetPath += TEXT("_C");
            }
            FSoftObjectPath SoftPath(WidgetPath);
            CrosshairProp->SetPropertyValue_InContainer(CDO, FSoftObjectPtr(SoftPath));
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set CrosshairWidget: %s"), *WidgetPath);
        }
    }

    // AimWeaponRenderFOV (float)
    if (Definition.AimWeaponRenderFOV != 0.0f)
    {
        if (FFloatProperty* FOVProp = CastField<FFloatProperty>(ItemClass->FindPropertyByName(TEXT("AimWeaponRenderFOV"))))
        {
            FOVProp->SetPropertyValue_InContainer(CDO, Definition.AimWeaponRenderFOV);
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set AimWeaponRenderFOV: %.2f"), Definition.AimWeaponRenderFOV);
        }
    }

    // AimWeaponFStop (float)
    if (Definition.AimWeaponFStop != 0.0f)
    {
        if (FFloatProperty* FStopProp = CastField<FFloatProperty>(ItemClass->FindPropertyByName(TEXT("AimWeaponFStop"))))
        {
            FStopProp->SetPropertyValue_InContainer(CDO, Definition.AimWeaponFStop);
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set AimWeaponFStop: %.2f"), Definition.AimWeaponFStop);
        }
    }

    // MoveSpeedAddDegrees (float)
    if (Definition.MoveSpeedAddDegrees != 0.0f)
    {
        if (FFloatProperty* MoveSpeedProp = CastField<FFloatProperty>(ItemClass->FindPropertyByName(TEXT("MoveSpeedAddDegrees"))))
        {
            MoveSpeedProp->SetPropertyValue_InContainer(CDO, Definition.MoveSpeedAddDegrees);
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set MoveSpeedAddDegrees: %.2f"), Definition.MoveSpeedAddDegrees);
        }
    }

    // CrouchSpreadMultiplier (float)
    if (Definition.CrouchSpreadMultiplier != 1.0f)
    {
        if (FFloatProperty* CrouchProp = CastField<FFloatProperty>(ItemClass->FindPropertyByName(TEXT("CrouchSpreadMultiplier"))))
        {
            CrouchProp->SetPropertyValue_InContainer(CDO, Definition.CrouchSpreadMultiplier);
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set CrouchSpreadMultiplier: %.2f"), Definition.CrouchSpreadMultiplier);
        }
    }

    // AimSpreadMultiplier (float)
    if (Definition.AimSpreadMultiplier != 1.0f)
    {
        if (FFloatProperty* AimProp = CastField<FFloatProperty>(ItemClass->FindPropertyByName(TEXT("AimSpreadMultiplier"))))
        {
            AimProp->SetPropertyValue_InContainer(CDO, Definition.AimSpreadMultiplier);
            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set AimSpreadMultiplier: %.2f"), Definition.AimSpreadMultiplier);
        }
    }

    // RecoilImpulseTranslationMin (FVector)
    if (!Definition.RecoilImpulseTranslationMin.IsZero())
    {
        if (FStructProperty* RecoilMinProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("RecoilImpulseTranslationMin"))))
        {
            FVector* VecPtr = RecoilMinProp->ContainerPtrToValuePtr<FVector>(CDO);
            if (VecPtr)
            {
                *VecPtr = Definition.RecoilImpulseTranslationMin;
                UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set RecoilImpulseTranslationMin: %s"), *Definition.RecoilImpulseTranslationMin.ToString());
            }
        }
    }

    // RecoilImpulseTranslationMax (FVector)
    if (!Definition.RecoilImpulseTranslationMax.IsZero())
    {
        if (FStructProperty* RecoilMaxProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("RecoilImpulseTranslationMax"))))
        {
            FVector* VecPtr = RecoilMaxProp->ContainerPtrToValuePtr<FVector>(CDO);
            if (VecPtr)
            {
                *VecPtr = Definition.RecoilImpulseTranslationMax;
                UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set RecoilImpulseTranslationMax: %s"), *Definition.RecoilImpulseTranslationMax.ToString());
            }
        }
    }

    // HipRecoilImpulseTranslationMin (FVector)
    if (!Definition.HipRecoilImpulseTranslationMin.IsZero())
    {
        if (FStructProperty* HipRecoilMinProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("HipRecoilImpulseTranslationMin"))))
        {
            FVector* VecPtr = HipRecoilMinProp->ContainerPtrToValuePtr<FVector>(CDO);
            if (VecPtr)
            {
                *VecPtr = Definition.HipRecoilImpulseTranslationMin;
                UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set HipRecoilImpulseTranslationMin: %s"), *Definition.HipRecoilImpulseTranslationMin.ToString());
            }
        }
    }

    // HipRecoilImpulseTranslationMax (FVector)
    if (!Definition.HipRecoilImpulseTranslationMax.IsZero())
    {
        if (FStructProperty* HipRecoilMaxProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("HipRecoilImpulseTranslationMax"))))
        {
            FVector* VecPtr = HipRecoilMaxProp->ContainerPtrToValuePtr<FVector>(CDO);
            if (VecPtr)
            {
                *VecPtr = Definition.HipRecoilImpulseTranslationMax;
                UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set HipRecoilImpulseTranslationMax: %s"), *Definition.HipRecoilImpulseTranslationMax.ToString());
            }
        }
    }
}
```

### 3.2 FEquippableItemGenerator - Weapon Attachment Slots TMap

Add after the recoil properties:

```cpp
// =============================================================================
// v3.10: Weapon Attachment Slot Configuration (TMap automation)
// =============================================================================

if (Definition.WeaponAttachmentSlots.Num() > 0)
{
    // Find the WeaponAttachmentConfiguration property (TMap<FGameplayTag, FWeaponAttachmentSlotConfig>)
    FMapProperty* AttachMapProp = CastField<FMapProperty>(ItemClass->FindPropertyByName(TEXT("WeaponAttachmentConfiguration")));

    if (AttachMapProp)
    {
        void* MapPtr = AttachMapProp->ContainerPtrToValuePtr<void>(CDO);
        FScriptMapHelper MapHelper(AttachMapProp, MapPtr);

        // Clear existing entries
        MapHelper.EmptyValues();

        for (const FManifestWeaponAttachmentSlot& SlotDef : Definition.WeaponAttachmentSlots)
        {
            // Create the key (FGameplayTag)
            FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*SlotDef.Slot), false);
            if (!SlotTag.IsValid())
            {
                UE_LOG(LogGasAbilityGenerator, Warning, TEXT("  [EI] Invalid attachment slot tag: %s"), *SlotDef.Slot);
                continue;
            }

            // Add entry to map
            int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();

            // Set the key
            FStructProperty* KeyProp = CastField<FStructProperty>(AttachMapProp->KeyProp);
            if (KeyProp)
            {
                uint8* KeyPtr = MapHelper.GetKeyPtr(NewIndex);
                FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(KeyPtr);
                *TagPtr = SlotTag;
            }

            // Set the value (FWeaponAttachmentSlotConfig struct)
            uint8* ValuePtr = MapHelper.GetValuePtr(NewIndex);
            FStructProperty* ValueProp = CastField<FStructProperty>(AttachMapProp->ValueProp);

            if (ValueProp && ValuePtr)
            {
                UScriptStruct* SlotConfigStruct = ValueProp->Struct;

                // Set Socket (FName)
                if (FNameProperty* SocketProp = CastField<FNameProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Socket"))))
                {
                    SocketProp->SetPropertyValue_InContainer(ValuePtr, FName(*SlotDef.Socket));
                }

                // Set Offset (FVector)
                if (FStructProperty* OffsetProp = CastField<FStructProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Offset"))))
                {
                    FVector* OffsetPtr = OffsetProp->ContainerPtrToValuePtr<FVector>(ValuePtr);
                    if (OffsetPtr)
                    {
                        *OffsetPtr = SlotDef.Offset;
                    }
                }

                // Set Rotation (FRotator)
                if (FStructProperty* RotProp = CastField<FStructProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Rotation"))))
                {
                    FRotator* RotPtr = RotProp->ContainerPtrToValuePtr<FRotator>(ValuePtr);
                    if (RotPtr)
                    {
                        *RotPtr = SlotDef.Rotation;
                    }
                }
            }

            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Added attachment slot: %s -> Socket:%s Offset:%s Rot:%s"),
                *SlotDef.Slot, *SlotDef.Socket, *SlotDef.Offset.ToString(), *SlotDef.Rotation.ToString());
        }

        // Rehash the map for proper serialization
        MapHelper.Rehash();

        UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set %d weapon attachment slots"), Definition.WeaponAttachmentSlots.Num());
    }
    else
    {
        UE_LOG(LogGasAbilityGenerator, Warning, TEXT("  [EI] WeaponAttachmentConfiguration property not found - may not be a WeaponItem"));
    }
}
```

### 3.3 FEquippableItemGenerator - Clothing Mesh Materials

Add after weapon attachment slots:

```cpp
// =============================================================================
// v3.10: Clothing Mesh Materials Automation
// =============================================================================

if (Definition.ClothingMeshMaterials.Num() > 0)
{
    // Find ClothingMeshData struct first
    FStructProperty* MeshDataProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("ClothingMeshData")));

    if (MeshDataProp)
    {
        void* MeshDataPtr = MeshDataProp->ContainerPtrToValuePtr<void>(CDO);
        UScriptStruct* MeshDataStruct = MeshDataProp->Struct;

        // Find MeshMaterials array within ClothingMeshData
        FArrayProperty* MaterialsArrayProp = CastField<FArrayProperty>(MeshDataStruct->FindPropertyByName(TEXT("MeshMaterials")));

        if (MaterialsArrayProp && MeshDataPtr)
        {
            void* ArrayPtr = MaterialsArrayProp->ContainerPtrToValuePtr<void>(MeshDataPtr);
            FScriptArrayHelper ArrayHelper(MaterialsArrayProp, ArrayPtr);

            // Clear existing and resize
            ArrayHelper.EmptyValues();
            ArrayHelper.Resize(Definition.ClothingMeshMaterials.Num());

            FStructProperty* ElementProp = CastField<FStructProperty>(MaterialsArrayProp->Inner);

            for (int32 i = 0; i < Definition.ClothingMeshMaterials.Num(); i++)
            {
                const FManifestClothingMeshMaterial& MatDef = Definition.ClothingMeshMaterials[i];
                uint8* ElementPtr = ArrayHelper.GetRawPtr(i);

                if (ElementProp && ElementPtr)
                {
                    UScriptStruct* MatStruct = ElementProp->Struct;

                    // Set Material (TSoftObjectPtr<UMaterialInterface>)
                    if (FSoftObjectProperty* MatProp = CastField<FSoftObjectProperty>(MatStruct->FindPropertyByName(TEXT("Material"))))
                    {
                        FSoftObjectPath MatPath(MatDef.Material);
                        MatProp->SetPropertyValue_InContainer(ElementPtr, FSoftObjectPtr(MatPath));
                    }

                    // Set ScalarParameters (TArray<FCreatorMaterialScalarParam>)
                    if (FArrayProperty* ScalarArrayProp = CastField<FArrayProperty>(MatStruct->FindPropertyByName(TEXT("ScalarParameters"))))
                    {
                        void* ScalarArrayPtr = ScalarArrayProp->ContainerPtrToValuePtr<void>(ElementPtr);
                        FScriptArrayHelper ScalarHelper(ScalarArrayProp, ScalarArrayPtr);
                        ScalarHelper.EmptyValues();
                        ScalarHelper.Resize(MatDef.ScalarParams.Num());

                        FStructProperty* ScalarElementProp = CastField<FStructProperty>(ScalarArrayProp->Inner);

                        for (int32 j = 0; j < MatDef.ScalarParams.Num(); j++)
                        {
                            const FManifestMaterialScalarParam& ScalarDef = MatDef.ScalarParams[j];
                            uint8* ScalarPtr = ScalarHelper.GetRawPtr(j);

                            if (ScalarElementProp && ScalarPtr)
                            {
                                UScriptStruct* ScalarStruct = ScalarElementProp->Struct;

                                if (FNameProperty* NameProp = CastField<FNameProperty>(ScalarStruct->FindPropertyByName(TEXT("ParameterName"))))
                                {
                                    NameProp->SetPropertyValue_InContainer(ScalarPtr, FName(*ScalarDef.Name));
                                }
                                if (FFloatProperty* ValueProp = CastField<FFloatProperty>(ScalarStruct->FindPropertyByName(TEXT("Value"))))
                                {
                                    ValueProp->SetPropertyValue_InContainer(ScalarPtr, ScalarDef.Value);
                                }
                            }
                        }
                    }

                    // Set VectorParameters (TArray<FCreatorMaterialVectorParam>)
                    if (FArrayProperty* VectorArrayProp = CastField<FArrayProperty>(MatStruct->FindPropertyByName(TEXT("VectorParameters"))))
                    {
                        void* VectorArrayPtr = VectorArrayProp->ContainerPtrToValuePtr<void>(ElementPtr);
                        FScriptArrayHelper VectorHelper(VectorArrayProp, VectorArrayPtr);
                        VectorHelper.EmptyValues();
                        VectorHelper.Resize(MatDef.VectorParams.Num());

                        FStructProperty* VectorElementProp = CastField<FStructProperty>(VectorArrayProp->Inner);

                        for (int32 j = 0; j < MatDef.VectorParams.Num(); j++)
                        {
                            const FManifestMaterialVectorParam& VectorDef = MatDef.VectorParams[j];
                            uint8* VectorPtr = VectorHelper.GetRawPtr(j);

                            if (VectorElementProp && VectorPtr)
                            {
                                UScriptStruct* VectorStruct = VectorElementProp->Struct;

                                if (FNameProperty* NameProp = CastField<FNameProperty>(VectorStruct->FindPropertyByName(TEXT("ParameterName"))))
                                {
                                    NameProp->SetPropertyValue_InContainer(VectorPtr, FName(*VectorDef.Name));
                                }
                                if (FStructProperty* ColorProp = CastField<FStructProperty>(VectorStruct->FindPropertyByName(TEXT("Value"))))
                                {
                                    FLinearColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<FLinearColor>(VectorPtr);
                                    if (ColorPtr)
                                    {
                                        *ColorPtr = VectorDef.Value;
                                    }
                                }
                            }
                        }
                    }
                }

                UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set clothing material[%d]: %s (%d scalars, %d vectors)"),
                    i, *MatDef.Material, MatDef.ScalarParams.Num(), MatDef.VectorParams.Num());
            }

            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set %d clothing mesh materials"), Definition.ClothingMeshMaterials.Num());
        }
    }
}

// =============================================================================
// v3.10: Clothing Mesh Morphs Automation
// =============================================================================

if (Definition.ClothingMeshMorphs.Num() > 0)
{
    FStructProperty* MeshDataProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(TEXT("ClothingMeshData")));

    if (MeshDataProp)
    {
        void* MeshDataPtr = MeshDataProp->ContainerPtrToValuePtr<void>(CDO);
        UScriptStruct* MeshDataStruct = MeshDataProp->Struct;

        // Find Morphs array within ClothingMeshData
        FArrayProperty* MorphsArrayProp = CastField<FArrayProperty>(MeshDataStruct->FindPropertyByName(TEXT("Morphs")));

        if (MorphsArrayProp && MeshDataPtr)
        {
            void* ArrayPtr = MorphsArrayProp->ContainerPtrToValuePtr<void>(MeshDataPtr);
            FScriptArrayHelper ArrayHelper(MorphsArrayProp, ArrayPtr);

            ArrayHelper.EmptyValues();
            ArrayHelper.Resize(Definition.ClothingMeshMorphs.Num());

            FStructProperty* ElementProp = CastField<FStructProperty>(MorphsArrayProp->Inner);

            for (int32 i = 0; i < Definition.ClothingMeshMorphs.Num(); i++)
            {
                const FString& MorphName = Definition.ClothingMeshMorphs[i];
                uint8* ElementPtr = ArrayHelper.GetRawPtr(i);

                if (ElementProp && ElementPtr)
                {
                    UScriptStruct* MorphStruct = ElementProp->Struct;

                    // Set MorphTargetName (FName)
                    if (FNameProperty* NameProp = CastField<FNameProperty>(MorphStruct->FindPropertyByName(TEXT("MorphTargetName"))))
                    {
                        NameProp->SetPropertyValue_InContainer(ElementPtr, FName(*MorphName));
                    }
                }
            }

            UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [EI] Set %d clothing mesh morphs"), Definition.ClothingMeshMorphs.Num());
        }
    }
}
```

---

## Part 4: Example Manifest Usage

### 4.1 Full RangedWeaponItem Example

```yaml
equippable_items:
  - name: EI_AdvancedRifle
    parent_class: RangedWeaponItem
    folder: Items/Weapons
    display_name: "Advanced Combat Rifle"
    description: "Military-grade rifle with advanced recoil compensation."
    equipment_slot: Narrative.Equipment.Slot.Weapon

    # Existing v3.4 properties
    weapon_hand: TwoHanded
    attack_damage: 35.0
    heavy_attack_damage_multiplier: 1.5
    allow_manual_reload: true
    clip_size: 30
    base_spread_degrees: 1.5
    max_spread_degrees: 6.0
    spread_fire_bump: 0.4
    spread_decrease_speed: 5.0
    aim_fov_pct: 0.6

    # v3.10: New spread properties
    move_speed_add_degrees: 2.5
    crouch_spread_multiplier: 0.6
    aim_spread_multiplier: 0.4

    # v3.10: Aim render properties
    crosshair_widget: WBP_RifleCrosshair
    aim_weapon_render_fov: 55.0
    aim_weapon_fstop: 2.8

    # v3.10: Recoil impulse vectors
    recoil_impulse_translation_min: [-0.5, -1.0, 2.0]
    recoil_impulse_translation_max: [0.5, 1.0, 4.0]
    hip_recoil_impulse_translation_min: [-1.0, -2.0, 3.0]
    hip_recoil_impulse_translation_max: [1.0, 2.0, 6.0]

    # v3.10: Weapon attachment slots
    weapon_attachment_slots:
      - slot: Narrative.Equipment.Weapon.AttachSlot.Sight
        socket: SightMount
        offset: [0, 0, 5.0]
        rotation: [0, 0, 0]
      - slot: Narrative.Equipment.Weapon.AttachSlot.Barrel
        socket: BarrelMount
        offset: [0, 30.0, 0]
        rotation: [0, 0, 0]
      - slot: Narrative.Equipment.Weapon.AttachSlot.Grip
        socket: GripMount
        offset: [0, 10.0, -3.0]
        rotation: [0, 0, 0]

    # Existing weapon abilities
    weapon_abilities:
      - GA_RifleShot
      - GA_RifleAim
```

### 4.2 Clothing Item with Materials Example

```yaml
equippable_items:
  - name: EI_PlateArmor
    parent_class: EquippableItem
    folder: Items/Armor
    display_name: "Steel Plate Armor"
    description: "Heavy armor providing excellent protection."
    equipment_slot: Narrative.Equipment.Slot.Chest
    armor_rating: 50.0
    stealth_rating: -15.0
    weight: 25.0

    # v3.9.8 Clothing mesh
    clothing_mesh:
      mesh: /Game/Characters/Armor/SK_PlateArmor
      use_leader_pose: true
      is_static_mesh: false
      mesh_attach_socket: Spine2

    # v3.10: Clothing mesh materials with parameters
    clothing_mesh_materials:
      - material: /Game/Materials/M_ArmorBase
        scalar_params:
          - name: Metallic
            value: 0.95
          - name: Roughness
            value: 0.3
          - name: EmissiveIntensity
            value: 0.0
        vector_params:
          - name: BaseColor
            value: [0.4, 0.4, 0.45, 1.0]
          - name: EmissiveColor
            value: [0.0, 0.0, 0.0, 1.0]
      - material: /Game/Materials/M_ArmorLeather
        scalar_params:
          - name: Roughness
            value: 0.7
        vector_params:
          - name: TintColor
            value: [0.3, 0.2, 0.1, 1.0]

    # v3.10: Morph targets for body fit
    clothing_mesh_morphs:
      - chest_expand
      - shoulder_width
      - arm_bulk
```

---

## Part 5: Testing Checklist

After implementation, verify with these test cases:

### 5.1 RangedWeaponItem Properties Test
- [ ] Create EI_ with all new spread multipliers
- [ ] Verify recoil vectors appear in asset details
- [ ] Confirm crosshair widget reference resolves
- [ ] Test aim FOV and F-stop values save correctly

### 5.2 Weapon Attachment Slots Test
- [ ] Create weapon with 3+ attachment slots
- [ ] Verify TMap is populated in asset
- [ ] Confirm tags resolve correctly
- [ ] Check socket/offset/rotation values

### 5.3 Clothing Mesh Materials Test
- [ ] Create clothing item with 2+ materials
- [ ] Verify scalar params save (Metallic, Roughness)
- [ ] Verify vector params save (colors)
- [ ] Test morph target names populate

### 5.4 Regeneration Safety Test
- [ ] Run with `-dryrun` first
- [ ] Verify hash changes trigger MODIFY status
- [ ] Confirm existing assets aren't overwritten without changes

---

## Summary

**Files to Modify:**
1. `Source/GasAbilityGenerator/Public/GasAbilityGeneratorTypes.h` - Add struct fields
2. `Source/GasAbilityGenerator/Private/GasAbilityGeneratorParser.cpp` - Add parsing logic
3. `Source/GasAbilityGenerator/Private/GasAbilityGeneratorGenerators.cpp` - Add property setters

**Expected Automation Level After v3.10:**
- EI_ (Equippable Items): 92% → **98%**
- BPA_ (Activities): 88% → **95%** (if parent class flexibility added)

**Properties Added:**
- 10 RangedWeaponItem float/vector properties
- 1 TSoftClassPtr (CrosshairWidget)
- Full TMap automation for WeaponAttachmentConfiguration
- Full TArray automation for ClothingMesh Materials and Morphs
