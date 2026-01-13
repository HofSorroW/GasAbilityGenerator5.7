// GasAbilityGenerator v3.9
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.9: Helper classes for NPC Activity Schedule generation

#pragma once

#include "CoreMinimal.h"
#include "AI/Activities/NPCActivitySchedule.h"
#include "GasScheduledBehaviorHelpers.generated.h"

/**
 * Concrete implementation of UScheduledBehavior_AddNPCGoal that allows specifying
 * a goal class directly via property rather than requiring Blueprint override.
 *
 * This enables YAML-driven schedule generation where goals are specified by class reference.
 * At runtime, ProvideGoal() instantiates the specified GoalClass.
 */
UCLASS(Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class GASABILITYGENERATOR_API UScheduledBehavior_AddNPCGoalByClass : public UScheduledBehavior_AddNPCGoal
{
	GENERATED_BODY()

public:

	UScheduledBehavior_AddNPCGoalByClass();

	// The goal class to instantiate when this scheduled behavior starts
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scheduled Behavior - Add NPC Goal")
	TSubclassOf<class UNPCGoalItem> GoalClass;

protected:

	// Override to instantiate GoalClass instead of requiring Blueprint override
	virtual class UNPCGoalItem* ProvideGoal_Implementation() const override;
};
