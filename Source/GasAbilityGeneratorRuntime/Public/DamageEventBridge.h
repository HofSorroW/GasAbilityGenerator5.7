// GasAbilityGeneratorRuntime v7.5.3
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// DamageEventBridge - Native bridge for delegate bindings with const-ref parameters
// See: ClaudeContext/Handoffs/Delegate_Binding_Crash_Audit_v7_3.md (Track E)
//
// v7.5.3: Moved to Runtime module to allow runtime Blueprint access
//
// Purpose:
// - Binds to UNarrativeAbilitySystemComponent delegates with exact native signatures
// - Forwards BP-safe FDamageEventSummary payload to Blueprint subscribers
// - Bypasses K2 const-ref limitation without engine patch

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "DamageEventBridge.generated.h"

// Forward declarations
class UNarrativeAbilitySystemComponent;

/**
 * BP-safe payload struct containing all relevant data from FGameplayEffectSpec.
 * Extracted defensively using stable public GAS accessors.
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATORRUNTIME_API FDamageEventSummary
{
	GENERATED_BODY()

	// === TIER 1: Core event data ===

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	UNarrativeAbilitySystemComponent* SourceASC = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	UNarrativeAbilitySystemComponent* TargetASC = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	float Amount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	AActor* InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	AActor* EffectCauserActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	UObject* SourceObject = nullptr;  // const_cast from Context->GetSourceObject()

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	TSubclassOf<UGameplayEffect> EffectClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	FName EffectName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event")
	float EffectLevel = 0.0f;

	// === TIER 2: Tags (from stable public GAS accessors) ===

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Tags")
	FGameplayTagContainer AssetTags;  // Spec.GetAllAssetTags()

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Tags")
	FGameplayTagContainer GrantedTags;  // Spec.GetAllGrantedTags()

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Tags")
	FGameplayTagContainer CapturedSourceTags;  // Spec.CapturedSourceTags.GetAggregatedTags()

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Tags")
	FGameplayTagContainer CapturedTargetTags;  // Spec.CapturedTargetTags.GetAggregatedTags()

	// === TIER 2: Hit context ===

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Hit")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Event|Hit")
	bool bHasHitResult = false;
};

/**
 * Native bridge component for safely forwarding ASC damage/heal events to Blueprint.
 *
 * Binds to UNarrativeAbilitySystemComponent delegates with exact C++ signatures:
 * - OnDamagedBy(ASC*, float, const FGameplayEffectSpec&)
 * - OnDealtDamage(ASC*, float, const FGameplayEffectSpec&)
 * - OnHealedBy(ASC*, float, const FGameplayEffectSpec&)
 *
 * Broadcasts BP-safe FDamageEventSummary to subscribers.
 * Created lazily at runtime via GetOrCreate().
 */
UCLASS(ClassGroup=(Narrative), meta=(BlueprintSpawnableComponent))
class GASABILITYGENERATORRUNTIME_API UDamageEventBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	UDamageEventBridge();

	// BP-safe multicast delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageEventBP, const FDamageEventSummary&, Summary);

	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageEventBP OnDamagedByBP;

	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageEventBP OnDealtDamageBP;

	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageEventBP OnHealedByBP;

	/**
	 * Get or create a DamageEventBridge for the given actor and ASC.
	 * Creates the bridge lazily at runtime if it doesn't exist.
	 *
	 * @param OwnerActor The actor to attach the bridge to
	 * @param OwnerASC The ASC to bind delegates to
	 * @return The bridge component, or nullptr if creation failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Events", meta = (DisplayName = "Get Or Create Damage Event Bridge"))
	static UDamageEventBridge* GetOrCreate(AActor* OwnerActor, UNarrativeAbilitySystemComponent* OwnerASC);

	/**
	 * Get or create a DamageEventBridge from just an ASC.
	 * Resolves the owner actor automatically.
	 *
	 * @param OwnerASC The ASC to bind delegates to
	 * @return The bridge component, or nullptr if creation failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Events", meta = (DisplayName = "Get Or Create Bridge From ASC"))
	static UDamageEventBridge* GetOrCreateFromASC(UNarrativeAbilitySystemComponent* OwnerASC);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	UPROPERTY()
	TObjectPtr<UNarrativeAbilitySystemComponent> OwnerASC = nullptr;

	bool bIsBound = false;

	void BindIfNeeded(UNarrativeAbilitySystemComponent* InASC);
	void UnbindFromASC();

	// Native handlers (exact delegate signatures)
	UFUNCTION()
	void HandleDamagedByNative(UNarrativeAbilitySystemComponent* SourceASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION()
	void HandleDealtDamageNative(UNarrativeAbilitySystemComponent* TargetASC, float Damage, const FGameplayEffectSpec& Spec);

	UFUNCTION()
	void HandleHealedByNative(UNarrativeAbilitySystemComponent* HealerASC, float Amount, const FGameplayEffectSpec& Spec);

	// Helper to build summary from spec
	FDamageEventSummary BuildSummary(UNarrativeAbilitySystemComponent* OtherASC, float Amount, const FGameplayEffectSpec& Spec);
};
