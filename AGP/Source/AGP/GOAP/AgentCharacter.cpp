/**
 * AAgentCharacter cpp
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
 * Author: Karl Birk
 * UTS AGP 2026 Group 10
 */


// ReSharper disable CppUE4CodingStandardNamingViolationWarning
// ReSharper disable CppTooWideScope
#include "AgentCharacter.h"

#include "Action.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "../Pathfinding/NavigationNode.h"
#include "Components/CapsuleComponent.h"
#include "../Landscape/ObstacleBase.h"

// #include "Components/WidgetComponent.h"

// struct FAgentPlannerNode;


AAgentCharacter::AAgentCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}


void AAgentCharacter::BeginPlay()
{
	Super::BeginPlay();


	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AAgentCharacter::OnNPCOverlapBegin);
	}


	// Empty planning collections
	AgentCurrentPlan.Empty();
	CurrentPlanQueue.Empty();

	// Fetch the PCGMaze Start and End. Initially for testing, now built into
	// Action weighting variables. If direction of next path ends up being
	// farther away from EndOfMaze, Action is less favourable. There's another
	// heuristic related to preferring a path less travelled too.
	AActor* FoundActor =
		UGameplayStatics::GetActorOfClass(GetWorld(), AProceduralMazeFacility::StaticClass());
	if (FoundActor)
	{
		AProceduralMazeFacility* ProceduralMazeFacility =
			Cast<AProceduralMazeFacility>(FoundActor);
		if (ProceduralMazeFacility)
		{
			// Converted some ProceduralMazeFacility variables to UPROPERTY
			// for getters of PCG maze's FVectors Start and End. Values
			// needed to be retained between pre-PIE and post-PIE instances.
			StartOfMaze = ProceduralMazeFacility->GetMazeStartCellWorldPosition();
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter StartOfMaze %s"), *StartOfMaze.ToString());
			EndOfMaze = ProceduralMazeFacility->GetMazeEndCellWorldPosition();
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter EndOfMaze %s"), *EndOfMaze.ToString());
		}
	}


	// PCGMaze problem specific WorldStates, Goals and Actions
	// with a fallback Wander (aka Patrol).
	// Right now, NPC loves it when a Plan comes together.
	InitGOAPGoalsActionsWorldStates();

	// Goal priority 1.0f is higher than 10.0f
	SortGoalsAscendingPriority();

	// Super::CurrentNodesPath is utilised for pathing
	// refreshed during Super::BeginPlay()
	// (cardinal direction pathing, not quickest path cheating)
	CurrentNodesPath.Empty();
}

void AAgentCharacter::Tick(float DeltaTime)
{
	if (!bHasReachedEndOfMaze)
	{
		// This tracks enemy-sensing from Super::UpdateTargetLocation
		UpdateTargetLocation();

		// Some custom work for PCGMaze		
		TickDoCurrentPlan5();

		// Win condition check
		CheckReachedEndOfMaze();
	}
}


// PCGMaze specific implementation, observing current plan's action
// then enacting on an implementation. Currently, each action 
// implementation can be called in Tick, and toggle its bIsComplete 
// after it's determined that it is complete.
void AAgentCharacter::TickDoActionImplementation(TObjectPtr<UAction> Action)
{
	// Perform implementations of actions from here.
	// e.g. Evade(), Fire(), MoveTowardsGoal()

	// NOTE: GOAP allows for sequencing of actions to reach goals.
	// Cardinal direction choices in maze-solving criteria, by
	// themselves look boring.
	if (Action->UniqueName == Action_Wander)
	{
		AgentTickPatrol(Action);
	}
	else if (Action->UniqueName == Action_MoveTowards_StartOfMaze)
	{
		Action->bIsCompleted = MoveTowards(StartOfMaze);

		// AgentTickMoveToMazeStart(Action);
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

// PCGMaze problem specific WorldStates, Goals and Actions
// with a fallback Wander (aka Patrol).
// Right now, NPC loves it when a Plan comes together.
void AAgentCharacter::InitGOAPGoalsActionsWorldStates()
{
	// PCGMaze-specific base. 
	// NOTE: Weighting of Actions for NESW cardinal movement
	// updates in TickDoCurrentPlan5()
	// Goals weights are constant in this GOAP implementation.
	InitPCGMazeGoals();
}


// Observing CurrentQueuedAction (from a cached TQueue which came
// from a variable TArray). It is called once per frame.
// It acknowledges and resets movement-goal states after they're met.
void AAgentCharacter::TickDoCurrentPlan5()
{
	// ACTION bIsCompleted FLAGGED. 
	// Parse once and apply AgentWorldStates updates
	if (CurrentQueuedAction && CurrentQueuedAction->bIsCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("TickDoCurrentPlan5 ActionCompleted: %s. Applying outcomes."),
		       *CurrentQueuedAction->UniqueName);

		// Update AgentWorldStates. Some changes can be permanent
		// e.g. reaching a maze start goal.
		for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
		{
			UE_LOG(LogTemp, Warning, TEXT("TickDoCurrentPlan5 ActionOutcome.Key: %s"), *ActionOutcome.Key);


			// For goal completion like State_bHasReached_EndOfMaze
			AgentWorldState.StatesMap.Emplace(ActionOutcome.Key, ActionOutcome.Value);

			// Tried numerous permutations of WorldStates, Goals and Actions.
			// Track individual NESW movement goals and resets amongst.
			// Moving to nearby NavigationNodePtr->bIsJunction.
			// Temporarily blocking backwards pathing (after I observed behaviours).
		}

		// RESETTING cardinal movement mini goals. 
		// Could filter Action uniquename choices and blanket a few resets.
		AgentWorldState.StatesMap.Emplace(State_bHasMoved, false);


		// The boolean reset on the Action
		CurrentQueuedAction->bIsCompleted = false;
		CurrentQueuedAction = nullptr;

		// Checks whether cell (ANavigationNode location underfoot)
		// has available NESW openings.
		// In testing, this works best if calling at least upon:
		// • Each cell movement completion 
		// • Each other action completion
		// • Each new plan creation
		RefreshCardinalPathWorldStates();

		// In testing, early exit break here allows for AgentWorldState values
		// changes to be recognised by next GetPlan(). 
		// GetPlan was hidden from TickDoCurrentPlan5's access until 
		// bIsCompleted flags were all updated and parsed TQueue invalid.
		return;
	}

	// PER TICK, update action costs for NESW movement.
	// NOTES: In testing, this works best if called in Tick.
	// ANavigationNode* Current = GetNearestNavigationNode();
	ANavigationNode* North = GetNorthCellNavigationNode();
	ANavigationNode* South = GetSouthCellNavigationNode();
	ANavigationNode* East = GetEastCellNavigationNode();
	ANavigationNode* West = GetWestCellNavigationNode();

	// TEST: Adding favourability to EndOfMaze to each cardinal direction
	// EDIT: Final build GOAP HEURISTICS, because it works well!
	// PARSE CARDINAL MOVEMENT ACTIONS and update weighting, accounting for:
	// • ANavigationNode's NPCVisitedCounter (increments upon cell movements)
	//   and
	// • Distance to EndOfMaze delta between cardinal directions cell choices
	//   and
	// • Some trial and error with cardinal direction favourability 
	//   (North, positive X direction, favoured over E,W,S, from PCGMap design)
	// TODO:
	// (NiceToHave?) Cardinal direction favourability flexibility for
	// variants of PCGMaze that may start NESW and end NESW, of which N to N
	// in practice would be hard for GOAP alone, 
	// maybe by spawning additional Dijkstra bot zone scanners.
	for (UAction* Action : AgentActions)
	{
		// NPC's AgentActions TArray contains a blank Action.
		// Current GOAP design and implementation has permanency 
		// in all NPC's AgentActions.
		if (!Action) continue;

		// Favourite direction is north
		if (Action->UniqueName == Action_MoveTowards_North)
		{
			if (North)
			{
				float NorthBaseCost = GetDirectionalMoveCost(
					North,
					0.0f,
					EndOfMaze,
					GetActorLocation());
				Action->ActionCost =
					NorthBaseCost +
					(North->NPCVisitedCounter * 2.0f);
			}
		}
		else if (Action->UniqueName == Action_MoveTowards_South)
		{
			if (South)
			{
				// South direction is least favourable
				float SouthBaseCost =
					GetDirectionalMoveCost(
						South,
						0.5f,
						EndOfMaze,
						GetActorLocation());
				Action->ActionCost =
					SouthBaseCost +
					(South->NPCVisitedCounter * 2.0f);
			}
		}
		else if (Action->UniqueName == Action_MoveTowards_East)
		{
			if (East)
			{
				// East and west with equal base costs and 
				// NPCVisitedCounter considerations both
				// prevents ping-pong inside concave maze bends,
				// and lets NPC try south again if NEW has piles
				// of breadcrumbs.
				float EastBaseCost = GetDirectionalMoveCost(
					East,
					0.2f,
					EndOfMaze,
					GetActorLocation());
				Action->ActionCost =
					EastBaseCost +
					(East->NPCVisitedCounter * 2.0f);
			}
		}
		else if (Action->UniqueName == Action_MoveTowards_West)
		{
			if (West)
			{
				float WestBaseCost = GetDirectionalMoveCost(
					West,
					0.2f,
					EndOfMaze,
					GetActorLocation());
				Action->ActionCost =
					WestBaseCost +
					(West->NPCVisitedCounter * 2.0f);
			}
		}
	}
	// // THIS APPROACH HATES SOUTH TOO MUCH.
	// for (UAction* Action : AgentActions)
	// {
	// 	if (!Action) continue;
	// 	if (Action->UniqueName == Action_MoveTowards_North)
	// 	{
	// 		Action->ActionCost = 1.0f + (North->NPCVisitedCounter * 2.0f);
	// 	}
	// 	else if (Action->UniqueName == Action_MoveTowards_South)
	// 	{
	// 		Action->ActionCost = 10.0f + (South->NPCVisitedCounter * 2.0f);
	// 	}
	// 	else if (Action->UniqueName == Action_MoveTowards_East)
	// 	{
	// 		Action->ActionCost = 3.0f + (East->NPCVisitedCounter * 5.0f);
	// 	}
	// 	else if (Action->UniqueName == Action_MoveTowards_West)
	// 	{
	// 		Action->ActionCost = 3.0f + (West->NPCVisitedCounter * 5.0f);
	// 	}
	// }


	// PLAN QUEUE INVALID ACTION
	if (CurrentQueuedAction == nullptr)
	{
		// Refresh cardinal movement access in AgentWorldState
		RefreshCardinalPathWorldStates();

		// Pop invalid Action
		if (!CurrentPlanQueue.IsEmpty())
		{
			CurrentPlanQueue.Dequeue(CurrentQueuedAction);
		}
		else
		{
			// Can't pop from empty queue.
			// Try to find new Plan

			bool bIsPlanFound = false;

			// NOTE: AgentGoals was pre-sorted upon BeginPlay()
			// with priority 1.0f being higher than 10.0f priority.
			// Goal priority is constant in this GOAP implementation
			for (TObjectPtr<UGoal> Goal : AgentGoals)
			{
				// If no Goals found, there's a fallback hack later.
				if (!Goal) continue;


				// If this Goal's WorldStates are already entirely met, 
				// skip (e.g. State_bHasReached_StartOfMaze).
				// If no accessible Goals, there's a fallback later.
				if (AgentWorldState.HasAll(Goal->GoalWorldState))
				{
					continue;
				}

				// Empty the whole Plan TArray for clean Plan
				AgentCurrentPlan.Empty();


				// Fill the whole TArray for a new Plan (for this Goal)
				AgentGetPlan(
					AgentActions,
					AgentWorldState,
					Goal->GoalWorldState,
					AgentCurrentPlan);

				if (AgentCurrentPlan.Num() > 0)
				{
					/* Lots of printouts in troubleshooting */
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
						UE_LOG(LogTemp, Warning, TEXT("NEW PLAN starting with AgentWorldState: %s, %s"), *Pair.Key,
						       Pair.Value ? TEXT("true") : TEXT("false"));
					}


					for (const UAction* Action : AgentCurrentPlan)
					{
						UE_LOG(LogTemp, Warning, TEXT("NEW PLAN -> Plan Step: %s"), *Action->UniqueName);
					}
					/* Lots of printouts in troubleshooting */


					// Found a valid plan for best available goal
					bIsPlanFound = true;

					// Plan TArray built. TQueue rebuild (for poppable)
					BuildQueueFromPlan();

					break;
				}
			}


			// If PLAN was found
			if (bIsPlanFound && !CurrentPlanQueue.IsEmpty())
			{
				CurrentPlanQueue.Dequeue(CurrentQueuedAction);
			}
			else
			{
				// WHEN TROUBLESHOOTING, HAVE THIS UNCOMMENTED
				// IF GOAP Goals, Actions, WorldStates and therefore Plans
				// come together, this is block is never called.

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
		// Execution method for any Action (keeping AAction light weight)
		TickDoActionImplementation(CurrentQueuedAction);
	}
}


// From A3 APlanner::GetPlan, but now with MaxPlannerIterations
// to prevent PIE crashes when Plan stacks one Action infinitely.
// Research suggests Plan should be smaller and not stack one 
// Action on top of itself repeatedly.
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


// Copy of AGP TickPatrol but calling AgentMoveAlongPath(UAction*)
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


// PCGMaze-specific base world states, goals, actions
// NOTE: Weighting of Actions for NESW cardinal movement
// updates in TickDoCurrentPlan5()
// Goals weights are constant in this GOAP implementation.
void AAgentCharacter::InitPCGMazeGoals()
{
	// Agent's WorldStates
	AgentWorldState.StatesMap.Add(State_bHasWandered, false);
	AgentWorldState.StatesMap.Add(State_bHasReached_StartOfMaze, false);
	AgentWorldState.StatesMap.Add(State_bHasReached_EndOfMaze, false);
	AgentWorldState.StatesMap.Add(State_bHasPathNorth, true);
	AgentWorldState.StatesMap.Add(State_bHasPathSouth, true);
	AgentWorldState.StatesMap.Add(State_bHasPathEast, true);
	AgentWorldState.StatesMap.Add(State_bHasPathWest, true);

	// Agent's Goals
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

	TObjectPtr<UGoal> GoalMakeMazeMove = NewObject<UGoal>(this);
	GoalMakeMazeMove->GoalWorldState.StatesMap.Add(State_bHasMoved, true);
	GoalMakeMazeMove->GoalPriority = 2.0f; // Evaluated right after StartOfMaze
	AgentGoals.Add(GoalMakeMazeMove);

	// Agent's Actions - used by planner, implementation elsewhere, e.g. Patrol()
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
	ActionMoveTowardsNorth->ActionOutcomes.StatesMap.Add(State_bHasMoved, true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(State_bHasPathNorth, true);
	AgentActions.Add(ActionMoveTowardsNorth);

	TObjectPtr<UAction> ActionMoveTowardsSouth = NewObject<UAction>(this);
	ActionMoveTowardsSouth->UniqueName = Action_MoveTowards_South;
	ActionMoveTowardsSouth->ActionCost = 10.0;
	ActionMoveTowardsSouth->ActionOutcomes.StatesMap.Add(State_bHasMoved, true);
	ActionMoveTowardsSouth->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsSouth->ActionPrereqs.StatesMap.Add(State_bHasPathSouth, true);
	AgentActions.Add(ActionMoveTowardsSouth);

	TObjectPtr<UAction> ActionMoveTowardsEast = NewObject<UAction>(this);
	ActionMoveTowardsEast->UniqueName = Action_MoveTowards_East;
	ActionMoveTowardsEast->ActionCost = 3.0;
	ActionMoveTowardsEast->ActionOutcomes.StatesMap.Add(State_bHasMoved, true);
	ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(State_bHasPathEast, true);
	AgentActions.Add(ActionMoveTowardsEast);

	TObjectPtr<UAction> ActionMoveTowardsWest = NewObject<UAction>(this);
	ActionMoveTowardsWest->UniqueName = Action_MoveTowards_West;
	ActionMoveTowardsWest->ActionCost = 3.0;
	ActionMoveTowardsWest->ActionOutcomes.StatesMap.Add(State_bHasMoved, true);
	ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(State_bHasReached_StartOfMaze, true);
	ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(State_bHasPathWest, true);
	AgentActions.Add(ActionMoveTowardsWest);
}


// MOVEMENT comes through this method.
// Same as AGP MoveAlongPath with UAction* bIsCompleted flagged true
// and some minor PCGMaze manoeuvrability tricks (tight corners).
// Also increments a nearby ANavigationNode->NPCVisitedCounter++
void AAgentCharacter::AgentMoveAlongPath(UAction* CurrentAgentAction)
{
	if (CurrentNodesPath.IsEmpty())
	{
		// Never called. Checking whether UPathfindingSubsystem called
		// previously to have ANavigationNode spawning and connecting.
		// CurrentAction->bIsCompleted = true;
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAction->bIsCompleted = true;"));
		return;
	}


	// PCGMaze unstuck code - NPC was cutting corners and getting stuck
	// and PCGMaze helper class was spawning Obstacles blueprints that 
	// had full mesh colliders.
	// 50.0f here means NPC hasn't moved more than 50cm per tick.
	if (FVector::DistSquared(GetActorLocation(), LastCheckedLocation) < 50.0f)
	{
		StuckTimer += GetWorld()->GetDeltaSeconds();
		if (StuckTimer > 3.0f) // Stuck for 3 seconds
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
	// Flat maze so using 2D distances and movement vectors.
	float MazePathingLocationThreshold = 110.0f; // 2D distance;

	bool bHasReachedNextLocationXY =
		FVector::DistSquared(NextLocationXY, NPCLocationXY) <=
		(MazePathingLocationThreshold * MazePathingLocationThreshold);
	if (bHasReachedNextLocationXY)
	{
		LastCheckedLocation = NPCLocation;
		StuckTimer = 0.0f;

		// Incrementing a NavigationNode's visits counter
		// EDIT: Moved incrementing logic to ANavigationNode.
		// EDIT: Moved back to AAgentCharacter because new collider 
		// broke ANavigationNode's USceneComponent (but ANavigationNode 
		// seems to support ActorGetLocation() as an Undo of this choice?).
		GetNearestNavigationNode()->NPCVisitedCounter++;

		RefreshCardinalPathWorldStates();

		CurrentNodesPath.RemoveAt(0);
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentNodesPath.RemoveAt(0);"));


		if (CurrentNodesPath.IsEmpty())
		{
			CurrentAgentAction->bIsCompleted = true;
			UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAgentAction->bIsCompleted: %s"),
			       *CurrentAgentAction->UniqueName);
		}
	}
}


// Listening for collider overlaps for AObstacleBase
void AAgentCharacter::OnNPCOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		AObstacleBase* OverlappedObstacle = Cast<AObstacleBase>(OtherActor);

		if (OverlappedObstacle)
		{
			HandleObstacleCollision(OverlappedObstacle);
		}
	}
}

void AAgentCharacter::HandleObstacleCollision( AObstacleBase* Obstacle)
{
	if (Obstacle)
	{
		bool bNPCKnowsItIsMud = Obstacle->IsMud();
		bool bNPCKnowsItIsSlime = Obstacle->IsSlime();

		if (bNPCKnowsItIsMud || bNPCKnowsItIsSlime)
		{
			SetNearestNavNodeIsMud(true);
		}

	}
}


// Using ANavigationNode pathing nodes and neighbour-connections, then
// AgentMoveAlongPath(UAction*) can update the UAction's bIsCompleted
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


// Using ANavigationNode pathing nodes and neighbour-connections, then
// AgentMoveAlongPath(UAction*) can update the UAction's bIsCompleted
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


// Using ANavigationNode pathing nodes and neighbour-connections, then
// AgentMoveAlongPath(UAction*) can update the UAction's bIsCompleted
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


// Using ANavigationNode pathing nodes and neighbour-connections, then
// AgentMoveAlongPath(UAction*) can update the UAction's bIsCompleted
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


// Using ANavigationNode pathing nodes and neighbour-connections, then
// AgentMoveAlongPath(UAction*) can update the UAction's bIsCompleted
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


// Checks whether cell (ANavigationNode location underfoot)
// has available NESW openings.
// In testing, this works best if calling at least upon:
// • Each cell movement completion 
// • Each other action completion
// • Each new plan creation
void AAgentCharacter::RefreshCardinalPathWorldStates()
{
	// Defaults first
	AgentWorldState.StatesMap.Emplace(State_bHasPathNorth, false);
	AgentWorldState.StatesMap.Emplace(State_bHasPathSouth, false);
	AgentWorldState.StatesMap.Emplace(State_bHasPathEast, false);
	AgentWorldState.StatesMap.Emplace(State_bHasPathWest, false);


	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			float CellDistanceThreshold = 150.0f; // Half of the 300 units square cells in PCGMaze

			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				// ReSharper disable once CommentTypo
				// Iterate through neighbournodes
				for (int32 i = 0; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					ANavigationNode* NeighbourNode = NearestNode->GetConnectedNodes()[i];

					float DistanceX =
						NeighbourNode->LocationComponent->GetComponentLocation().X -
						NearestNode->LocationComponent->GetComponentLocation().X;

					float DistanceY =
						NeighbourNode->LocationComponent->GetComponentLocation().Y -
						NearestNode->LocationComponent->GetComponentLocation().Y;

					// North, south, east, west, looking for positives
					bool bHasPathNorth = DistanceX > CellDistanceThreshold;
					if (bHasPathNorth)
					{
						AgentWorldState.StatesMap.Emplace(State_bHasPathNorth, true);
					}

					bool bHasPathSouth = -DistanceX > CellDistanceThreshold;
					if (bHasPathSouth)
					{
						AgentWorldState.StatesMap.Emplace(State_bHasPathSouth, bHasPathSouth);
					}

					bool bHasPathEast = DistanceY > CellDistanceThreshold;
					if (bHasPathEast)
					{
						AgentWorldState.StatesMap.Emplace(State_bHasPathEast, bHasPathEast);
					}

					bool bHasPathWest = -DistanceY > CellDistanceThreshold;
					if (bHasPathWest)
					{
						AgentWorldState.StatesMap.Emplace(State_bHasPathWest, bHasPathWest);
					}
				}
			}
		}
	}
}


// Win condition called inside Tick, which had stopped NPC's GOAP engine
// upon bHasReachedEndOfMaze = true;
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

// Helper method that allows for shorter lines code
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


// Update nearest ANavigationNode's bIsMud bool
// ANavigationNode's memory is accessible by all NPCs
// so seems to work best if NPCs vs PCs multiplayer
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


// Update nearest ANavigationNode's bIsBlocked bool
// ANavigationNode's memory is accessible by all NPCs
// so seems to work best if NPCs vs PCs multiplayer
void AAgentCharacter::SetNearestNavNodeIsBlocked(bool bIsBlocked)
{
	ANavigationNode* NearestNavNode = GetNearestNavigationNode();
	if (NearestNavNode)
	{
		NearestNavNode->bIsBlocked = bIsBlocked;
	}
}


// Update nearest ANavigationNode's bIsJunction bool
// ANavigationNode's memory is accessible by all NPCs
// so seems to work best if NPCs vs PCs multiplayer
void AAgentCharacter::SetNearestNavNodeIsJunction(bool bIsJunction)
{
	ANavigationNode* NearestNavNode = GetNearestNavigationNode();
	if (NearestNavNode)
	{
		NearestNavNode->bIsJunction = bIsJunction;
	}
}

// Update nearest ANavigationNode's bIsDeadend bool
// ANavigationNode's memory is accessible by all NPCs
// so seems to work best if NPCs vs PCs multiplayer
void AAgentCharacter::SetNearestNavNodeIsDeadend(bool bIsDeadend)
{
	ANavigationNode* NearestNavNode = GetNearestNavigationNode();
	if (NearestNavNode)
	{
		NearestNavNode->bIsDeadend = bIsDeadend;
	}
}


// Update nearest ANavigationNode's bIsDistraction bool
// ANavigationNode's memory is accessible by all NPCs
// so seems to work best if NPCs vs PCs multiplayer
void AAgentCharacter::SetNearestNavNodeIsDistraction(bool bIsDistraction)
{
	ANavigationNode* NearestNavNode = GetNearestNavigationNode();
	if (NearestNavNode)
	{
		NearestNavNode->bIsDistraction = bIsDistraction;
	}
}


// Update TQueue of Action pointers. Queue has Pop abilities
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

// Unused in final A4 build. 
// Convenient TickMovement with destination-reached boolean
// especially if UPathfindingSubsystem isn't a thing.
bool AAgentCharacter::MoveTowards(const FVector& DesiredLocation)
{
	FVector NPCLocation = GetActorLocation();

	FVector Direction = DesiredLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);

	// Walking through PCGMaze likes some tighter waypoints adherence
	// if (FVector::DistSquared(DesiredLocation, NPCLocation) <= 62'500.0f) // 2.5m
	if (FVector::DistSquared(DesiredLocation, NPCLocation) <= 22'500.0f) // 1.5m
	{
		return true;
	}
	else
	{
		return false;
	}
}


// Goal priority as 1.0f higher than 10.0f. Can be called within
// Planning methods if variable goals priorities, but current
// GOAP implementation has constant goal priorities.
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

// Cardinal direction node getter
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


// Cardinal direction node getter
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


// Cardinal direction node getter
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


// Cardinal direction node getter
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


// Copy of A3 Planner::AddToPlannerNodes
int32 AAgentCharacter::AgentAddToPlannerNodes(const FAgentPlannerNode& NextNode)
{
	// Research suggests using Emplace on TArray shouldn't break index tracking
	int32 NextIndex = PlannerNodes.Emplace(NextNode);

	PlannerNodes[NextIndex].Index = NextIndex;

	return NextIndex;
}

float AAgentCharacter::GetDirectionalMoveCost(
	const ANavigationNode* TargetNode,
	float FlatModifier,
	const FVector& GoalLocation,
	const FVector& CurrentLocation)
{
	// Penalty for invalid or null nodes
	if (!TargetNode)
	{
		return 100.0f;
	}

	float CurrentDist = FVector::Dist(CurrentLocation, GoalLocation);
	float TargetDist = FVector::Dist(TargetNode->GetActorLocation(), GoalLocation);

	// If the move takes us further from the goal (like moving South), 
	// it applies a small, natural penalty rather than a hardcoded block.
	float DistancePenalty = 0.0f;
	if (TargetDist > CurrentDist)
	{
		// DistancePenalty = 1.5f;
		DistancePenalty = 1.1f;
	}

	return 1.0f + DistancePenalty + FlatModifier;
}
