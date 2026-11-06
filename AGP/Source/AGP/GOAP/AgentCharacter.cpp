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


	// Resetting most goals after 10 seconds
	// GetWorld()->GetTimerManager().SetTimer(
	// 	VariableTimerHandle,
	// 	this,
	// 	&AAgentCharacter::ResetGoldBlueGreenGoals,
	// 	10.0f,
	// 	false
	// );

	// ANavigationNode* FindNearestNode(const FVector& TargetLocation);


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

			// EDIT: unexpected defaults appearing
			// Reviewing ProceduralMazeFacility public getters 
			// Note:
			// "Unreal Engine runs multiple worlds simultaneously in the editor:
			// • Editor World (EWorldType::Editor): 
			// This is the world you see when modifying levels, placing actors, and 
			// where your editor-ticking actor is updating the variable.
			// • PIE World (EWorldType::PIE): 
			// This is a brand-new, isolated world created the moment you click "Play.""
		}
	}


	InitGOAPGoalsActionsWorldStates();

	SortGoalsDescendingPriority();
}

void AAgentCharacter::Tick(float DeltaTime)
{
	UpdateTargetLocation();

	TickDoCurrentPlan5();


	// Visual lines and linetracing in maze
	// FVector StartPos = GetActorLocation();
	//
	// // 4 lines, 150 units in world X and world Y axes
	// FVector EndXPlus  = StartPos + FVector(150.f, 0.f, 0.f);
	// FVector EndXMinus = StartPos + FVector(-150.f, 0.f, 0.f);
	// FVector EndYPlus  = StartPos + FVector(0.f, 150.f, 0.f);
	// FVector EndYMinus = StartPos + FVector(0.f, -150.f, 0.f);
	//
	// // Line Thickness = 2.0f
	// DrawDebugLine(GetWorld(), StartPos, EndXPlus,  FColor::Red,    false, -1, 0, 2.f);
	// DrawDebugLine(GetWorld(), StartPos, EndXMinus, FColor::Red,  false, -1, 0, 2.f);
	// DrawDebugLine(GetWorld(), StartPos, EndYPlus,  FColor::Red,   false, -1, 0, 2.f);
	// DrawDebugLine(GetWorld(), StartPos, EndYMinus, FColor::Red, false, -1, 0, 2.f);


	// Linetracing and debugdrawlines
	// FVector StartLocation = GetActorLocation();
	//
	// // Line ends
	// FVector X150Plus = StartLocation + FVector(150.0f, 0.0f, 0.0f);
	// FVector X150Minus = StartLocation + FVector(-150.f, 0.f, 0.f);
	// FVector Y150Plus = StartLocation + FVector(0.f, 150.f, 0.f);
	// FVector Y150Minus = StartLocation + FVector(0.f, -150.f, 0.f);
	//
	// // Ignore hits against the owner NPC 
	// FHitResult HitResult;
	// FCollisionQueryParams QueryParams;
	// QueryParams.AddIgnoredActor(this);
	//
	// // Line trace looking for World static objects (like walls)
	// bool bHitX150Plus = GetWorld()->LineTraceSingleByChannel(
	// 	HitResult,
	// 	StartLocation,
	// 	X150Plus,
	// 	ECC_WorldStatic, // Static walls
	// 	QueryParams
	// );
	// DrawDebugLine(GetWorld(), StartLocation, X150Plus, bHitX150Plus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	//               2.0f);
	//
	// bool bHitX150Minus = GetWorld()->LineTraceSingleByChannel(
	// 	HitResult,
	// 	StartLocation,
	// 	X150Minus,
	// 	ECC_WorldStatic, // Static walls
	// 	QueryParams
	// );
	// DrawDebugLine(GetWorld(), StartLocation, X150Minus, bHitX150Minus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	//               2.0f);
	//
	// bool bHitY150Plus = GetWorld()->LineTraceSingleByChannel(
	// 	HitResult,
	// 	StartLocation,
	// 	Y150Plus,
	// 	ECC_WorldStatic, // Static walls
	// 	QueryParams
	// );
	// DrawDebugLine(GetWorld(), StartLocation, Y150Plus, bHitY150Plus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	//               2.0f);
	//
	// bool bHitY150Minus = GetWorld()->LineTraceSingleByChannel(
	// 	HitResult,
	// 	StartLocation,
	// 	Y150Minus,
	// 	ECC_WorldStatic, // Static walls
	// 	QueryParams
	// );
	// DrawDebugLine(GetWorld(), StartLocation, Y150Minus, bHitY150Minus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	//               2.0f);

	// If a wall is detected, tell PathfindingSubsystem
	// if (bHitX150Plus && HitResult.GetActor())
	// {
	// 	if (PathfindingSubsystem)
	// 	{
	// 		// UProceduralMeshComponent
	// 		UE_LOG(LogTemp, Warning, TEXT("bHitX150Plus = True"));
	// 	}
	// }
	// if (bHitX150Minus && HitResult.GetActor())
	// {
	// 	if (PathfindingSubsystem)
	// 	{
	// 		// UProceduralMeshComponent
	// 		UE_LOG(LogTemp, Warning, TEXT("bHitX150Minus = True"));
	// 	}
	// }
	// if (bHitY150Plus && HitResult.GetActor())
	// {
	// 	if (PathfindingSubsystem)
	// 	{
	// 		// UProceduralMeshComponent
	// 		UE_LOG(LogTemp, Warning, TEXT("bHitY150Plus = True"));
	// 	}
	// }
	// if (bHitY150Minus && HitResult.GetActor())
	// {
	// 	if (PathfindingSubsystem)
	// 	{
	// 		// UProceduralMeshComponent
	// 		UE_LOG(LogTemp, Warning, TEXT("bHitY150Minus = True"));
	// 	}
	// }
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
		// UE_LOG(LogTemp, Display, TEXT("StartOfMaze: %s"), *StartOfMaze.ToString());

		bool bIsGoalMet = MoveTowards(StartOfMaze);
		if (bIsGoalMet)
		{
			UE_LOG(LogTemp, Warning, TEXT("bIsGoalMet = MoveTowards(StartOfMaze): %s"), *StartOfMaze.ToString());

			Action->bIsCompleted = true;

			// MANUAL HACK - CurrentNodesPath was earlier autogenerated upon BeginPlay()
			CurrentNodesPath.Empty();
			RefreshCardinalPathWorldStates();
		}
	}
	else if (Action->UniqueName == "Action_MoveTowards_North")
	{
		AgentTickMoveNorth(Action);
	}
	else if (Action->UniqueName == "Action_MoveTowards_South")
	{
		AgentTickMoveSouth(Action);
	}
	else if (Action->UniqueName == "Action_MoveTowards_East")
	{
		AgentTickMoveEast(Action);
	}
	else if (Action->UniqueName == "Action_MoveTowards_West")
	{
		AgentTickMoveWest(Action);
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

void AAgentCharacter::InitGOAPGoalsActionsWorldStates()
{
	// Wander - Default goal and action, goal is never reached by NPC (poor guy).
	AgentWorldState.StatesMap.Add(TEXT("bHasWandered"), false);

	TObjectPtr<UGoal> GoalWander = NewObject<UGoal>(this);
	GoalWander->GoalWorldState.StatesMap.Add(TEXT("bHasWandered"), true);
	GoalWander->GoalPriority = 99.0f;
	AgentGoals.Add(GoalWander);

	TObjectPtr<UAction> ActionWander = NewObject<UAction>(this);
	ActionWander->UniqueName = "Action_Wander";
	ActionWander->ActionCost = 10.0f;
	ActionWander->ActionOutcomes.StatesMap.Add(TEXT("bHasWandered"), true);
	ActionWander->ActionPrereqs.StatesMap.Empty();
	AgentActions.Add(ActionWander);


	InitPCGMazeGoals();
}

void AAgentCharacter::SortGoalsDescendingPriority()
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


void AAgentCharacter::TickDoCurrentPlan5()
{
	// When action has completed, update AgentWorldState
	if (CurrentQueuedAction && CurrentQueuedAction->bIsCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Action completed: %s. Applying outcomes."), *CurrentQueuedAction->UniqueName);

		// Update AgentWorldState
		for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
		{
			AgentWorldState.StatesMap.Emplace(ActionOutcome.Key, ActionOutcome.Value);
		}

		CurrentQueuedAction->bIsCompleted = false;
		CurrentQueuedAction = nullptr;

		// Early exit breaks patrol state?
		return;
	}

	// If queued action empty/null, try to fetch or build a new plan
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
				if (!Goal) continue;

				// If the goal state is already entirely met, skip planning for it
				if (AgentWorldState.HasAll(Goal->GoalWorldState))
				{
					continue;
				}

				// Clean the plan container before rewriting
				AgentCurrentPlan.Empty();

				// Testing hanging code here:
				// Safe guard: Ensure planner actually populates data safely
				// Planner->GetPlan(AgentActions, AgentWorldState, Goal->GoalWorldState, AgentCurrentPlan);
				AgentGetPlan(AgentActions, AgentWorldState, Goal->GoalWorldState, AgentCurrentPlan);

				if (AgentCurrentPlan.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("NPC successfully formed a new plan for goal!"));
					for (const UAction* Action : AgentCurrentPlan)
					{
						UE_LOG(LogTemp, Warning, TEXT(" -> Plan Step: %s"), *Action->UniqueName);
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
				if (CurrentQueuedAction && CurrentQueuedAction->UniqueName == "Action_Wander")
				{
					// The NPC is already wandering! Letting it keep its path.
				}
				else
				{
					UE_LOG(LogTemp, VeryVerbose, TEXT("No viable GOAP plan found. Wandering."));
					for (TObjectPtr<UAction> Action : AgentActions)
					{
						if (Action && Action->UniqueName == "Action_Wander")
						{
							UE_LOG(LogTemp, VeryVerbose, TEXT("NPC DEFAULT WANDER BEHAVIOUR CHANGED. BUHAO"));

							// THIS hack IGNORES whether this action's outcome had updated agentworldstate.
							CurrentQueuedAction = Action;
							CurrentQueuedAction->bIsCompleted = false;
							AgentWorldState.StatesMap.Emplace(TEXT("bHasWandered"), false);
							break;
						}
					}
				}
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
	// TSet<uint32> OpenSet;

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
			UE_LOG(LogTemp, Error, TEXT("AgentGetPlan: MaxPlannerIterations"));
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

int32 AAgentCharacter::AgentAddToPlannerNodes(const FAgentPlannerNode& NextNode)
{
	// Research suggests using Emplace on TArray shouldn't break index tracking
	int32 NextIndex = PlannerNodes.Emplace(NextNode);

	PlannerNodes[NextIndex].Index = NextIndex;

	return NextIndex;
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
				CurrentAgentAction->bIsCompleted = true;
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickPatrol NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
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
			// for (FVector currentNodeLoc : CurrentNodesPath)
			// for (int32 i = 0; i < CurrentNodesPath.Num(); ++i)
			// {
			// 	UE_LOG(LogTemp, Warning, TEXT("Have CurrentNodesPath; CurrentNodesPath[%d]: %s"), i,*CurrentNodesPath[i].ToString());
			// }
			// UE_LOG(LogTemp, Warning, TEXT("Have CurrentNodesPath; CurrentNodesPath[0]: %s"),
			//        *CurrentNodesPath[0].ToString());

			// UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickPatrol() CurrentNodesPath.IsEmpty() == false"));
		}


		// 
		// CurrentAgentAction->bIsCompleted = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickPatrol PathfindingSubsystem not found"));
	}


	AgentMoveAlongPath(CurrentAgentAction);
}

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

	if (FVector::DistSquared(NextLocationXY, NPCLocationXY) <=
		(MazePathingLocationThreshold * MazePathingLocationThreshold))
	{
		LastCheckedLocation = NPCLocation;
		StuckTimer = 0.0f;

		CurrentNodesPath.RemoveAt(0);
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentNodesPath.RemoveAt(0);"));

		if (CurrentNodesPath.IsEmpty())
		{
			CurrentAgentAction->bIsCompleted = true;
			UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAction->bIsCompleted = true;"));
		}
	}
}


void AAgentCharacter::InitPCGMazeGoals()
{
	AgentWorldState.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), false);

	TObjectPtr<UGoal> GoalBeAtStartOfMaze = NewObject<UGoal>(this);
	GoalBeAtStartOfMaze->GoalWorldState.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	GoalBeAtStartOfMaze->GoalPriority = 1.0f;
	AgentGoals.Add(GoalBeAtStartOfMaze);

	TObjectPtr<UAction> ActionMoveTowardsStartOfMaze = NewObject<UAction>(this);
	ActionMoveTowardsStartOfMaze->UniqueName = "Action_MoveTowards_StartOfMaze";
	ActionMoveTowardsStartOfMaze->ActionCost = 0.2;
	ActionMoveTowardsStartOfMaze->ActionOutcomes.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	ActionMoveTowardsStartOfMaze->ActionPrereqs.StatesMap.Empty();
	AgentActions.Add(ActionMoveTowardsStartOfMaze);


	AgentWorldState.StatesMap.Add(TEXT("bHasMovedNorth"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasMovedSouth"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasMovedEast"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasMovedWest"), false);


	AgentWorldState.StatesMap.Add(TEXT("bHasPathNorth"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasPathSouth"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasPathEast"), false);
	AgentWorldState.StatesMap.Add(TEXT("bHasPathWest"), false);

	TObjectPtr<UGoal> GoalHasMovedNorth = NewObject<UGoal>(this);
	GoalHasMovedNorth->GoalWorldState.StatesMap.Add(TEXT("bHasMovedNorth"), true);
	GoalHasMovedNorth->GoalPriority = 2.0f;
	AgentGoals.Add(GoalHasMovedNorth);

	TObjectPtr<UAction> ActionMoveTowardsNorth = NewObject<UAction>(this);
	ActionMoveTowardsNorth->UniqueName = "Action_MoveTowards_North";
	ActionMoveTowardsNorth->ActionCost = 1.0;
	ActionMoveTowardsNorth->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedNorth"), true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(TEXT("bHasPathNorth"), true);
	AgentActions.Add(ActionMoveTowardsNorth);


	TObjectPtr<UGoal> GoalHasMovedSouth = NewObject<UGoal>(this);
	GoalHasMovedSouth->GoalWorldState.StatesMap.Add(TEXT("bHasMovedSouth"), true);
	GoalHasMovedSouth->GoalPriority = 4.0f;
	AgentGoals.Add(GoalHasMovedSouth);

	TObjectPtr<UAction> ActionMoveTowardsSouth = NewObject<UAction>(this);
	ActionMoveTowardsSouth->UniqueName = "Action_MoveTowards_South";
	ActionMoveTowardsSouth->ActionCost = 1.0;
	ActionMoveTowardsSouth->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedSouth"), true);
	ActionMoveTowardsSouth->ActionPrereqs.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(TEXT("bHasPathSouth"), true);
	AgentActions.Add(ActionMoveTowardsSouth);


	TObjectPtr<UGoal> GoalHasMovedEast = NewObject<UGoal>(this);
	GoalHasMovedEast->GoalWorldState.StatesMap.Add(TEXT("bHasMovedEast"), true);
	GoalHasMovedEast->GoalPriority = 3.0f;
	AgentGoals.Add(GoalHasMovedEast);

	TObjectPtr<UAction> ActionMoveTowardsEast = NewObject<UAction>(this);
	ActionMoveTowardsEast->UniqueName = "Action_MoveTowards_East";
	ActionMoveTowardsEast->ActionCost = 1.0;
	ActionMoveTowardsEast->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedEast"), true);
	ActionMoveTowardsEast->ActionPrereqs.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(TEXT("bHasPathEast"), true);
	AgentActions.Add(ActionMoveTowardsEast);


	TObjectPtr<UGoal> GoalHasMovedWest = NewObject<UGoal>(this);
	GoalHasMovedWest->GoalWorldState.StatesMap.Add(TEXT("bHasMovedWest"), true);
	GoalHasMovedWest->GoalPriority = 3.0f;
	AgentGoals.Add(GoalHasMovedWest);

	TObjectPtr<UAction> ActionMoveTowardsWest = NewObject<UAction>(this);
	ActionMoveTowardsWest->UniqueName = "Action_MoveTowards_West";
	ActionMoveTowardsWest->ActionCost = 1.0;
	ActionMoveTowardsWest->ActionOutcomes.StatesMap.Add(TEXT("bHasMovedWest"), true);
	ActionMoveTowardsWest->ActionPrereqs.StatesMap.Add(TEXT("bHasReachedStartOfMaze"), true);
	ActionMoveTowardsNorth->ActionPrereqs.StatesMap.Add(TEXT("bHasPathWest"), true);
	AgentActions.Add(ActionMoveTowardsWest);
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


ANavigationNode* AAgentCharacter::GetNorthCellNavigationNode()
{
	ANavigationNode* NorthernMostNeighbourNode = nullptr;

	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{
			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				NorthernMostNeighbourNode =
					NearestNode->GetConnectedNodes()[0];

				for (int32 i = 1; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					if (NearestNode[i].
					    GetConnectedNodes()[i]->
					    LocationComponent->
					    GetComponentLocation().X
						>
						NorthernMostNeighbourNode->
						LocationComponent->
						GetComponentLocation().X)
					{
						NorthernMostNeighbourNode = NearestNode->GetConnectedNodes()[i];
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
			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				SouthernMostNeighbourNode =
					NearestNode->GetConnectedNodes()[0];

				for (int32 i = 1; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					if (NearestNode[i].
					    GetConnectedNodes()[i]->
					    LocationComponent->
					    GetComponentLocation().X
						>
						SouthernMostNeighbourNode->
						LocationComponent->
						GetComponentLocation().X)
					{
						SouthernMostNeighbourNode = NearestNode->GetConnectedNodes()[i];
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
			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				EasternMostNeighbourNode =
					NearestNode->GetConnectedNodes()[0];

				for (int32 i = 1; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					if (NearestNode[i].
					    GetConnectedNodes()[i]->
					    LocationComponent->
					    GetComponentLocation().X
						>
						EasternMostNeighbourNode->
						LocationComponent->
						GetComponentLocation().X)
					{
						EasternMostNeighbourNode = NearestNode->GetConnectedNodes()[i];
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
			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				WesternMostNeighbourNode =
					NearestNode->GetConnectedNodes()[0];

				for (int32 i = 1; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					if (NearestNode[i].
					    GetConnectedNodes()[i]->
					    LocationComponent->
					    GetComponentLocation().X
						>
						WesternMostNeighbourNode->
						LocationComponent->
						GetComponentLocation().X)
					{
						WesternMostNeighbourNode = NearestNode->GetConnectedNodes()[i];
					}
				}
			}
		}
	}

	return WesternMostNeighbourNode;
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
			CurrentNodesPath = PathfindingSubsystem->
				GetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveNorth() GetRandomPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				CurrentAgentAction->bIsCompleted = true;
				RefreshCardinalPathWorldStates();
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveNorth NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
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
				GetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveSouth() GetRandomPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				CurrentAgentAction->bIsCompleted = true;
				RefreshCardinalPathWorldStates();

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveSouth NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
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
				GetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveEast() GetRandomPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				CurrentAgentAction->bIsCompleted = true;
				RefreshCardinalPathWorldStates();

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveEast NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
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
}

void AAgentCharacter::AgentTickMoveWest(UAction* CurrentAgentAction)
{
	FVector DestinationLocation = GetWestCellNavigationNode()->LocationComponent->GetComponentLocation();
	if (PathfindingSubsystem)
	{
		if (CurrentNodesPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::AgentTickMoveWest() CurrentNodesPath.IsEmpty() == true"));

			CurrentNodesPath = PathfindingSubsystem->
				GetPath(
					GetActorLocation(),
					DestinationLocation);


			UE_LOG(LogTemp, Warning, TEXT("AgentTickMoveWest() GetRandomPath(GetActorLocation()"));

			if (CurrentNodesPath.IsEmpty())
			{
				CurrentAgentAction->bIsCompleted = true;
				RefreshCardinalPathWorldStates();

				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter AgentTickMoveWest NPC Loc: %s | First Path Node: %s"),
				       *GetActorLocation().ToString(),
				       *CurrentNodesPath[0].ToString());
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
}

// Updates Agent's wordstate of cardinal direction pathing available
void AAgentCharacter::RefreshCardinalPathWorldStates()
{
	if (PathfindingSubsystem)
	{
		ANavigationNode* NearestNode = PathfindingSubsystem->FindNearestNode(GetActorLocation());
		if (NearestNode)
		{

			float CellDistanceThreshold = 150.0f;

			if (NearestNode->GetConnectedNodes().Num() > 0)
			{
				for (int32 i = 0; i < NearestNode->GetConnectedNodes().Num(); ++i)
				{
					ANavigationNode* NeighbourNode = NearestNode->GetConnectedNodes()[i];
					bool bHasPathNorth =
						NeighbourNode->LocationComponent->GetComponentLocation().X -
						(NearestNode->LocationComponent->GetComponentLocation().X)
						< CellDistanceThreshold;

					bool bHasPathSouth =
						NeighbourNode->LocationComponent->GetComponentLocation().X -
						(NearestNode->LocationComponent->GetComponentLocation().X)
						> CellDistanceThreshold;

					bool bHasPathEast =
						NeighbourNode->LocationComponent->GetComponentLocation().Y -
						(NearestNode->LocationComponent->GetComponentLocation().Y)
						< CellDistanceThreshold;

					bool bHasPathWest = 
						NeighbourNode->LocationComponent->GetComponentLocation().Y -
						(NearestNode->LocationComponent->GetComponentLocation().Y) 
						> CellDistanceThreshold;


					if (bHasPathNorth)
					{
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathNorth"), true);
					}
					
					if (bHasPathSouth)
					{
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathSouth"), true);
					}
					
					if (bHasPathEast)
					{
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathEast"), true);
					}
					
					if (bHasPathWest)
					{
						AgentWorldState.StatesMap.Emplace(TEXT("bHasPathWest"), true);
					}
				}
			}
		}
	}
}
