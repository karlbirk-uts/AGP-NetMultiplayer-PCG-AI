// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Action.h"
#include "FWorldState.h"
#include "Planner.generated.h"

struct FPlannerNode;

/**
 * Planning engine for GOAP.
 *  
 */
UCLASS()
class AGP_API UPlanner : public UObject
{
	GENERATED_BODY()


public:
	// Get A* efficient queue of Actions to reach Goal
	bool GetPlan(
		TArray<TObjectPtr<UAction>> AllNPCActions,
		const FWorldState& NPCState,
		const FWorldState& GoalState,
		TArray<TObjectPtr<UAction>>& OutPlan
	);

private:

	// Local reusable node queue to test all from-Goal-to-Actions-to-NPCState 
	// permutations	that potentially connect NPCState to Goal.
	TArray<FPlannerNode> PlannerNodes;

	// Parsing nodes using specific index
	TArray<int32> OpenSetIndex;
	TArray<int32> ClosedSetIndex;

	// NON-POINTER version of graph building because GC 
	// seems to be hungry.
	int32 AddToPlannerNodes(const FPlannerNode& NextNode);
};

struct FPlannerNode
{
	// Tree info 
	int32 Index = INDEX_NONE;
	int32 ParentIndex = INDEX_NONE;

	// One action per node.
	// Action/s is/are owned by UGOAPAgentComponent
	// TWeakObjectPtr<UGOAPAction> Action = nullptr;
	TObjectPtr<UAction> NodeAction = nullptr;

	// Node-specific info on its cost to tree branch (stack) cost.
	float GCost = 0.0f; // GCost. From starting node, sum of all costs.
	float HCost = 0.0f; // Cost to get to goal (not used yet).
	float FCost() const { return GCost + HCost; }; // A* total cost ( GCost + HCost ).

	// Upon tree planning, truth table evolves per node.
	FWorldState NodeWorldState;
};
