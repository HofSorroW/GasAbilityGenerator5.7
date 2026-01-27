// GasAbilityGenerator v7.4
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// DamageEventBridge - Implementation
// See: ClaudeContext/Handoffs/Delegate_Binding_Crash_Audit_v7_3.md (Track E)

#include "DamageEventBridge.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GAS/NarrativeAbilitySystemComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamageEventBridge, Log, All);

UDamageEventBridge::UDamageEventBridge()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
}

UDamageEventBridge* UDamageEventBridge::GetOrCreate(AActor* OwnerActor, UNarrativeAbilitySystemComponent* OwnerASC)
{
	// Guard for non-gameplay contexts (editor, early initialization)
	if (!IsValid(OwnerActor) || !OwnerActor->GetWorld())
	{
		return nullptr;
	}

	// Check if bridge already exists
	UDamageEventBridge* Bridge = OwnerActor->FindComponentByClass<UDamageEventBridge>();
	if (!Bridge)
	{
		// Create at runtime (NOT visible in editor Components panel)
		Bridge = NewObject<UDamageEventBridge>(OwnerActor);

		// HARDENED: Robust UE component creation sequence
		// Order matters: AddInstanceComponent -> OnComponentCreated -> RegisterComponent
		OwnerActor->AddInstanceComponent(Bridge);  // Ensures actor "owns" it in components list
		Bridge->OnComponentCreated();
		Bridge->RegisterComponent();  // Or RegisterComponentWithWorld(OwnerActor->GetWorld())

		// Debug logging for visibility
		UE_LOG(LogDamageEventBridge, Log, TEXT("DamageEventBridge created for %s"), *OwnerActor->GetName());
	}

	// Bind to ASC if provided (AFTER component fully registered)
	if (IsValid(OwnerASC))
	{
		Bridge->BindIfNeeded(OwnerASC);
	}

	return Bridge;
}

UDamageEventBridge* UDamageEventBridge::GetOrCreateFromASC(UNarrativeAbilitySystemComponent* OwnerASC)
{
	if (!IsValid(OwnerASC))
	{
		return nullptr;
	}

	AActor* OwnerActor = OwnerASC->GetOwner();
	return GetOrCreate(OwnerActor, OwnerASC);
}

void UDamageEventBridge::BindIfNeeded(UNarrativeAbilitySystemComponent* InASC)
{
	// RULE: Don't bind until component is fully registered with valid world/owner
	if (bIsBound || !IsValid(InASC) || !IsRegistered())
	{
		return;
	}

	// Set owner reference BEFORE binding
	OwnerASC = InASC;

	// Bind to native delegates with exact signatures
	OwnerASC->OnDamagedBy.AddDynamic(this, &UDamageEventBridge::HandleDamagedByNative);
	OwnerASC->OnDealtDamage.AddDynamic(this, &UDamageEventBridge::HandleDealtDamageNative);
	OwnerASC->OnHealedBy.AddDynamic(this, &UDamageEventBridge::HandleHealedByNative);

	bIsBound = true;

	UE_LOG(LogDamageEventBridge, Log, TEXT("DamageEventBridge bound to ASC on %s"),
		   *GetOwner()->GetName());
}

void UDamageEventBridge::UnbindFromASC()
{
	if (!bIsBound || !IsValid(OwnerASC))
	{
		return;
	}

	// Remove delegate bindings to prevent dangling references
	OwnerASC->OnDamagedBy.RemoveDynamic(this, &UDamageEventBridge::HandleDamagedByNative);
	OwnerASC->OnDealtDamage.RemoveDynamic(this, &UDamageEventBridge::HandleDealtDamageNative);
	OwnerASC->OnHealedBy.RemoveDynamic(this, &UDamageEventBridge::HandleHealedByNative);

	bIsBound = false;
	OwnerASC = nullptr;

	UE_LOG(LogDamageEventBridge, Log, TEXT("DamageEventBridge unbound from ASC"));
}

void UDamageEventBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromASC();
	Super::EndPlay(EndPlayReason);
}

void UDamageEventBridge::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	UnbindFromASC();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

FDamageEventSummary UDamageEventBridge::BuildSummary(UNarrativeAbilitySystemComponent* OtherASC, float Amount, const FGameplayEffectSpec& Spec)
{
	ensure(IsInGameThread());

	FDamageEventSummary Summary;
	Summary.SourceASC = OtherASC;
	Summary.TargetASC = IsValid(OwnerASC) ? OwnerASC.Get() : nullptr;
	Summary.Amount = Amount;
	Summary.EffectLevel = Spec.GetLevel();

	// Context extraction
	if (const FGameplayEffectContext* Context = Spec.GetContext().Get())
	{
		Summary.InstigatorActor = Context->GetOriginalInstigator();
		Summary.EffectCauserActor = Context->GetEffectCauser();

		const UObject* SrcObj = Context->GetSourceObject();
		Summary.SourceObject = SrcObj ? const_cast<UObject*>(SrcObj) : nullptr;

		if (const FHitResult* Hit = Context->GetHitResult())
		{
			Summary.HitResult = *Hit;
			Summary.bHasHitResult = true;
		}
	}

	// Effect identity
	if (Spec.Def)
	{
		Summary.EffectClass = Spec.Def->GetClass();
		Summary.EffectName = Spec.Def->GetFName();
	}

	// Tag extraction (public stable APIs)
	Spec.GetAllAssetTags(Summary.AssetTags);
	Spec.GetAllGrantedTags(Summary.GrantedTags);

	if (const FGameplayTagContainer* Agg = Spec.CapturedSourceTags.GetAggregatedTags())
	{
		Summary.CapturedSourceTags = *Agg;
	}

	if (const FGameplayTagContainer* Agg = Spec.CapturedTargetTags.GetAggregatedTags())
	{
		Summary.CapturedTargetTags = *Agg;
	}

	return Summary;
}

void UDamageEventBridge::HandleDamagedByNative(UNarrativeAbilitySystemComponent* SourceASC, float Damage, const FGameplayEffectSpec& Spec)
{
	FDamageEventSummary Summary = BuildSummary(SourceASC, Damage, Spec);
	// For OnDamagedBy: SourceASC is the damager, TargetASC is us (owner)
	Summary.SourceASC = SourceASC;
	Summary.TargetASC = IsValid(OwnerASC) ? OwnerASC.Get() : nullptr;

	OnDamagedByBP.Broadcast(Summary);
}

void UDamageEventBridge::HandleDealtDamageNative(UNarrativeAbilitySystemComponent* TargetASC, float Damage, const FGameplayEffectSpec& Spec)
{
	FDamageEventSummary Summary = BuildSummary(TargetASC, Damage, Spec);
	// For OnDealtDamage: SourceASC is us (owner), TargetASC is the damaged actor
	Summary.SourceASC = IsValid(OwnerASC) ? OwnerASC.Get() : nullptr;
	Summary.TargetASC = TargetASC;

	OnDealtDamageBP.Broadcast(Summary);
}

void UDamageEventBridge::HandleHealedByNative(UNarrativeAbilitySystemComponent* HealerASC, float Amount, const FGameplayEffectSpec& Spec)
{
	FDamageEventSummary Summary = BuildSummary(HealerASC, Amount, Spec);
	// For OnHealedBy: SourceASC is the healer, TargetASC is us (owner)
	Summary.SourceASC = HealerASC;
	Summary.TargetASC = IsValid(OwnerASC) ? OwnerASC.Get() : nullptr;

	OnHealedByBP.Broadcast(Summary);
}
