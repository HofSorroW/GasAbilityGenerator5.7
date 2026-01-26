// GasAbilityGenerator v4.31 - Function Resolution Parity System
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorFunctionResolver.h"

// Core includes
#include "UObject/UObjectGlobals.h"
#include "UObject/Class.h"
#include "AssetRegistry/AssetRegistryModule.h"

// Classes for WellKnownFunctions table
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BlueprintGameplayTagLibrary.h"
#include "NiagaraFunctionLibrary.h"

// AI includes for WellKnownFunctions
#include "AIController.h"

// Blueprint includes for FunctionGraph resolution (v4.31)
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_FunctionEntry.h"

// Narrative Pro includes for WellKnownFunctions
#include "Items/InventoryComponent.h"
#include "Items/WeaponItem.h"
#include "Items/EquippableItem.h"
#include "UnrealFramework/NarrativeCharacter.h"
// Note: ArsenalStatics not included directly due to header dependency chain issues
// GetAttitude resolved via dynamic class lookup below

// Static member definitions
TMap<FString, UClass*> FGasAbilityGeneratorFunctionResolver::WellKnownFunctions;
bool FGasAbilityGeneratorFunctionResolver::bWellKnownFunctionsInitialized = false;

void FGasAbilityGeneratorFunctionResolver::EnsureWellKnownFunctionsInitialized()
{
	if (bWellKnownFunctionsInitialized)
	{
		return;
	}

	// Character functions
	WellKnownFunctions.Add(TEXT("GetCharacterMovement"), ACharacter::StaticClass());
	WellKnownFunctions.Add(TEXT("PlayAnimMontage"), ACharacter::StaticClass());
	WellKnownFunctions.Add(TEXT("StopAnimMontage"), ACharacter::StaticClass());
	WellKnownFunctions.Add(TEXT("GetController"), APawn::StaticClass());
	WellKnownFunctions.Add(TEXT("GetPlayerController"), UGameplayStatics::StaticClass());

	// v6.8: AI Controller functions
	WellKnownFunctions.Add(TEXT("RunBehaviorTree"), AAIController::StaticClass());
	WellKnownFunctions.Add(TEXT("GetBlackboardComponent"), AAIController::StaticClass());

	// Actor functions
	WellKnownFunctions.Add(TEXT("GetActorLocation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("SetActorLocation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("GetActorRotation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("SetActorRotation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("GetActorForwardVector"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_GetActorLocation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_SetActorLocation"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_DestroyActor"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_AttachToComponent"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_DetachFromActor"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("SetActorHiddenInGame"), AActor::StaticClass());
	WellKnownFunctions.Add(TEXT("GetComponentByClass"), AActor::StaticClass());
	// Note: GetActorTransform's C++ name is "GetTransform" with ScriptName="GetActorTransform"
	WellKnownFunctions.Add(TEXT("GetTransform"), AActor::StaticClass());

	// ActorComponent functions
	WellKnownFunctions.Add(TEXT("GetOwner"), UActorComponent::StaticClass());

	// SceneComponent functions
	WellKnownFunctions.Add(TEXT("GetSocketLocation"), USceneComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("GetSocketRotation"), USceneComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_GetComponentLocation"), USceneComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_SetWorldLocation"), USceneComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("SetVisibility"), USceneComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("SetHiddenInGame"), USceneComponent::StaticClass());

	// AbilitySystemComponent functions
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectSpecToSelf"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectSpecToTarget"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToSelf"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToTarget"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveActiveGameplayEffect"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveActiveGameplayEffectBySourceEffect"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("GetActiveGameplayEffectStackCount"), UAbilitySystemComponent::StaticClass());
	// v4.35: Tag manipulation functions on ASC
	WellKnownFunctions.Add(TEXT("AddLooseGameplayTag"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveLooseGameplayTag"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("AddLooseGameplayTags"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveLooseGameplayTags"), UAbilitySystemComponent::StaticClass());

	// AbilitySystemBlueprintLibrary static functions
	WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectSpecToOwner"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("GetAbilitySystemComponent"), UAbilitySystemBlueprintLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("SendGameplayEventToActor"), UAbilitySystemBlueprintLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeSpecHandle"), UAbilitySystemBlueprintLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("AssignTagSetByCallerMagnitude"), UAbilitySystemBlueprintLibrary::StaticClass());
	// v4.36: Static tag manipulation functions (takes ASC as parameter)
	// NOTE: These are different from the instance methods on UAbilitySystemComponent
	// The library versions take ASC as first parameter, making them usable without target_self
	// We add both mappings so the resolver finds the correct class based on manifest context

	// BlueprintGameplayTagLibrary functions
	WellKnownFunctions.Add(TEXT("MakeLiteralGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeGameplayTagContainerFromTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeGameplayTagContainerFromArray"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("AddGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("HasTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("HasAnyTags"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("HasAllTags"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MatchesTag"), UBlueprintGameplayTagLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("GetTagName"), UBlueprintGameplayTagLibrary::StaticClass());
	// v4.32.2: Actor gameplay tag checking
	WellKnownFunctions.Add(TEXT("ActorHasMatchingGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());

	// GameplayAbility functions (core)
	WellKnownFunctions.Add(TEXT("K2_EndAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CommitAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CommitAbilityCooldown"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CommitAbilityCost"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ActivateAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CanActivateAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("GetAvatarActorFromActorInfo"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("GetOwningActorFromActorInfo"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("GetAbilitySystemComponentFromActorInfo"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeOutgoingGameplayEffectSpec"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_MakeOutgoingGameplayEffectSpec"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToOwner"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToTarget"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToSelf"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToOwner"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToTarget"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToSelf"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithAssetTags"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithGrantedTags"), UGameplayAbility::StaticClass());

	// KismetMathLibrary functions (Float)
	WellKnownFunctions.Add(TEXT("Multiply_FloatFloat"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Divide_FloatFloat"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Add_FloatFloat"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Subtract_FloatFloat"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("FClamp"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("RandomFloatInRange"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("GetForwardVector"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeRotator"), UKismetMathLibrary::StaticClass());

	// KismetMathLibrary functions (Double)
	WellKnownFunctions.Add(TEXT("Multiply_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Divide_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Add_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Subtract_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Less_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Greater_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("LessEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("GreaterEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("EqualEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("NotEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());

	// v4.35: KismetMathLibrary conversion functions (UE5 double precision)
	WellKnownFunctions.Add(TEXT("Conv_IntToDouble"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Conv_DoubleToInt64"), UKismetMathLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("Conv_Int64ToDouble"), UKismetMathLibrary::StaticClass());

	// KismetSystemLibrary functions
	WellKnownFunctions.Add(TEXT("Delay"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("PrintString"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("IsValid"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("IsValidClass"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_SetTimer"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ClearTimer"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_SetTimerDelegate"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ClearAndInvalidateTimerHandle"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("SphereOverlapActors"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("LineTraceSingle"), UKismetSystemLibrary::StaticClass());
	// v4.39.3: Literal value functions
	WellKnownFunctions.Add(TEXT("MakeLiteralBool"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralInt"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralFloat"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralDouble"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralString"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralText"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralName"), UKismetSystemLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("MakeLiteralByte"), UKismetSystemLibrary::StaticClass());

	// GameplayStatics functions
	WellKnownFunctions.Add(TEXT("SpawnEmitterAtLocation"), UGameplayStatics::StaticClass());
	WellKnownFunctions.Add(TEXT("SpawnSoundAtLocation"), UGameplayStatics::StaticClass());
	WellKnownFunctions.Add(TEXT("ApplyDamage"), UGameplayStatics::StaticClass());
	WellKnownFunctions.Add(TEXT("GetAllActorsOfClass"), UGameplayStatics::StaticClass());
	WellKnownFunctions.Add(TEXT("GetPlayerCharacter"), UGameplayStatics::StaticClass());
	WellKnownFunctions.Add(TEXT("GetPlayerPawn"), UGameplayStatics::StaticClass());

	// Niagara functions
	WellKnownFunctions.Add(TEXT("SpawnSystemAttached"), UNiagaraFunctionLibrary::StaticClass());
	WellKnownFunctions.Add(TEXT("SpawnSystemAtLocation"), UNiagaraFunctionLibrary::StaticClass());

	// Narrative Pro Inventory functions
	WellKnownFunctions.Add(TEXT("TryAddItemFromClass"), UNarrativeInventoryComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("RemoveItem"), UNarrativeInventoryComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("ConsumeItem"), UNarrativeInventoryComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("FindItemOfClass"), UNarrativeInventoryComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("GetTotalQuantityOfItem"), UNarrativeInventoryComponent::StaticClass());

	// Narrative Pro Weapon functions
	WellKnownFunctions.Add(TEXT("WieldInSlot"), UWeaponItem::StaticClass());
	WellKnownFunctions.Add(TEXT("IsWielded"), UWeaponItem::StaticClass());
	WellKnownFunctions.Add(TEXT("IsHolstered"), UWeaponItem::StaticClass());

	// Narrative Pro Equippable functions
	WellKnownFunctions.Add(TEXT("EquipItem"), UEquippableItem::StaticClass());
	WellKnownFunctions.Add(TEXT("UnequipItem"), UEquippableItem::StaticClass());
	WellKnownFunctions.Add(TEXT("IsEquipped"), UEquippableItem::StaticClass());

	// v4.39.3: Narrative Pro Character functions
	WellKnownFunctions.Add(TEXT("GetHealth"), ANarrativeCharacter::StaticClass());
	WellKnownFunctions.Add(TEXT("GetMaxHealth"), ANarrativeCharacter::StaticClass());

	// v4.39.3: Narrative Pro ArsenalStatics functions (dynamic lookup to avoid header chain issues)
	if (UClass* ArsenalStaticsClass = FindObject<UClass>(nullptr, TEXT("/Script/NarrativeArsenal.ArsenalStatics")))
	{
		WellKnownFunctions.Add(TEXT("GetAttitude"), ArsenalStaticsClass);
	}

	// ========================================================================
	// v4.29: Option 2 - Missing ScriptName entries from Parity Audit
	// These allow manifest to use Blueprint-visible names (ScriptName aliases)
	// ========================================================================

	// UGameplayAbility - 14 missing entries
	WellKnownFunctions.Add(TEXT("K2_CancelAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CheckAbilityCooldown"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CheckAbilityCost"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_CommitExecute"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ShouldAbilityRespondToEvent"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ActivateAbilityFromEvent"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_EndAbilityLocally"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_OnEndAbility"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithHandle"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ExecuteGameplayCue"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_ExecuteGameplayCueWithParams"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_AddGameplayCue"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_AddGameplayCueWithParams"), UGameplayAbility::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_RemoveGameplayCue"), UGameplayAbility::StaticClass());

	// UAbilitySystemComponent - 3 missing entries
	WellKnownFunctions.Add(TEXT("K2_InitStats"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_GiveAbility"), UAbilitySystemComponent::StaticClass());
	WellKnownFunctions.Add(TEXT("K2_GiveAbilityAndActivateOnce"), UAbilitySystemComponent::StaticClass());

	bWellKnownFunctionsInitialized = true;
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveViaWellKnown(const FString& FunctionName)
{
	EnsureWellKnownFunctionsInitialized();

	// Generate variants: Name, K2_Name, BP_Name
	TArray<FString> Variants;
	Variants.Add(FunctionName);
	Variants.Add(TEXT("K2_") + FunctionName);
	Variants.Add(TEXT("BP_") + FunctionName);

	for (const FString& Variant : Variants)
	{
		if (UClass** FoundClass = WellKnownFunctions.Find(Variant))
		{
			UFunction* Function = (*FoundClass)->FindFunctionByName(*Variant);
			if (Function)
			{
				FResolvedFunction Result;
				Result.Function = Function;
				Result.OwnerClass = *FoundClass;
				Result.bFound = true;
				Result.ResolutionPath = FString::Printf(TEXT("WellKnown[%s]"), *Variant);
				return Result;
			}
		}
	}

	return FResolvedFunction();
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveViaExplicitClass(const FString& FunctionName, const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return FResolvedFunction();
	}

	UClass* Class = FindClassByName(ClassName);
	if (!Class)
	{
		return FResolvedFunction();
	}

	// Exact name only - NO variants
	UFunction* Function = Class->FindFunctionByName(*FunctionName);
	if (Function)
	{
		FResolvedFunction Result;
		Result.Function = Function;
		Result.OwnerClass = Class;
		Result.bFound = true;
		Result.ResolutionPath = FString::Printf(TEXT("ExplicitClass[%s::%s]"), *ClassName, *FunctionName);
		return Result;
	}

	return FResolvedFunction();
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveViaParentChain(const FString& FunctionName, UClass* ParentClass)
{
	if (!ParentClass)
	{
		return FResolvedFunction();
	}

	// Walk up the class hierarchy
	UClass* CurrentClass = ParentClass;
	while (CurrentClass)
	{
		// Exact name only - NO variants
		UFunction* Function = CurrentClass->FindFunctionByName(*FunctionName);
		if (Function)
		{
			FResolvedFunction Result;
			Result.Function = Function;
			Result.OwnerClass = CurrentClass;
			Result.bFound = true;
			Result.ResolutionPath = FString::Printf(TEXT("ParentChain[%s::%s]"), *CurrentClass->GetName(), *FunctionName);
			return Result;
		}
		CurrentClass = CurrentClass->GetSuperClass();
	}

	return FResolvedFunction();
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveViaLibraryFallback(const FString& FunctionName)
{
	// 13-class fallback list in exact order from generator
	static const TArray<UClass*> FallbackClasses = {
		UKismetSystemLibrary::StaticClass(),
		UKismetMathLibrary::StaticClass(),
		UGameplayStatics::StaticClass(),
		UGameplayAbility::StaticClass(),
		UAbilitySystemBlueprintLibrary::StaticClass(),
		UAbilitySystemComponent::StaticClass(),
		ACharacter::StaticClass(),
		AActor::StaticClass(),
		APawn::StaticClass(),
		UCharacterMovementComponent::StaticClass(),
		USceneComponent::StaticClass(),
		USkeletalMeshComponent::StaticClass(),
		UPrimitiveComponent::StaticClass()
	};

	for (UClass* Class : FallbackClasses)
	{
		// Exact name only - NO variants
		UFunction* Function = Class->FindFunctionByName(*FunctionName);
		if (Function)
		{
			FResolvedFunction Result;
			Result.Function = Function;
			Result.OwnerClass = Class;
			Result.bFound = true;
			Result.ResolutionPath = FString::Printf(TEXT("LibraryFallback[%s::%s]"), *Class->GetName(), *FunctionName);
			return Result;
		}
	}

	return FResolvedFunction();
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveViaBlueprintFunctionGraph(const FString& FunctionName, UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FunctionResolver] ResolveViaBlueprintFunctionGraph: Blueprint is null for function '%s'"), *FunctionName);
		return FResolvedFunction();
	}

	// v4.31: Debug logging for custom function resolution
	UE_LOG(LogTemp, Warning, TEXT("[FunctionResolver] Searching Blueprint '%s' FunctionGraphs for '%s' (Count: %d)"),
		*Blueprint->GetName(), *FunctionName, Blueprint->FunctionGraphs.Num());
	for (UEdGraph* DbgGraph : Blueprint->FunctionGraphs)
	{
		if (DbgGraph)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FunctionResolver]   - Found FunctionGraph: '%s'"), *DbgGraph->GetFName().ToString());
		}
	}

	// Search all FunctionGraphs in the Blueprint for a matching function name
	// FunctionGraphs are created by custom_functions in the manifest
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		// Check if the graph name matches the function name
		if (Graph->GetFName() == FName(*FunctionName))
		{
			// Found a FunctionGraph with matching name
			// The function should exist on the Blueprint's generated class skeleton
			if (Blueprint->SkeletonGeneratedClass)
			{
				UFunction* Function = Blueprint->SkeletonGeneratedClass->FindFunctionByName(*FunctionName);
				if (Function)
				{
					FResolvedFunction Result;
					Result.Function = Function;
					Result.OwnerClass = Blueprint->SkeletonGeneratedClass;
					Result.bFound = true;
					Result.ResolutionPath = FString::Printf(TEXT("BlueprintFunctionGraph[%s::%s]"), *Blueprint->GetName(), *FunctionName);
					return Result;
				}
			}

			// Fallback: If skeleton doesn't have the function yet (during generation),
			// return a valid result with nullptr Function but bFound = true
			// The generator will create the CallFunction node targeting Self
			FResolvedFunction Result;
			Result.Function = nullptr;  // Will be resolved at compile time
			Result.OwnerClass = Blueprint->SkeletonGeneratedClass;
			Result.bFound = true;
			Result.ResolutionPath = FString::Printf(TEXT("BlueprintFunctionGraph[%s::%s](pending)"), *Blueprint->GetName(), *FunctionName);
			return Result;
		}
	}

	return FResolvedFunction();
}

FResolvedFunction FGasAbilityGeneratorFunctionResolver::ResolveFunction(
	const FString& FunctionName,
	const FString& ExplicitClassName,
	UClass* ParentClass,
	bool bTargetSelf,
	UBlueprint* Blueprint)
{
	if (FunctionName.IsEmpty())
	{
		return FResolvedFunction();
	}

	// Step 1: WellKnownFunctions probe (WITH variants)
	FResolvedFunction Result = ResolveViaWellKnown(FunctionName);
	if (Result.bFound)
	{
		return Result;
	}

	// Step 2: Explicit class (exact name only)
	if (!ExplicitClassName.IsEmpty())
	{
		Result = ResolveViaExplicitClass(FunctionName, ExplicitClassName);
		if (Result.bFound)
		{
			return Result;
		}
	}

	// Step 3: Parent chain (exact name only) - used when target_self or no explicit class
	if (bTargetSelf || ExplicitClassName.IsEmpty())
	{
		Result = ResolveViaParentChain(FunctionName, ParentClass);
		if (Result.bFound)
		{
			return Result;
		}
	}

	// Step 4: Library fallback (exact name only)
	Result = ResolveViaLibraryFallback(FunctionName);
	if (Result.bFound)
	{
		return Result;
	}

	// Step 5: Blueprint FunctionGraph (v4.31) - search custom functions in same Blueprint
	// Only applies during generation when Blueprint is available
	if (Blueprint)
	{
		Result = ResolveViaBlueprintFunctionGraph(FunctionName, Blueprint);
	}

	return Result;
}

bool FGasAbilityGeneratorFunctionResolver::FunctionExists(
	const FString& FunctionName,
	const FString& ExplicitClassName,
	UClass* ParentClass,
	bool bTargetSelf)
{
	FResolvedFunction Result = ResolveFunction(FunctionName, ExplicitClassName, ParentClass, bTargetSelf);
	return Result.bFound;
}

UClass* FGasAbilityGeneratorFunctionResolver::FindClassByName(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return nullptr;
	}

	// Try native class first (strip U/A prefix if present)
	FString NormalizedName = ClassName;
	if (NormalizedName.Len() > 1 && (NormalizedName[0] == 'U' || NormalizedName[0] == 'A'))
	{
		// Check if removing prefix gives valid class
		FString WithoutPrefix = NormalizedName.Mid(1);
		UClass* Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *WithoutPrefix));
		if (Class) return Class;

		Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *WithoutPrefix));
		if (Class) return Class;

		Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *WithoutPrefix));
		if (Class) return Class;

		Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *WithoutPrefix));
		if (Class) return Class;
	}

	// Try with original name
	UClass* Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/AIModule.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Niagara.%s"), *ClassName));
	if (Class) return Class;

	Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/GameplayTags.%s"), *ClassName));
	if (Class) return Class;

	// Try simple name lookup (handles common cases like "Actor", "Character")
	// Note: ANY_PACKAGE was deprecated in UE5, use nullptr instead
	Class = FindObject<UClass>(nullptr, *ClassName);
	if (Class) return Class;

	// Try Asset Registry for Blueprint classes
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("/Game/Blueprints/%s.%s_C"), *ClassName, *ClassName),
		FString::Printf(TEXT("/Game/Characters/%s.%s_C"), *ClassName, *ClassName),
		FString::Printf(TEXT("/Game/Actors/%s.%s_C"), *ClassName, *ClassName),
	};

	for (const FString& Path : SearchPaths)
	{
		FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Path));
		if (AssetData.IsValid())
		{
			Class = LoadObject<UClass>(nullptr, *Path);
			if (Class) return Class;
		}
	}

	return nullptr;
}
