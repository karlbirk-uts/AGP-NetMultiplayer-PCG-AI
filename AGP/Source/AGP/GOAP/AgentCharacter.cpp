// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AgentCharacter.h"

AAgentCharacter::AAgentCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAgentCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Empty planning collections
	AgentCurrentPlan.Empty();
	CurrentPlanQueue.Empty();


	InitGOAPGoalsActionsWorldStates();
	
	// ==== GOAP Planner (using a variant of A* from AGP PathfindingSubsystem) ====
	Planner = NewObject<UPlanner>(this);


	SortGoalsDescendingPriority();

	// TickDoCurrentPlan();
}

void AAgentCharacter::Tick(float DeltaTime)
{
	// Super::Tick(DeltaTime); // FSM from AGP

	UpdateTargetLocation();

	TickDoCurrentPlan();
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
		// UE_LOG(LogTemp, Warning, TEXT("TickDoImplementation() WanderAction"));
		TickPatrol();
	}
	else if (Action->UniqueName == "GoTowardsGoldSphereAction")
	{
		// UE_LOG(LogTemp, Warning, TEXT("TickDoImplementation() GoTowardsGoldSphereAction"));
		FVector GoldSphereLocation = FVector(2750, 2490, 360);
		bool bIsGoalMet = MoveTowards(GoldSphereLocation);
		if (bIsGoalMet)
		{
			UE_LOG(LogTemp, Warning, TEXT("bIsGoalMet = MoveTowards(GoldSphereLocation)"));

			// This updated Action state should now
			// prompt for AgentWorldState's Goal's review.
			Action->bIsCompleted = true;
		}
	}
}


void AAgentCharacter::TickDoCurrentPlan()
{
	// 1. Handle completion state first
	if (CurrentQueuedAction && CurrentQueuedAction->bIsCompleted)
	{
		for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
		{
			AgentWorldState.StatesMap.Emplace(ActionOutcome.Key, ActionOutcome.Value);
		}

		CurrentQueuedAction->bIsCompleted = false;
		CurrentQueuedAction = nullptr;
		// REMOVED: return; -> Let the frame fall through to fetch the next plan/action immediately!
	}

	// 2. If queued action empty/null, try to fetch or build a new plan
	if (CurrentQueuedAction == nullptr)
	{
		if (!CurrentPlanQueue.IsEmpty())
		{
			CurrentPlanQueue.Dequeue(CurrentQueuedAction);
		}
		else
		{
			bool bIsPlanFound = false;

			for (TObjectPtr<UGoal> Goal : AgentGoals)
			{
				if (Goal)
				{
					// Clear previous plans before passing it into your Planner reference!
					AgentCurrentPlan.Empty();

					Planner->GetPlan(AgentActions, AgentWorldState, Goal->GoalWorldState, AgentCurrentPlan);

					if (AgentCurrentPlan.Num() > 0)
					{
						BuildQueueFromPlan();
						bIsPlanFound = true;
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

	// 3. Execute the current action if valid
	if (CurrentQueuedAction)
	{
		TickDoActionImplementation(CurrentQueuedAction);
	}
}


void AAgentCharacter::BuildQueueFromPlan()
{
	CurrentPlanQueue.Empty();

	for (TObjectPtr<UAction> Action : AgentCurrentPlan)
	{
		if (Action)
		{
			CurrentPlanQueue.Enqueue(Action);
		}
	}
}

bool AAgentCharacter::MoveTowards(const FVector& TargetLocation)
{
	FVector NPCLocation = GetActorLocation();

	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);

	// if (FVector::DistSquared(TargetLocation, GetActorLocation()) <= 3500.0f) // ~ 0.6mm
	if (FVector::DistSquared(TargetLocation, NPCLocation) <= 22500.0f) // 1.5m
	{
		return true;
	}
	else
	{
		return false;
	}
}

void AAgentCharacter::InitGOAPGoalsActionsWorldStates()
{
	AgentWorldState.StatesMap.Add(TEXT("bHasWandered"), false);

	TObjectPtr<UGoal> GoalWander = NewObject<UGoal>(this);
	GoalWander->GoalWorldState.StatesMap.Add(TEXT("bHasWandered"), true);
	GoalWander->GoalPriority = 10.0f;
	AgentGoals.Add(GoalWander);

	TObjectPtr<UAction> ActionWander = NewObject<UAction>(this);
	ActionWander->UniqueName = "ActionWander";
	ActionWander->ActionCost = 0.1;
	ActionWander->ActionOutcomes.StatesMap.Add(TEXT("bHasWandered"), true);
	AgentActions.Add(ActionWander);
	
	
	
	
	// For MultiplayerMap examples (without rebuilding existing terrain)
	// GoldSphere
	AgentWorldState.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), false);

	TObjectPtr<UGoal> GoalBeAtGoldSphere = NewObject<UGoal>(this);
	GoalBeAtGoldSphere->GoalWorldState.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), true);
	GoalBeAtGoldSphere->GoalPriority = 1.0f;
	AgentGoals.Add(GoalBeAtGoldSphere);

	TObjectPtr<UAction> GoTowardsGoldSphereAction = NewObject<UAction>(this);
	GoTowardsGoldSphereAction->UniqueName = "Action_MoveTowardsGoldSphere";
	GoTowardsGoldSphereAction->ActionCost = 0.1;
	GoTowardsGoldSphereAction->ActionOutcomes.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), true);
	AgentActions.Add(GoTowardsGoldSphereAction);

	
	// BlueCylinder
	AgentWorldState.StatesMap.Add(TEXT("bHasWalkedToBlueCylinder"), false);

	TObjectPtr<UGoal> GetToBlueCylinderGoal = NewObject<UGoal>(this);
	GetToBlueCylinderGoal->GoalWorldState.StatesMap.Add(TEXT("bHasWalkedToBlueCylinder"), true);
	GetToBlueCylinderGoal->GoalPriority = 1.0f;
	AgentGoals.Add(GetToBlueCylinderGoal);

	TObjectPtr<UAction> GoTowardsBlueCylinderAction = NewObject<UAction>(this);
	GoTowardsBlueCylinderAction->UniqueName = "GoTowardsBlueCylinderAction";
	GoTowardsBlueCylinderAction->ActionCost = 0.1;
	GoTowardsBlueCylinderAction->ActionOutcomes.StatesMap.Add(TEXT("bHasWalkedToBlueCylinder"), true);
	AgentActions.Add(GoTowardsBlueCylinderAction);

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
}

void AAgentCharacter::SortGoalsDescendingPriority()
{
	// ==== Sort Goals ====
	// Sort the Goals for quick queue parsing later (Selection Sort)

	// Loop through all Goals, pushing highest priority to front
	// NOTE: GoalPriority 1.0f is higher priority than 10.0f
	for (int32 i = 0; i < AgentGoals.Num() - 1; ++i)
	{
		int32 HighestPriorityIndex = i;

		// Loop through all Goals again to find any newer lower number
		for (int32 j = i + 1; j < AgentGoals.Num(); ++j)
		{
			// Compare priorities to find the highest
			if (AgentGoals[j]->GoalPriority < AgentGoals[HighestPriorityIndex]->GoalPriority)
			{
				// The lower number is found (higher priority)
				HighestPriorityIndex = j;
			}
		}

		// If this look like a higher priority, swap
		if (HighestPriorityIndex != i)
		{
			TObjectPtr<UGoal> Temp = AgentGoals[i];
			AgentGoals[i] = AgentGoals[HighestPriorityIndex];
			AgentGoals[HighestPriorityIndex] = Temp;
		}
	}
}

// void AAgentCharacter::TickDoCurrentPlan()
// {
// 	// If queued action empty/null
// 	if (CurrentQueuedAction == nullptr)
// 	{
// 		if (!CurrentPlanQueue.IsEmpty())
// 		{
// 			// Dequeue next action from plan
// 			CurrentPlanQueue.Dequeue(CurrentQueuedAction);
// 		}
// 		else
// 		{
// 			// If queue is empty, next plan
// 			bool bIsPlanFound = false;
//
// 			// Presorted Goals in order of their priorities (1.0f is higher priority than 10.0f)
// 			for (TObjectPtr<UGoal> Goal : Goals)
// 			{
// 				if (Goal)
// 				{
// 					// Have to try for a new plan
// 					CurrentPlan.Empty();
// 					
// 					Planner->GetPlan(
// 						Actions,
// 						AgentWorldState,
// 						Goal->GoalWorldState,
// 						CurrentPlan);
//
// 					if (CurrentPlan.Num() > 0)
// 					{
// 						BuildQueueFromPlan();
// 						bIsPlanFound = true;
//
// 						// Going towards this Goal with CurrentPlan
// 						break;
// 					}
// 				}
// 			}
//
// 			if (bIsPlanFound && !CurrentPlanQueue.IsEmpty())
// 			{
// 				CurrentPlanQueue.Dequeue(CurrentQueuedAction);
// 			}
// 		}
// 	}
//
//
// 	// Else, if CurrentQueuedAction looks valid, continue with it
// 	if (CurrentQueuedAction)
// 	{
// 		// When the action finishes its task
// 		if (CurrentQueuedAction->bIsCompleted)
// 		{
// 			// it must set CurrentQueuedAction->bIsCompleted = true;
// 			for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
// 			{
// 				AgentWorldState.StatesMap.Emplace(
// 					ActionOutcome.Key,
// 					ActionOutcome.Value);
// 			}
//
//
// 			// Reset bIsCompleted to false
// 			CurrentQueuedAction->bIsCompleted = false;
// 			CurrentQueuedAction = nullptr;
// 			return;
// 		}
//
// 		TickDoActionImplementation(CurrentQueuedAction);
// 	}
// }
