// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppUE4CodingStandardNamingViolationWarning
// ReSharper disable CppTooWideScope
#include "AgentCharacter.h"

#include "Action.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "../Pathfinding/NavigationNode.h"
#include "Components/WidgetComponent.h"

// struct FAgentPlannerNode;


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

	// Wanting the start position and end position of the maze for NPC testing
	AActor* FoundActor =
		UGameplayStatics::GetActorOfClass(GetWorld(), AProceduralMazeFacility::StaticClass());
	if (FoundActor)
	{
		AProceduralMazeFacility* ProceduralMazeFacility =
			Cast<AProceduralMazeFacility>(FoundActor);
		if (ProceduralMazeFacility)
		{
			StartOfMaze = ProceduralMazeFacility->GetMazeStartCellWorldPosition();
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter StartOfMaze %s"), *StartOfMaze.ToString());
			EndOfMaze = ProceduralMazeFacility->GetMazeEndCellWorldPosition();
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter EndOfMaze %s"), *EndOfMaze.ToString());

			
		}
	}


	InitGOAPGoalsActionsWorldStates();

	SortGoalsAscendingPriority();

	CurrentNodesPath.Empty();
}

void AAgentCharacter::Tick(float DeltaTime)
{
	if (!bHasReachedEndOfMaze)
	{
		UpdateTargetLocation();

		TickDoCurrentPlan5();


		CheckReachedEndOfMaze();
	}
}


void AAgentCharacter::TickDoActionImplementation(TObjectPtr<UAction> Action)
{
	// Perform implementations of actions from here.
	// e.g. Evade(), Fire(), MoveTowardsGoal()


	if (Action->UniqueName == "Action_Wander")
	{
		AgentTickPatrol(Action);
	}
	else if (Action->UniqueName == "Action_MoveTowards_StartOfMaze")
	{
		AgentTickMoveToMazeStart(Action);
	}
	else if (Action->UniqueName == Action_MoveTowards_North)
	{
		AgentTickMoveNorth(Action);
	}
	else if (Action->UniqueName == Action_MoveTowards_South)
	{
		AgentTickMoveSouth(Action);
	}
	else if (Action->UniqueName == Action_MoveTowards_East)
	{
		AgentTickMoveEast(Action);
	}
	else if (Action->UniqueName == Action_MoveTowards_West)
	{
		AgentTickMoveWest(Action);
	}
}

void AAgentCharacter::InitGOAPGoalsActionsWorldStates()
{
	InitPCGMazeGoals();
}




void AAgentCharacter::TickDoCurrentPlan5()
{
	// ACTION bIsCompleted true - updating AgentWorldState
	if (CurrentQueuedAction && CurrentQueuedAction->bIsCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("TickDoCurrentPlan5 ActionCompleted: %s. Applying outcomes."), *CurrentQueuedAction->UniqueName);

		// Update AgentWorldState
		for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
		{
			UE_LOG(LogTemp, Warning, TEXT("TickDoCurrentPlan5 ActionOutcome.Key: %s"), *ActionOutcome.Key);


			// For goal completion like bHasReachedStartOfMaze
			AgentWorldState.StatesMap.Emplace(ActionOutcome.Key, ActionOutcome.Value);
			// Trying new pseudocode. This now toggles off some worldstates

		}

		CurrentQueuedAction->bIsCompleted = false;
		CurrentQueuedAction = nullptr;

		RefreshCardinalPathWorldStates();

		// Early exit breaks patrol state?
		return;
	}


	// Updating actioncosts for NESW movement
	// ANavigationNode* Current = GetNearestNavigationNode();
	ANavigationNode* North = GetNorthCellNavigationNode();
	ANavigationNode* South = GetSouthCellNavigationNode();
	ANavigationNode* East = GetEastCellNavigationNode();
	ANavigationNode* West = GetWestCellNavigationNode();
	for (UAction* Action : AgentActions)
	{
		if (!Action) continue;
		if (Action->UniqueName == Action_MoveTowards_North)
		{
			Action->ActionCost = 1.0f + (North->NPCVisitedCounter * 2.0f);
		}
		else if (Action->UniqueName == Action_MoveTowards_South)
		{
			Action->ActionCost = 10.0f + (South->NPCVisitedCounter * 2.0f);
		}
		else if (Action->UniqueName == Action_MoveTowards_East)
		{
			Action->ActionCost = 3.0f + (East->NPCVisitedCounter * 5.0f);
		}
		else if (Action->UniqueName == Action_MoveTowards_West)
		{
			Action->ActionCost = 3.0f + (West->NPCVisitedCounter * 5.0f);
		}
	}


	// If queued action empty/null, try to fetch or build a new plan
	if (CurrentQueuedAction == nullptr)
	{
		RefreshCardinalPathWorldStates();

		if (!CurrentPlanQueue.IsEmpty())
		{
			CurrentPlanQueue.Dequeue(CurrentQueuedAction);
		}
		else
		{
			bool bIsPlanFound = false;

			for (TObjectPtr<UGoal> Goal : AgentGoals)
			{
				if (!Goal) continue;

				// If the goal state is already entirely met, skip planning for it
				if (AgentWorldState.HasAll(Goal->GoalWorldState))
				{
					continue;
				}

				// Clean the plan container before rewriting
				AgentCurrentPlan.Empty();

				AgentGetPlan(AgentActions, AgentWorldState, Goal->GoalWorldState, AgentCurrentPlan);

				if (AgentCurrentPlan.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("NPC successfully formed a NEW PLAN for goal!"));

					if (Goal->GoalWorldState.StatesMap.Num() > 0)
					{
						for (const TPair<FString, bool>& Pair : Goal->GoalWorldState.StatesMap)
						{
							UE_LOG(LogTemp, Warning, TEXT("NEW PLAN looking to meet Goal key: %s"), *Pair.Key);
						}
					}
					
					for (const TPair<FString, bool>& Pair : AgentWorldState.StatesMap)
					{
						UE_LOG(LogTemp, Warning, TEXT("NEW PLAN starting with AgentWorldState: %s, %s"), *Pair.Key, Pair.Value ? TEXT("true") : TEXT("false"));
					}


					for (const UAction* Action : AgentCurrentPlan)
					{
						UE_LOG(LogTemp, Warning, TEXT("NEW PLAN -> Plan Step: %s"), *Action->UniqueName);
					}

					BuildQueueFromPlan();
					bIsPlanFound = true;
					break; // Found a valid plan, stop looking at lower priority goals
				}
			}

			if (bIsPlanFound && !CurrentPlanQueue.IsEmpty())
			{
				CurrentPlanQueue.Dequeue(CurrentQueuedAction);
			}
			else
			{
				// If no plan was found for ANY goal, fallback to Action_Wander
				// if (CurrentQueuedAction && CurrentQueuedAction->UniqueName == Action_Wander)
				// {
				// 	// The NPC is already wandering! Letting it keep its path.
				// 	UE_LOG(LogTemp, Warning, TEXT("PLANNER: Wandering."));
				// }
				// else
				// {
				// 	UE_LOG(LogTemp, Warning, TEXT("PLANNER: viable GOAP plan found. Wandering."));
				// 	for (TObjectPtr<UAction> Action : AgentActions)
				// 	{
				// 		if (Action && Action->UniqueName == Action_Wander)
				// 		{
				// 			UE_LOG(LogTemp, Warning, TEXT("NPC DEFAULT WANDER BEHAVIOUR - BUHAO"));
				//
				// 			// THIS hack IGNORES whether this action's outcome had updated agentworldstate.
				// 			CurrentQueuedAction = Action;
				// 			CurrentQueuedAction->bIsCompleted = false;
				// 			AgentWorldState.StatesMap.Emplace(State_bHasWandered, false);
				// 			break;
				// 		}
				// 	}
				// }
			}
		}
	}

	// Execute the current action if valid
	if (CurrentQueuedAction)
	{
		TickDoActionImplementation(CurrentQueuedAction);
	}
}


bool AAgentCharacter::AgentGetPlan(
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
	FAgentPlannerNode RootNode;
	RootNode.Index = 0;
	RootNode.NodeWorldState = NPCState; // The NPC's truth
	RootNode.ParentIndex = INDEX_NONE; // No parent
	RootNode.NodeAction = nullptr; // Checked before final OutPlan build
	RootNode.GCost = 0.0f; // Not used


	// Graph building
	int32 RootIndex = AgentAddToPlannerNodes(RootNode);
	OpenSetIndex.Add(RootIndex);

	bool bIsGoalReachable = false;

	int32 CurrentNodeIndex = INDEX_NONE;

	// Planning Loop (A* of PlannerNodes). Same as AGP Pathfinding. Not using HCost yet.
	// and... Nodes are structs because they're not in Scene/World/Level and
	// I'm hiding from the UE GC Monster.

	int32 IterationsCounter = 0;
	const int32 MaxPlannerIterations = 1000; // Preventing infinite loops

	while (OpenSetIndex.Num() > 0)
	{
		if (++IterationsCounter > MaxPlannerIterations)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentGetPlan: MaxPlannerIterations"));
			return false;
		}


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

		// Current node's most up to date FWorldState, copied
		const FWorldState CurrentNodeWorldState = PlannerNodes[CurrentNodeIndex].NodeWorldState;

		// If current node satisfies all GoalState KVPs
		// then we can leave the while loop early.
		if (CurrentNodeWorldState.HasAll(GoalState))
		{
			bIsGoalReachable = true;
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
				// One action per plan (graph branch)
				bool bActionAlreadyUsed = false;
				int32 CheckParentIndex = CurrentNodeIndex;

				while (CheckParentIndex != INDEX_NONE)
				{
					if (PlannerNodes[CheckParentIndex].NodeAction == NPCAction)
					{
						bActionAlreadyUsed = true;
						break;
					}
					CheckParentIndex = PlannerNodes[CheckParentIndex].ParentIndex;
				}

				if (bActionAlreadyUsed)
				{
					continue; // Skip this action in this graph
				}


				// Find next action that seems ready to use
				bool bIsEligible =
					CurrentNodeWorldState.HasAll(NPCAction->ActionPrereqs);

				if (bIsEligible)
				{
					// We're going to append a node to graph
					// and "apply" NPCAction's outcomes to see whether
					// we can find a way to reach our Goal.

					// Fresh child node to append to graph
					FAgentPlannerNode ChildEligibleNode;

					// Copy FWorldState
					FWorldState WorldState = CurrentNodeWorldState;

					// "Apply" the NPCAction's outcomes KVPs values
					WorldState.StatesMap
					          .Append(NPCAction->ActionOutcomes.StatesMap);
					ChildEligibleNode.NodeWorldState = WorldState;

					// Link to parent
					ChildEligibleNode.ParentIndex = CurrentNodeIndex;

					// First run, this'll be 
					ChildEligibleNode.NodeAction = NPCAction;


					// A* costs
					ChildEligibleNode.GCost =
						PlannerNodes[CurrentNodeIndex].GCost
						+
						NPCAction->ActionCost;

					int32 ChildIndex = AgentAddToPlannerNodes(ChildEligibleNode);
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
			FAgentPlannerNode& CurrentNode = PlannerNodes[CurrentNodeIndex];

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

	return false;
}




void AAgentCharacter::AgentTickPatrol(UAction* CurrentAgentAction)
{
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickPatrol() CurrentNodesPath.IsEmpty() == true"));

			CurrentNodesPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());

			UE_LOG(LogTemp, Warning, TEXT("AgentTickPatrol() GetRandomPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING
				CurrentAgentAction->bIsCompleted = true;
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter Loc: %s to PathNode: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}

			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			// CHECK THIS
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}


			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
	}

	AgentMoveAlongPath(CurrentAgentAction);
}



void AAgentCharacter::InitPCGMazeGoals()
{
	// Agent WorldStates
	AgentWorldState.StatesMap.Add(State_bHasWandered, false);
	AgentWorldState.StatesMap.Add(State_bHasReached_StartOfMaze, false);
	AgentWorldState.StatesMap.Add(State_bHasReached_EndOfMaze, false);
	AgentWorldState.StatesMap.Add(State_bHasMovedVertically, false);
	AgentWorldState.StatesMap.Add(State_bHasMovedLaterally, false);
	AgentWorldState.StatesMap.Add(State_bHasPathNorth, false);
	AgentWorldState.StatesMap.Add(State_bHasPathSouth, false);
	AgentWorldState.StatesMap.Add(State_bHasPathEast, false);
	AgentWorldState.StatesMap.Add(State_bHasPathWest, false);
	AgentWorldState.StatesMap.Add(TEXT("bHasRepeatedZone"), false);


	// Goals
	TObjectPtr<UGoal> GoalWander = NewObject<UGoal>(this);
	GoalWander->GoalWorldState.StatesMap.Add(State_bHasWandered, true);
	GoalWander->GoalPriority = 99.0f;
	AgentGoals.Add(GoalWander);

	TObjectPtr<UGoal> GoalBeAtStartOfMaze = NewObject<UGoal>(this);
	GoalBeAtStartOfMaze->GoalWorldState.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	GoalBeAtStartOfMaze->GoalPriority = 1.0f;
	AgentGoals.Add(GoalBeAtStartOfMaze);

	TObjectPtr<UGoal> GoalHasArrivedAtExit = NewObject<UGoal>(this);
	GoalHasArrivedAtExit->GoalWorldState.StatesMap.Add(State_bHasReached_EndOfMaze, true);
	GoalHasArrivedAtExit->GoalPriority = 2.0f;
	AgentGoals.Add(GoalHasArrivedAtExit);

	TObjectPtr<UGoal> GoalHasMovedVertically = NewObject<UGoal>(this);
	GoalHasMovedVertically->GoalWorldState.StatesMap.Add(State_bHasMovedVertically, true);
	GoalHasMovedVertically->GoalPriority = 3.0f;
	AgentGoals.Add(GoalHasMovedVertically);


	// Actions - used by planner, implementation elsewhere, e.g. Patrol()
	TObjectPtr<UAction> ActionWander = NewObject<UAction>(this);
	ActionWander->UniqueName = Action_Wander;
	ActionWander->ActionCost = 10.0f;
	ActionWander->ActionOutcomes.StatesMap.Add(State_bHasWandered, true);
	ActionWander->ActionPrereqs.StatesMap.Empty();
	AgentActions.Add(ActionWander);

	TObjectPtr<UAction> ActionMoveTowardsStartOfMaze = NewObject<UAction>(this);
	ActionMoveTowardsStartOfMaze->UniqueName = Action_MoveTowards_StartOfMaze;
	ActionMoveTowardsStartOfMaze->ActionCost = 0.2;
	ActionMoveTowardsStartOfMaze->ActionOutcomes.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsStartOfMaze->ActionPrereqs.StatesMap.Empty();
	AgentActions.Add(ActionMoveTowardsStartOfMaze);


	TObjectPtr<UAction> ActionMoveTowardsNorth = NewObject<UAction>(this);
	ActionMoveTowardsNorth->UniqueName = Action_MoveTowards_North;
	ActionMoveTowardsNorth->ActionCost = 1.0;
	ActionMoveTowardsNorth->ActionOutcomes.StatesMap.Add(State_bHasMovedVertically, true);
	ActionMoveTowardsNorth->ActionOutcomes.StatesMap.Add(State_bHasMovedLaterally, false);

	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(State_bHasPathNorth, true);

	AgentActions.Add(ActionMoveTowardsNorth);


	TObjectPtr<UAction> ActionMoveTowardsSouth = NewObject<UAction>(this);
	ActionMoveTowardsSouth->UniqueName = Action_MoveTowards_South;
	ActionMoveTowardsSouth->ActionCost = 10.0;
	ActionMoveTowardsSouth->ActionOutcomes.StatesMap.Add(State_bHasMovedVertically, true);
	ActionMoveTowardsSouth->ActionOutcomes.StatesMap.Add(State_bHasMovedLaterally, false);

	ActionMoveTowardsSouth->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsSouth->ActionPrereqs.StatesMap.Add(State_bHasPathSouth, true);
	AgentActions.Add(ActionMoveTowardsSouth);


	TObjectPtr<UAction> ActionMoveTowardsEast = NewObject<UAction>(this);
	ActionMoveTowardsEast->UniqueName = Action_MoveTowards_East;
	ActionMoveTowardsEast->ActionCost = 3.0;
	ActionMoveTowardsEast->ActionOutcomes.StatesMap.Add(State_bHasMovedVertically, false);
	ActionMoveTowardsEast->ActionOutcomes.StatesMap.Add(State_bHasMovedLaterally, true);

	ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(State_bHasPathEast, true);
	// ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(State_bHasMovedLaterally, false);
	AgentActions.Add(ActionMoveTowardsEast);


	TObjectPtr<UAction> ActionMoveTowardsWest = NewObject<UAction>(this);
	ActionMoveTowardsWest->UniqueName = Action_MoveTowards_West;
	ActionMoveTowardsWest->ActionCost = 3.0;
	ActionMoveTowardsWest->ActionOutcomes.StatesMap.Add(State_bHasMovedVertically, false);
	ActionMoveTowardsWest->ActionOutcomes.StatesMap.Add(State_bHasMovedLaterally, true);

	ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(State_bHasPathWest, true);
	// ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(State_bHasMovedLaterally, false);
	AgentActions.Add(ActionMoveTowardsWest);
	
}






// Updates Agent's wordstate of cardinal direction pathing available
void AAgentCharacter::RefreshCardinalPathWorldStates()
{
	AgentWorldState.StatesMap.Emplace(TEXT("bHasPathNorth"), false);
	AgentWorldState.StatesMap.Emplace(TEXT("bHasPathSouth"), false);
	AgentWorldState.StatesMap.Emplace(TEXT("bHasPathEast"), false);
	AgentWorldState.StatesMap.Emplace(TEXT("bHasPathWest"), false);


	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			float CellDistanceThreshold = 150.0f; // Half of the 300 units square cells in PCGMaze

			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				// UE_LOG(LogTemp, Warning,
				//        TEXT("RefreshCardinalPathWorldStates NearestNode->GetConnectedNodes().Num(): %d"),
				//        NearestNode->GetConnectedNodes().Num());


				for (int32 i = 0; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					ANavigationNode* NeighbourNode = NearestNode->GetConnectedNodes()[i];

					float DistanceX =
						NeighbourNode->LocationComponent->GetComponentLocation().X -
						NearestNode->LocationComponent->GetComponentLocation().X;
					// UE_LOG(LogTemp, Warning,
					//        TEXT("RefreshCardinalPathWorldStates DistanceX: %f"),
					//        DistanceX);

					float DistanceY =
						NeighbourNode->LocationComponent->GetComponentLocation().Y -
						NearestNode->LocationComponent->GetComponentLocation().Y;
					// UE_LOG(LogTemp, Warning,
					//        TEXT("RefreshCardinalPathWorldStates DistanceY: %f"),
					//        DistanceY);

					// UE_LOG(LogTemp, Warning, TEXT("Nearest [%s] Location: %s"),
					//        *NearestNode->GetName(), *NearestNode->LocationComponent->GetComponentLocation().ToString());
					//
					// UE_LOG(LogTemp, Warning, TEXT("Neighbour [%s] Location: %s"),
					//        *NeighbourNode->GetName(),
					//        *NeighbourNode->LocationComponent->GetComponentLocation().ToString());

					bool bHasPathNorth = DistanceX > CellDistanceThreshold;
					bool bHasPathSouth = -DistanceX > CellDistanceThreshold;
					bool bHasPathEast = DistanceY > CellDistanceThreshold;
					bool bHasPathWest = -DistanceY > CellDistanceThreshold;


					// NORTH
					if (bHasPathNorth)
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathNorth"));

						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathNorth"), true);
					}
					else
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathNorth == false"));
						// AgentWorldState.StatesMap.Emplace(TEXT("bHasPathNorth"), false);
					}

					// SOUTH
					if (bHasPathSouth)
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathSouth"));
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathSouth"), true);
					}
					else
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathSouth == false"));
						//
						// AgentWorldState.StatesMap.Emplace(TEXT("bHasPathSouth"), false);
					}

					// EAST
					if (bHasPathEast)
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathEast"));
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathEast"), true);
					}
					else
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathEast == false"));
						// AgentWorldState.StatesMap.Emplace(TEXT("bHasPathEast"), false);
					}

					// WEST
					if (bHasPathWest)
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathWest"));
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathWest"), true);
					}
					else
					{
						// UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathWest == false"));
						// AgentWorldState.StatesMap.Emplace(TEXT("bHasPathWest"), false);
					}

					if (!(bHasPathNorth || bHasPathSouth || bHasPathEast || bHasPathWest))
					{
						UE_LOG(LogTemp, Warning, TEXT("RefreshCardinalPathWorldStates bHasPathNESW == false"));
						UE_LOG(LogTemp, Warning,
						       TEXT("RefreshCardinalPathWorldStates NearestNode->GetConnectedNodes().Num(): %d"),
						       NearestNode->GetConnectedNodes().Num());
					}
				}
			}
			else
			{
				// NearestNode->GetConnectedNodes().Num() > 0
				UE_LOG(LogTemp, Warning,
				       TEXT("NPC RefreshCardinalPathWorldStates NearestNode->GetConnectedNodes().Num() <= 0"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC RefreshCardinalPathWorldStates !NearestNode"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC RefreshCardinalPathWorldStates !PathfindingSubsystem"));
	}
}



void AAgentCharacter::AgentTickMoveToMazeStart(UAction* CurrentAgentAction)
{
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("AAgentCharacter::AgentTickMoveToMazeStart() CurrentNodesPath.IsEmpty() == true"));

			// CurrentNodesPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
			// CurrentNodesPath = PathfindingSubsystem->
			// 	GetPath(
			// 		GetActorLocation(),
			// 		StartOfMaze);
			CurrentNodesPath = PathfindingSubsystem->
				MazeGetPath(
					GetActorLocation(),
					StartOfMaze);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveToMazeStart() MazeGetPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING
				// RefreshCardinalPathWorldStates();

				CurrentAgentAction->bIsCompleted = true;
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning,
				       TEXT("AAgentCharacter AgentTickMoveToMazeStart NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}


			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}

			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			// UE_LOG(LogTemp, Warning, TEXT("CurrentNodesPath generated."));
			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
		else
		{
			// CurrentNodesPath bountiful
		}


		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveToMazeStart PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}


void AAgentCharacter::AgentTickMoveNorth(UAction* CurrentAgentAction)
{
	FVector DestinationLocation = GetNorthCellNavigationNode()->LocationComponent->GetComponentLocation();
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveNorth() CurrentNodesPath.IsEmpty() == true"));

			// CurrentNodesPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
			// CurrentNodesPath = PathfindingSubsystem->
			// 	GetPath(
			// 		GetActorLocation(),
			// 		DestinationLocation);
			CurrentNodesPath = PathfindingSubsystem->
				MazeGetPath(
					GetActorLocation(),
					DestinationLocation);

			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveNorth() MazeGetPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING
				UE_LOG(LogTemp, Warning,
				       TEXT("AAgentCharacter::AgentTickMoveNorth THIS SPACE IS NEVER CALLED IF NPC BEHAVING"));

				// RefreshCardinalPathWorldStates();

				CurrentAgentAction->bIsCompleted = true;
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveNorth NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}


			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}

			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			// UE_LOG(LogTemp, Warning, TEXT("CurrentNodesPath generated."));
			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
		else
		{
			// CurrentNodesPath bountiful
		}


		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveNorth PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}


void AAgentCharacter::AgentTickMoveSouth(UAction* CurrentAgentAction)
{
	FVector DestinationLocation = GetSouthCellNavigationNode()->LocationComponent->GetComponentLocation();
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveSouth() CurrentNodesPath.IsEmpty() == true"));

			CurrentNodesPath = PathfindingSubsystem->
				MazeGetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveSouth() MazeGetPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING

				// RefreshCardinalPathWorldStates();

				CurrentAgentAction->bIsCompleted = true;

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveSouth NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}


			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}

			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			// UE_LOG(LogTemp, Warning, TEXT("CurrentNodesPath generated."));
			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
		else
		{
			// CurrentNodesPath bountiful
		}


		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveSouth PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}

void AAgentCharacter::AgentTickMoveEast(UAction* CurrentAgentAction)
{
	FVector DestinationLocation = GetEastCellNavigationNode()->LocationComponent->GetComponentLocation();
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveEast() CurrentNodesPath.IsEmpty() == true"));

			CurrentNodesPath = PathfindingSubsystem->
				MazeGetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveEast() MazeGetPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING

				// RefreshCardinalPathWorldStates();

				CurrentAgentAction->bIsCompleted = true;

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveEast NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}


			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}

			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			// UE_LOG(LogTemp, Warning, TEXT("CurrentNodesPath generated."));
			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
		else
		{
			// CurrentNodesPath bountiful
		}


		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveEast PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}

void AAgentCharacter::AgentTickMoveWest(UAction* CurrentAgentAction)
{
	FVector DestinationLocation = GetWestCellNavigationNode()->LocationComponent->GetComponentLocation();
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveWest() CurrentNodesPath.IsEmpty() == true"));

			CurrentNodesPath = PathfindingSubsystem->
				MazeGetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveWest() MazeGetPath(GetActorLocation())"));

			if (CurrentNodesPath.IsEmpty())
			{
				// THIS SPACE IS NEVER CALLED IF NPC BEHAVING

				// RefreshCardinalPathWorldStates();

				CurrentAgentAction->bIsCompleted = true;

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveWest NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
			}


			// Freezing if path num == 2
			// Removing the starting node to try 
			// the neighbournode first
			if (CurrentNodesPath.Num() > 1)
			{
				CurrentNodesPath.RemoveAt(0);
			}

			StuckTimer = 0.0f;
			LastCheckedLocation = GetActorLocation();


			// UE_LOG(LogTemp, Warning, TEXT("CurrentNodesPath generated."));
			UE_LOG(LogTemp, Warning, TEXT("Generated CurrentNodesPath; CurrentNodesPath[0]: %s"),
			       *CurrentNodesPath[0].ToString());
		}
		else
		{
			// CurrentNodesPath bountiful
		}


		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveWest PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}

void AAgentCharacter::CheckReachedEndOfMaze()
{
	FVector NPCLocation = GetActorLocation();


	// Walking through PCGMaze likes some tighter waypoints adherence
	if (FVector::DistSquared(EndOfMaze, NPCLocation) <= 22'500.0f) // 1.5m
	{
		bHasReachedEndOfMaze = true;
		UE_LOG(LogTemp, Warning, TEXT("NPC REACHED END OF MAZE"));
	}
}


ANavigationNode* AAgentCharacter::GetNearestNavigationNode()
{
	ANavigationNode* NearestNavigationNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			NearestNavigationNode = NearestNode;
		}
	}
	return NearestNavigationNode;
}

void AAgentCharacter::SetNearestNavNodeIsMud(bool bIsMud)
{
	if (PathfindingSubsystem)
	{
		ANavigationNode* NavigationNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());

		if (NavigationNode)
		{
			NavigationNode->bIsMud = bIsMud;
		}
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

bool AAgentCharacter::MoveTowards(const FVector& DesiredLocation)
{
	FVector NPCLocation = GetActorLocation();

	FVector Direction = DesiredLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);

	// Walking through PCGMaze likes some tighter waypoints adherence
	if (FVector::DistSquared(DesiredLocation, NPCLocation) <= 22'500.0f) // 1.5m
	{
		return true;
	}
	else
	{
		return false;
	}
}


void AAgentCharacter::SortGoalsAscendingPriority()
{
	// Sort the Goals for quick queue parsing later (Selection Sort)
	if (AgentGoals.Num() <= 1) return;

	// Loop through all Goals, pushing highest priority to front
	// NOTE: GoalPriority 1.0f is higher priority than 10.0f
	for (int32 i = 0; i < AgentGoals.Num() - 1; ++i)
	{
		int32 HighestPriorityIndex = i;

		// Loop through all Goals again to find any newer lower number
		for (int32 j = i + 1; j < AgentGoals.Num(); ++j)
		{
			// Compare priorities to find the highest priority
			if (AgentGoals[i] &&
				AgentGoals[j] &&
				AgentGoals[j]->GoalPriority < AgentGoals[HighestPriorityIndex]->GoalPriority)
			{
				// The lower number is found (higher priority)
				HighestPriorityIndex = j;
			}
		}

		// If this look like a higher priority, swap
		if (HighestPriorityIndex != i)
		{
			AgentGoals.Swap(i, HighestPriorityIndex);
		}
	}
}

ANavigationNode* AAgentCharacter::GetNorthCellNavigationNode()
{
	ANavigationNode* NorthernMostNeighbourNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			// TEMP
			const TArray<ANavigationNode*>& ConnectedNodes =
				NearestNode->GetConnectedNodes();

			int32 StartIndex = -1;
			for (int32 i = 0; i < ConnectedNodes.Num(); ++i)
			{
				if (ConnectedNodes[i] && ConnectedNodes[i]->LocationComponent)
				{
					NorthernMostNeighbourNode = ConnectedNodes[i];
					StartIndex = i + 1;
					break;
				}
			}

			// If a valid baseline node was found, compare it against the rest
			if (NorthernMostNeighbourNode && StartIndex > 0)
			{
				for (int32 i = StartIndex; i < ConnectedNodes.Num(); ++i)
				{
					if (!ConnectedNodes[i] || !ConnectedNodes[i]->LocationComponent) continue;

					if (ConnectedNodes[i]->
					    LocationComponent->
					    GetComponentLocation().X
						>
						NorthernMostNeighbourNode->
						LocationComponent->
						GetComponentLocation().X)
					{
						NorthernMostNeighbourNode = ConnectedNodes[i];
					}
				}
			}
		}
	}

	return NorthernMostNeighbourNode;
}

ANavigationNode* AAgentCharacter::GetSouthCellNavigationNode()
{
	ANavigationNode* SouthernMostNeighbourNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			const TArray<ANavigationNode*>& ConnectedNodes =
				NearestNode->GetConnectedNodes();
			
			int32 StartIndex = -1;
			for (int32 i = 0; i < ConnectedNodes.Num(); ++i)
			{
				if (ConnectedNodes[i] && ConnectedNodes[i]->LocationComponent)
				{
					SouthernMostNeighbourNode = ConnectedNodes[i];
					StartIndex = i + 1;
					break;
				}
			}

			// If a valid baseline node was found, compare it against the rest
			if (SouthernMostNeighbourNode && StartIndex > 0)
			{
				for (int32 i = StartIndex; i < ConnectedNodes.Num(); ++i)
				{
					if (!ConnectedNodes[i] || !ConnectedNodes[i]->LocationComponent) continue;

					if (ConnectedNodes[i]->LocationComponent->GetComponentLocation().X < SouthernMostNeighbourNode->
						LocationComponent->GetComponentLocation().X)
					{
						SouthernMostNeighbourNode = ConnectedNodes[i];
					}
				}
			}
		}
	}

	return SouthernMostNeighbourNode;
}

ANavigationNode* AAgentCharacter::GetEastCellNavigationNode()
{
	ANavigationNode* EasternMostNeighbourNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			const TArray<ANavigationNode*>& ConnectedNodes =
				NearestNode->GetConnectedNodes();

			int32 StartIndex = -1;
			for (int32 i = 0; i < ConnectedNodes.Num(); ++i)
			{
				if (ConnectedNodes[i] && ConnectedNodes[i]->LocationComponent)
				{
					EasternMostNeighbourNode = ConnectedNodes[i];
					StartIndex = i + 1;
					break;
				}
			}

			// If a valid baseline node was found, compare it against the rest
			if (EasternMostNeighbourNode && StartIndex > 0)
			{
				for (int32 i = StartIndex; i < ConnectedNodes.Num(); ++i)
				{
					if (!ConnectedNodes[i] || !ConnectedNodes[i]->LocationComponent) continue;

					if (ConnectedNodes[i]->LocationComponent->GetComponentLocation().Y > EasternMostNeighbourNode->
						LocationComponent->GetComponentLocation().Y)
					{
						EasternMostNeighbourNode = ConnectedNodes[i];
					}
				}
			}
		}
	}

	return EasternMostNeighbourNode;
}

ANavigationNode* AAgentCharacter::GetWestCellNavigationNode()
{
	ANavigationNode* WesternMostNeighbourNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			const TArray<ANavigationNode*>& ConnectedNodes =
				NearestNode->GetConnectedNodes();

			int32 StartIndex = -1;
			for (int32 i = 0; i < ConnectedNodes.Num(); ++i)
			{
				if (ConnectedNodes[i] && ConnectedNodes[i]->LocationComponent)
				{
					WesternMostNeighbourNode = ConnectedNodes[i];
					StartIndex = i + 1;
					break;
				}
			}

			// If a valid baseline node was found, compare it against the rest
			if (WesternMostNeighbourNode && StartIndex > 0)
			{
				for (int32 i = StartIndex; i < ConnectedNodes.Num(); ++i)
				{
					if (!ConnectedNodes[i] || !ConnectedNodes[i]->LocationComponent) continue;

					if (ConnectedNodes[i]->LocationComponent->GetComponentLocation().Y < WesternMostNeighbourNode->
						LocationComponent->GetComponentLocation().Y)
					{
						WesternMostNeighbourNode = ConnectedNodes[i];
					}
				}
			}
		}
	}

	return WesternMostNeighbourNode;
}


// ALL NPC movement comes through this method
void AAgentCharacter::AgentMoveAlongPath(UAction* CurrentAgentAction)
{
	if (CurrentNodesPath.IsEmpty())
	{
		// Never called
		// CurrentAction->bIsCompleted = true;
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAction->bIsCompleted = true;"));
		return;
	}


	// Unstuck code - NPC cutting corners and getting stuck (hasn't moved more than 10cm)
	if (FVector::DistSquared(GetActorLocation(), LastCheckedLocation) < 50.0f) // Hard matching 5cm movement
	{
		StuckTimer += GetWorld()->GetDeltaSeconds();
		if (StuckTimer > 3.0f) // Stuck for 2 seconds
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC is stuck trying to reach node! Forcing path clear."));
			CurrentNodesPath.Empty();
			StuckTimer = 0.0f;
			
			RefreshCardinalPathWorldStates();

			return;
		}
	}
	else
	{
		LastCheckedLocation = GetActorLocation();
		StuckTimer = 0.0f;
	}

	FVector NextLocation = CurrentNodesPath[0];
	FVector NPCLocation = GetActorLocation();

	FVector NextLocationXY = FVector(NextLocation.X, NextLocation.Y, 0.0f);
	FVector NPCLocationXY = FVector(NPCLocation.X, NPCLocation.Y, 0.0f);

	FVector Direction = NextLocationXY - NPCLocationXY;
	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);

	// float MazePathingLocationThreshold = 160.0f; // 3D distance
	float MazePathingLocationThreshold = 110.0f; // 2D distance;

	bool bHasReachedNextLocationXY =
		FVector::DistSquared(NextLocationXY, NPCLocationXY) <=
		(MazePathingLocationThreshold * MazePathingLocationThreshold);
	if (bHasReachedNextLocationXY)
	{
		LastCheckedLocation = NPCLocation;
		StuckTimer = 0.0f;

		// EDIT: Moved to NavNode
		// EDIT: Moved back to AAgentCharacter because collider broke NavNode's USceneComponent
		// Updating a NavigationNode's visits counter
		GetNearestNavigationNode()->NPCVisitedCounter++;
		RefreshCardinalPathWorldStates();

		CurrentNodesPath.RemoveAt(0);
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentNodesPath.RemoveAt(0);"));


		if (CurrentNodesPath.IsEmpty())
		{

			CurrentAgentAction->bIsCompleted = true;
			UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAgentAction->bIsCompleted: %s"), *CurrentAgentAction->UniqueName);
		}
	}
}



int32 AAgentCharacter::AgentAddToPlannerNodes(const FAgentPlannerNode& NextNode)
{
	// Research suggests using Emplace on TArray shouldn't break index tracking
	int32 NextIndex = PlannerNodes.Emplace(NextNode);

	PlannerNodes[NextIndex].Index = NextIndex;

	return NextIndex;
}