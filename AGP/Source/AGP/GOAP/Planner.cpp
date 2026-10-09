// Fill out your copyright notice in the Description page of Project Settings.


#include "Planner.h"


// Output a workable plan graph (queue of Actions) for a given Goal.
// Graph will represent NPC-Actions-path-to-Goal.
// OutPlan will be empty (unchanged) if:
// * Goal is unattainable through permutations of AllNPCActions.
//   (which individual Goal is passed into here is determined elsewhere,
//    normally the highest priority Goal checked before others,
//    and it's easy to validate Goal necessity vs NPCState prior).
bool UPlanner::GetPlan(
	TArray<TObjectPtr<UAction>> AllNPCActions,
	const FWorldState& NPCState,
	const FWorldState& GoalState,
	TArray<TObjectPtr<UAction>>& OutPlan)
{
	// Reset indices
	PlannerNodes.Reset();

	// Iterators to update and parse 
	// GCost and FCost (not using HCost yet)
	OpenSetIndex.Reset();
	ClosedSetIndex.Reset();


	// Local-scope node graphs root.
	// Non-pointer version to prevent memory leaks,
	// and it's very small, performing well.
	FPlannerNode RootNode;
	RootNode.Index = 0;
	RootNode.NodeWorldState = NPCState; // The NPC's truth
	RootNode.ParentIndex = INDEX_NONE; // No parent
	RootNode.NodeAction = nullptr; // Checked before final OutPlan build
	RootNode.GCost = 0.0f; // Not used


	// Graph building
	int32 RootIndex = AddToPlannerNodes(RootNode);
	OpenSetIndex.Add(RootIndex);

	bool bIsGoalReachable = false;

	int32 CurrentNodeIndex = INDEX_NONE;

	// Planning Loop (A* of PlannerNodes). Same as AGP Pathfinding. Not using HCost yet.
	// and... Nodes are structs because they're not in Scene/World/Level and
	// I'm hiding from the UE GC Monster.
	while (OpenSetIndex.Num() > 0)
	{
		// Find PlannerNode with lowest FCost
		CurrentNodeIndex = OpenSetIndex[0];
		float LowestFCost = PlannerNodes[CurrentNodeIndex].FCost();

		for (int32 Index : OpenSetIndex)
		{
			float OpenSetFCost = PlannerNodes[Index].FCost();

			bool bCostIsLower = OpenSetFCost < LowestFCost;

			if (bCostIsLower)
			{
				LowestFCost = OpenSetFCost;
				CurrentNodeIndex = Index;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("GetPlan() WHILELOOP LowestFCost: %f"), LowestFCost);


		// Current node's most up to date FWorldState
		// ... and PlannerNode reallocates memory...
		// const FWorldState& CurrentNodeWorldState = PlannerNodes[CurrentNodeIndex].NodeWorldState;
		// Copy the FWorldState for local use.
		const FWorldState CurrentNodeWorldState = PlannerNodes[CurrentNodeIndex].NodeWorldState;
 

		for (const TPair<FString, bool>& StateKVP : CurrentNodeWorldState.StatesMap)
		{
			UE_LOG(LogTemp, Warning, TEXT("GetPlan() WHILELOOP CurrentNodeWorldState.StatesMap Key: %s"), *StateKVP.Key);
			// First loop should match NPCState.
		}


		// If current node satisfies all GoalState KVPs
		// then we can leave the while loop early.
		// Note: A3 version for demo may only have FWorldState of one KVP, per Goal.
		if (CurrentNodeWorldState.HasAll(GoalState))
		{
			bIsGoalReachable = true;

			UE_LOG(LogTemp, Warning, TEXT("GetPlan() WHILELOOP break bIsGoalReachable == true"));

			break;
		}


		// PlannerNodes parsed thus far haven't been able to reach goal
		OpenSetIndex.Remove(CurrentNodeIndex);
		ClosedSetIndex.Add(CurrentNodeIndex);


		// Check AllNPCActions, parsing their WorldState KVPs.
		for (TObjectPtr<UAction> NPCAction : AllNPCActions)
		{
			if (NPCAction)
			{
				// Find next action that seems ready to use
				bool bIsEligible =
					CurrentNodeWorldState.HasAll(NPCAction->ActionPrereqs);

				if (bIsEligible)
				{
					// We're going to append a node to graph
					// and "apply" NPCAction's outcomes to see whether
					// we can find a way to reach our Goal.

					// e.g.
					// DoubleTheActionGoal GoalState "bDidSayGetDownAgain", true
					// DoubleTheActionGoal GoalState "bIsThisSummer", true
					// DoubleTheActionGoal GoalState "bIsLittleTortillaBoy", true

					// BecomeLittleTortillaBoyAction OutcomeState "bIsLittleTortillaBoy", true
					// BecomeLittleTortillaBoyAction PrerequisiteState "bDidGetDownAgain", true

					// GetDownAgainAction OutcomeState "bDidGetDownAgain", true
					// GetDownAgainAction PrerequisiteState "bIsThisSummer", true
					// NarrateThisSummerAction OutcomeState "bIsThisSummer", true
					// NarrateThisSummerAction PrerequisiteState {}


					// Fresh child node to append to graph
					FPlannerNode ChildEligibleNode;

					// Copy FWorldState
					FWorldState WorldState = CurrentNodeWorldState;

					// "Apply" the NPCAction's outcomes KVPs values
					WorldState.StatesMap
					          .Append(NPCAction->ActionOutcomes.StatesMap);
					ChildEligibleNode.NodeWorldState = WorldState;

					// TMap .Append / Add / Emplace can be tested if one doesn't behave as intended. 
					// UE TMap behaviour can differ to other Map collections libraries
					// code behaviours, I'm learning.
					// Can have a FWorldState helper function for TMap operations.
					// Currently, FWorldState structs have a very public TMap
					// due to them being very lightweight for quick copying.

					// Link to parent
					ChildEligibleNode.ParentIndex = CurrentNodeIndex;

					// First run, this'll be 
					ChildEligibleNode.NodeAction = NPCAction;


					// A* costs
					ChildEligibleNode.GCost =
						PlannerNodes[CurrentNodeIndex].GCost
						+
						NPCAction->ActionCost;

					int32 ChildIndex = AddToPlannerNodes(ChildEligibleNode);
					OpenSetIndex.Add(ChildIndex);
				}
			}
		}
	}


	// Reconstruct the Plan backwards ( same as AGP pathfinding subsystem A* )
	bool bIsPlanReady = bIsGoalReachable && CurrentNodeIndex != INDEX_NONE;

	if (bIsPlanReady)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetPlan() bIsPlanReady; Reverse and output"));

		TArray<TObjectPtr<UAction>> ReversePlan;


		// Have index, will parse all connecting PlannerNodes[index]
		while (CurrentNodeIndex != INDEX_NONE)
		{
			FPlannerNode& CurrentNode = PlannerNodes[CurrentNodeIndex];

			// Note: RootNode.NodeAction==nullptr and will not be added to ReversePlan
			if (CurrentNode.NodeAction)
			{
				if (UAction* CurrentNodeAction = CurrentNode.NodeAction)
				{
					ReversePlan.Add(CurrentNodeAction);
				}
			}


			CurrentNodeIndex = CurrentNode.ParentIndex;
		}


		// Current design adds to OutPlan, and OutPlan is Reset() elsewhere before calling GetPlan()
		for (int32 i = ReversePlan.Num() - 1; i >= 0; --i)
		{
			OutPlan.Add(ReversePlan[i]);
		}

		UE_LOG(LogTemp, Warning, TEXT("GetPlan() OutPlan.Num(): %d"), OutPlan.Num());


		return true;
	}

	// Returning a bool here currently, but not used.
	// OutPlan is empty if there is no Plan.
	// Currently, OutPlan (owned by "Agent"/NPC is reset elsewhere)
	// and enters this GetPlan() after a TArray .Reset()

	return false;
}

int32 UPlanner::AddToPlannerNodes(const FPlannerNode& NextNode)
{
	// Research suggests using Emplace on TArray shouldn't break index tracking
	int32 NextIndex = PlannerNodes.Emplace(NextNode);

	PlannerNodes[NextIndex].Index = NextIndex;

	return NextIndex;
}
