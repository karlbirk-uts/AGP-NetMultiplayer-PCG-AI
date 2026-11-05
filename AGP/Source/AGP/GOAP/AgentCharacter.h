// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldState.h"
#include "Goal.h"
// #include "Planner.h"
#include "DrawDebugHelpers.h"
#include "Action.h"
#include "../Characters/EnemyCharacter.h"
#include "Components/WidgetComponent.h"
#include "AgentCharacter.generated.h"

struct FAgentPlannerNode;
/**
 * 
 */
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
	// The real truth table stays here. Referred to before and after TickDoCurrentPlan()
	UPROPERTY()
	FWorldState AgentWorldState; // e.g bHasWeapon, bHasAmmo, bIsWeaponLegendary

	UPROPERTY()
	TArray<TObjectPtr<UGoal>> AgentGoals; // e.g. GetWeaponGoal, KillEnemyGoal

	UPROPERTY()
	TArray<TObjectPtr<UAction>> AgentActions; // e.g. FireAction, ReloadAction, GetWeaponAction


	// Decision-Making Engine
	// UPROPERTY()
	// TObjectPtr<UPlanner> Planner;

	// Agent-copy of current best plan. This can change once per tick, currently.
	UPROPERTY()
	TArray<TObjectPtr<UAction>> AgentCurrentPlan;

	UPROPERTY()
	int32 CurrentPlanIndex = 0;


	// UFUNCTION()
	// void TickDoCurrentPlan();

	UFUNCTION()
	void TickDoCurrentPlan5();

	// Poppable version of TArray used inside TickDoCurrentPlan()
	TQueue<TObjectPtr<UAction>> CurrentPlanQueue;

	void BuildQueueFromPlan();

	UPROPERTY()
	TObjectPtr<UAction> CurrentQueuedAction = nullptr;

	// Testing timer to reset some AgentWworldState values to false
	FTimerHandle VariableTimerHandle;


	// Helper implementation
	bool MoveTowards(const FVector& Location);

	void SortGoalsDescendingPriority();

protected:
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
	// TArray<FPlannerNode> PlannerNodes;
	TArray<FAgentPlannerNode> PlannerNodes;

	// Parsing nodes using specific index
	TArray<int32> OpenSetIndex;
	TArray<int32> ClosedSetIndex;

	// NON-POINTER version of graph building because GC 
	// seems to be hungry.
	int32 AgentAddToPlannerNodes(const FAgentPlannerNode& NextNode);


	// Maze testing
	FVector StartOfMaze;
	FVector EndOfMaze;


	// Maze movement required some NPC movement tweaking
	FVector LastCheckedLocation;
	float StuckTimer = 0.0f;
};


struct FAgentPlannerNode
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
