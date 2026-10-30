// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentCharacter.h"
#include "../Pathfinding/NavigationNode.h"

AAgentCharacter::AAgentCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAgentCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Empty planning collections
	CurrentPlan.Empty();
	CurrentPlanQueue.Empty();

	// ==== VIDEO example 1 ====

	// Fallback Goal and WorldState when no other actions seem feasible
	AgentWorldState.StatesMap.Add(TEXT("bHasWandered"), false);

	TObjectPtr<UGoal> WanderGoal = NewObject<UGoal>(this);
	WanderGoal->GoalWorldState.StatesMap.Add(TEXT("bHasWandered"), true);
	WanderGoal->GoalPriority = 10.0f;
	Goals.Add(WanderGoal);

	TObjectPtr<UAction> WanderAction = NewObject<UAction>(this);
	WanderAction->UniqueName = "WanderAction";
	WanderAction->ActionCost = 0.1;
	WanderAction->ActionOutcomes.StatesMap.Add(TEXT("bHasWandered"), true);
	Actions.Add(WanderAction);


	// Adding examples
	AgentWorldState.StatesMap.Add(TEXT("bHasWalkedToGoldSphere"), false);

	TObjectPtr<UGoal> GetToGoldSphereGoal = NewObject<UGoal>(this);
	GetToGoldSphereGoal->GoalWorldState.StatesMap.Add(TEXT("bHasWalkedToGoldSphere"), true);
	GetToGoldSphereGoal->GoalPriority = 1.0f;
	Goals.Add(GetToGoldSphereGoal);

	TObjectPtr<UAction> GoTowardsGoldSphereAction = NewObject<UAction>(this);
	GoTowardsGoldSphereAction->UniqueName = "GoTowardsGoldSphereAction";
	GoTowardsGoldSphereAction->ActionCost = 0.1;
	GoTowardsGoldSphereAction->ActionOutcomes.StatesMap.Add(TEXT("bHasWalkedToGoldSphere"), true);
	Actions.Add(GoTowardsGoldSphereAction);


	// VIDEO Example
	// AgentWorldState.StatesMap.Add(TEXT("bHasMovedToGold"), false);
	// AgentWorldState.StatesMap.Add(TEXT("bHasMovedToPink"), false);
	// AgentWorldState.StatesMap.Add(TEXT("bHasMovedToPurple"), false);
	// AgentWorldState.StatesMap.Add(TEXT("bHasMovedToThreeColours"), false);


	// VIDEO Example
	// TObjectPtr<UGoal> GetToThreePlanesGoal = NewObject<UGoal>(this);
	// GetToThreePlanesGoal->GoalWorldState.StatesMap.Add(TEXT("bHasMovedToThreeColours"), true);
	// GetToThreePlanesGoal->GoalPriority = 1.0f;
	// Goals.Add(GetToThreePlanesGoal);
	//
	// TObjectPtr<UGoal> GetToGoldPlaneGoal = NewObject<UGoal>(this);
	// GetToGoldPlaneGoal->GoalWorldState.StatesMap.Add(TEXT("bHasMovedToGold"), true);
	// GetToGoldPlaneGoal->GoalPriority = 2.0f;
	// Goals.Add(GetToGoldPlaneGoal);
	//
	// TObjectPtr<UGoal> GetToPinkPlaneGoal = NewObject<UGoal>(this);
	// GetToPinkPlaneGoal->GoalWorldState.StatesMap.Add(TEXT("bHasMovedToPink"), true);
	// GetToPinkPlaneGoal->GoalPriority = 3.0f;
	// Goals.Add(GetToPinkPlaneGoal);
	//
	// TObjectPtr<UGoal> GetToPurplePlaneGoal = NewObject<UGoal>(this);
	// GetToPurplePlaneGoal->GoalWorldState.StatesMap.Add(TEXT("bHasMovedToPurple"), true);
	// GetToPurplePlaneGoal->GoalPriority = 4.0f;
	// Goals.Add(GetToPurplePlaneGoal);


	// VIDEO Example
	// TObjectPtr<UAction> MoveToGoldAction = NewObject<UAction>(this);
	// MoveToGoldAction->UniqueName = "MoveToGoldAction";
	// MoveToGoldAction->ActionCost = FMath::FRandRange(1.0f, 10.0f);
	// MoveToGoldAction->ActionPrereqs.StatesMap.Add(TEXT("bHasMovedToPink"), true);
	// MoveToGoldAction->ActionPrereqs.StatesMap.Add(TEXT("bHasMovedToPurple"), true);
	// MoveToGoldAction->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedToGold"), true);
	// Actions.Add(MoveToGoldAction);
	//
	// TObjectPtr<UAction> MoveToPinkAction = NewObject<UAction>(this);
	// MoveToPinkAction->UniqueName = "MoveToPinkAction";
	// MoveToPinkAction->ActionCost = FMath::FRandRange(1.0f, 10.0f);
	// MoveToPinkAction->ActionPrereqs.StatesMap.Add(TEXT("bHasMovedToPurple"), true);
	// MoveToPinkAction->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedToPink"), true);
	// Actions.Add(MoveToPinkAction);
	//
	// TObjectPtr<UAction> MoveToPurpleAction = NewObject<UAction>(this);
	// MoveToPurpleAction->UniqueName = "MoveToPurpleAction";
	// MoveToPurpleAction->ActionCost = FMath::FRandRange(1.0f, 10.0f);
	// MoveToPurpleAction->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedToPurple"), true);
	// Actions.Add(MoveToPurpleAction);


	// ==== GOAP Planner (using a variant of A* from AGP PathfindingSubsystem) ====
	Planner = NewObject<UPlanner>(this);


	// ==== Sort Goals ====
	// Sort the Goals for quick queue parsing later (Selection Sort)

	// Loop through all Goals, pushing highest priority to front
	for (int32 i = 0; i < Goals.Num() - 1; ++i)
	{
		int32 HighestPriorityIndex = i;

		// Loop through all Goals again
		for (int32 j = i + 1; j < Goals.Num(); ++j)
		{
			// Compare priorities to find the highest
			if (Goals[j]->GoalPriority < Goals[HighestPriorityIndex]->GoalPriority)
			{
				HighestPriorityIndex = j;
			}
		}

		// If this look like a higher priority, swap
		if (HighestPriorityIndex != i)
		{
			TObjectPtr<UGoal> Temp = Goals[i];
			Goals[i] = Goals[HighestPriorityIndex];
			Goals[HighestPriorityIndex] = Temp;
		}
	}


	// TickDoCurrentPlan();
}

void AAgentCharacter::Tick(float DeltaTime)
{
	// Super::Tick(DeltaTime); // FSM from AGP

	UpdateTargetLocation();

	TickDoCurrentPlan();
}


void AAgentCharacter::TickDoCurrentPlan()
{
	// If queued action empty/null
	if (CurrentQueuedAction == nullptr)
	{
		if (!CurrentPlanQueue.IsEmpty())
		{
			// Dequeue next action from plan
			CurrentPlanQueue.Dequeue(CurrentQueuedAction);
		}
		else
		{
			// If queue is empty, next plan
			bool bIsPlanFound = false;

			// Presorted Goals in order of their priorities (1.0f is higher priority than 10.0f)
			for (TObjectPtr<UGoal> Goal : Goals)
			{
				if (Goal)
				{
					Planner->GetPlan(
						Actions,
						AgentWorldState,
						Goal->GoalWorldState,
						CurrentPlan);

					if (CurrentPlan.Num() > 0)
					{
						BuildQueueFromPlan();
						bIsPlanFound = true;

						// Going towards this Goal with CurrentPlan
						break;
					}
				}
			}

			if (bIsPlanFound && !CurrentPlanQueue.IsEmpty())
			{
				CurrentPlanQueue.Dequeue(CurrentQueuedAction);
			}
		}
	}


	// Else, if CurrentQueuedAction looks valid, continue with it
	if (CurrentQueuedAction)
	{
		// When the action finishes its task
		if (CurrentQueuedAction->bIsCompleted)
		{
			// it must set CurrentQueuedAction->bIsCompleted = true;
			for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
			{
				AgentWorldState.StatesMap.Emplace(
					ActionOutcome.Key,
					ActionOutcome.Value);
			}


			// Reset bIsCompleted to false
			CurrentQueuedAction->bIsCompleted = false;
			CurrentQueuedAction = nullptr;
			return;
		}

		TickDoActionImplementation(CurrentQueuedAction);
	}
}

void AAgentCharacter::BuildQueueFromPlan()
{
	CurrentPlanQueue.Empty();

	for (TObjectPtr<UAction> Action : CurrentPlan)
	{
		if (Action)
		{
			CurrentPlanQueue.Enqueue(Action);
		}
	}
}

void AAgentCharacter::TickDoActionImplementation(TObjectPtr<UAction> Action)
{
	// if (Action->ActionOutcomes.HasAll(AgentWorldState))
	// {
	// 	Action->bIsCompleted = true;
	// }


	// Perform implementations of actions from here.
	// e.g. Evade(), Fire(), MoveTowardsGoal()


	if (Action->UniqueName == "WanderAction")
	{
		TickPatrol();
	}
	else if (Action->UniqueName == "GoTowardsGoldSphereAction")
	{
		FVector GoldSphereLocation = FVector(2750, 2490, 360);
		bool bIsGoalMet = MoveTowards(GoldSphereLocation);
		if (bIsGoalMet)
		{
			Action->bIsCompleted = true;
		}
	}
}

bool AAgentCharacter::MoveTowards(const FVector& TargetLocation)
{
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction, 1);

	if (FVector::DistSquared(TargetLocation, GetActorLocation()) <= 3500.0f)
	{
		return true;
	}
	else
	{
		return false;
	}
}
