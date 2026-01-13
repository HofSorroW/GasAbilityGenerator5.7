// GasAbilityGenerator v3.9
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.9: Helper classes for NPC Activity Schedule generation

#include "GasScheduledBehaviorHelpers.h"
#include "AI/Activities/NPCGoalItem.h"

UScheduledBehavior_AddNPCGoalByClass::UScheduledBehavior_AddNPCGoalByClass()
{
	GoalClass = nullptr;
}

UNPCGoalItem* UScheduledBehavior_AddNPCGoalByClass::ProvideGoal_Implementation() const
{
	if (GoalClass)
	{
		// Create a new instance of the goal class
		// The outer is set to the transient package since goals are managed by the activity component
		return NewObject<UNPCGoalItem>(GetTransientPackage(), GoalClass);
	}

	return nullptr;
}
