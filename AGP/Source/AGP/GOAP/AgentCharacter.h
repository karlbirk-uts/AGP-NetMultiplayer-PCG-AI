/**
 * AAgentCharacter.h
 * 
 * GOAP Agent (NPC)
 * Hosts GOAP AgentWorldState, AgentActions, AgentGoals
 * and a customised Planner methods for PCGMaze
 * 
 * To solve PCGMaze, there are broad goals, with AgentAction 
 * heuristics variables weighing up deltas of distance to maze end, 
 * and each cardinal movement option considers how frequently NPC
 * has visited a path (cell / customised ANavigationNode).
 * 
 * This has considerable PCGMaze-specific edits compared to A3 version.
 * 
 * Author: Karl Birk
 * UTS AGP 2026 Group 10
 */

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#pragma once

#include "CoreMinimal.h"
#include "FWorldState.h"
#include "Goal.h"
// #include "Planner.h"
#include "DrawDebugHelpers.h"
#include "Action.h"
#include "../Characters/EnemyCharacter.h"
#include "AGP/Pathfinding/NavigationNode.h"
#include "Components/WidgetComponent.h"
#include "AgentCharacter.generated.h"


// Copy of A3 Planner.h's FPlannerNode. 
// It didn't end up being customised by A4.
USTRUCT(BlueprintType)
struct FAgentPlannerNode
{
	GENERATED_BODY()

	// Tree info 
	UPROPERTY()
	int32 Index = INDEX_NONE;

	UPROPERTY()
	int32 ParentIndex = INDEX_NONE;

	// One action per node.
	// Action/s is/are owned by UGOAPAgentComponent
	UPROPERTY()
	TObjectPtr<UAction> NodeAction = nullptr;

	// Node-specific info on its cost to tree branch (stack) cost.

	UPROPERTY()
	float GCost = 0.0f; // GCost. From starting node, sum of all costs.

	UPROPERTY()
	float HCost = 0.0f; // Cost to get to goal (not used yet).

	float FCost() const { return GCost + HCost; }; // A* total cost ( GCost + HCost ).

	// Upon tree planning, truth table evolves per node.
	FWorldState NodeWorldState;
};



UCLASS()
class AGP_API AAgentCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AAgentCharacter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	// GOAP agent's truthtable.
	UPROPERTY()
	FWorldState AgentWorldState; // e.g bHasWeapon, bHasAmmo, bIsWeaponLegendary

	// GOAP engine wants to complete a prioritised list of UGoal
	UPROPERTY()
	TArray<TObjectPtr<UGoal>> AgentGoals; // e.g. GetWeaponGoal, KillEnemyGoal

	// GOAP engine will develop plans from this finite list of UAction
	UPROPERTY()
	TArray<TObjectPtr<UAction>> AgentActions; // e.g. FireAction, ReloadAction, GetWeaponAction

	// GOAP agent's knowledge of current best plan for best attainable goal.
	UPROPERTY()
	TArray<TObjectPtr<UAction>> AgentCurrentPlan;

	// Poppable version of AgentCurrentPlan
	TQueue<TObjectPtr<UAction>> CurrentPlanQueue;

	void BuildQueueFromPlan();
	
	UPROPERTY()
	TObjectPtr<UAction> CurrentQueuedAction = nullptr;

	
	// UPROPERTY()
	// int32 CurrentPlanIndex = 0;
	
	// GOAP PLANNER (A4 edit, in house, somewhat bespoke for PCGMaze)
	UFUNCTION()
	void TickDoCurrentPlan5();


	
	// TESTING timeouts to reset some AgentWworldState values
	// FTimerHandle VariableTimerHandle;


	// Unused in final A4 build. 
	// Convenient TickMovement with destination-reached boolean
	// especially if UPathfindingSubsystem isn't a thing.
	bool MoveTowards(const FVector& Location);

	// Goals priority 1.0f is higher than 10.0f
	void SortGoalsAscendingPriority();

protected:
	// PCGMaze
	void TickDoActionImplementation(TObjectPtr<UAction> Action);

	void InitPCGMazeGoals();

	void InitGOAPGoalsActionsWorldStates();

	void AgentTickPatrol(UAction* CurrentAgentAction);
	void AgentMoveAlongPath(UAction* CurrentAgentAction);

public:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GOAP NPC")
	// TObjectPtr<UWidgetComponent> GoalWidgetComponent;
	//
	// // Function to update the text displayed on the widget
	// UFUNCTION(BlueprintImplementableEvent, Category = "GOAP NPC")
	// void UpdateGoalText(const FString& NewGoalText);


	void SetNearestNavNodeIsMud(bool bIsMud);


	// TESTING ALT PLANNER
public:
	// Get A* efficient queue of Actions to reach Goal
	bool AgentGetPlan(
		TArray<TObjectPtr<UAction>> AllNPCActions,
		const FWorldState& NPCState,
		const FWorldState& GoalState,
		TArray<TObjectPtr<UAction>>& OutPlan
	);

private:
	// Local reusable node queue to test all from-Goal-to-Actions-to-NPCState 
	// permutations	that potentially connect NPCState to Goal.
	UPROPERTY()
	TArray<FAgentPlannerNode> PlannerNodes;

	// Parsing nodes using specific index
	UPROPERTY()
	TArray<int32> OpenSetIndex;

	UPROPERTY()
	TArray<int32> ClosedSetIndex;

	// NON-POINTER version of graph building because UE GC
	// seems to be hungry.
	int32 AgentAddToPlannerNodes(const FAgentPlannerNode& NextNode);
	float GetDirectionalMoveCost(const ANavigationNode* TargetNode, float FlatModifier, const FVector& GoalLocation,
	                             const FVector& CurrentLocation);


	// Maze testing
	FVector StartOfMaze;
	FVector EndOfMaze;

	bool bHasReachedEndOfMaze = false;

	// Maze movement required some NPC movement tweaking
	FVector LastCheckedLocation;
	float StuckTimer = 0.0f;


	ANavigationNode* GetNorthCellNavigationNode();
	ANavigationNode* GetSouthCellNavigationNode();
	ANavigationNode* GetEastCellNavigationNode();
	ANavigationNode* GetWestCellNavigationNode();
	void AgentTickMoveToMazeStart(UAction* CurrentAgentAction);
	void AgentTickMoveNorth(UAction* CurrentAgentAction);
	void AgentTickMoveSouth(UAction* CurrentAgentAction);
	void AgentTickMoveEast(UAction* CurrentAgentAction);
	void AgentTickMoveWest(UAction* CurrentAgentAction);
	void RefreshCardinalPathWorldStates();
	void CheckReachedEndOfMaze();
	ANavigationNode* GetNearestNavigationNode();

	
	
	// Quotation mark text comparison errors made me want enum 
	// or const FStrings.
	const FString State_bHasWandered = "bHasWandered";
	
	const FString State_bHasReached_StartOfMaze = "bHasReachedStartOfMaze";
	
	const FString State_bHasReached_EndOfMaze = "bHasReachedEndOfMaze";
	
	// const FString State_bHasMovedVertically = "bHasMovedVertically";
	// const FString State_bHasMovedLaterally = "bHasMovedLaterally";
	const FString State_bHasMoved = "bHasMoved";

	const FString State_bHasPathNorth = "bHasPathNorth";
	const FString State_bHasPathSouth = "bHasPathSouth";
	const FString State_bHasPathEast = "bHasPathEast";
	const FString State_bHasPathWest = "bHasPathWest";

	
	const FString Action_Wander = "Action_Wander";
	const FString Action_MoveTowards_StartOfMaze = "Action_MoveTowards_StartOfMaze";
	const FString Action_MoveTowards_North = "Action_MoveTowards_North";
	const FString Action_MoveTowards_South = "Action_MoveTowards_South";
	const FString Action_MoveTowards_East = "Action_MoveTowards_East";
	const FString Action_MoveTowards_West = "Action_MoveTowards_West";

	
};

