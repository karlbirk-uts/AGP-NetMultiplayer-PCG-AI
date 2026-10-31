// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldState.h"
#include "Goal.h"
#include "Planner.h"
#include "../Characters/EnemyCharacter.h"
#include "AgentCharacter.generated.h"

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
	UPROPERTY()
	TObjectPtr<UPlanner> Planner;

	// Agent-copy of current best plan. This can change once per tick, currently.
	UPROPERTY()
	TArray<TObjectPtr<UAction>> AgentCurrentPlan;

	UPROPERTY()
	int32 CurrentPlanIndex = 0;


	UFUNCTION()
	void TickDoCurrentPlan();

	// Poppable version of TArray used inside TickDoCurrentPlan()
	TQueue<TObjectPtr<UAction>> CurrentPlanQueue;

	void BuildQueueFromPlan();

	UPROPERTY()
	TObjectPtr<UAction> CurrentQueuedAction = nullptr;


	
	
	// Helper implementation
	bool MoveTowards(const FVector& Location);

	void SortGoalsDescendingPriority();
	
protected:
	void TickDoActionImplementation(TObjectPtr<UAction> Action);

	void InitGOAPGoalsActionsWorldStates();
	
};
