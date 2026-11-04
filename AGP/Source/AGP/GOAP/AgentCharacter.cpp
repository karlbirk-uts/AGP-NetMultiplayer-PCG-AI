// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AgentCharacter.h"

#include "Action.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/Actor.h"

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


	InitGOAPGoalsActionsWorldStates();

	SortGoalsDescendingPriority();


	// Resetting most goals after 10 seconds
	GetWorld()->GetTimerManager().SetTimer(
		VariableTimerHandle,
		this,
		&AAgentCharacter::ResetGoldBlueGreenGoals,
		10.0f,
		false
	);
}

void AAgentCharacter::Tick(float DeltaTime)
{
	UpdateTargetLocation();

	TickDoCurrentPlan5();


	// Visual lines
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
	FVector StartLocation = GetActorLocation();

	// Line ends
	FVector X150Plus = StartLocation + FVector(150.0f, 0.0f, 0.0f);
	FVector X150Minus = StartLocation + FVector(-150.f, 0.f, 0.f);
	FVector Y150Plus = StartLocation + FVector(0.f, 150.f, 0.f);
	FVector Y150Minus = StartLocation + FVector(0.f, -150.f, 0.f);

	// Ignore hits against the owner NPC 
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Line trace looking for World static objects (like walls)
	bool bHitX150Plus = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		X150Plus,
		ECC_WorldStatic, // Static walls
		QueryParams
	);
	DrawDebugLine(GetWorld(), StartLocation, X150Plus, bHitX150Plus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	              2.0f);

	bool bHitX150Minus = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		X150Minus,
		ECC_WorldStatic, // Static walls
		QueryParams
	);
	DrawDebugLine(GetWorld(), StartLocation, X150Minus, bHitX150Minus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	              2.0f);

	bool bHitY150Plus = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		Y150Plus,
		ECC_WorldStatic, // Static walls
		QueryParams
	);
	DrawDebugLine(GetWorld(), StartLocation, Y150Plus, bHitY150Plus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	              2.0f);

	bool bHitY150Minus = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		Y150Minus,
		ECC_WorldStatic, // Static walls
		QueryParams
	);
	DrawDebugLine(GetWorld(), StartLocation, Y150Minus, bHitY150Minus ? FColor::Green : FColor::Red, false, -1.0f, 0,
	              2.0f);

	// If a wall is detected, tell PathfindingSubsystem
	if (bHitX150Plus && HitResult.GetActor())
	{
		if (PathfindingSubsystem)
		{
			// UProceduralMeshComponent
			UE_LOG(LogTemp, Warning, TEXT("bHitX150Plus = True"));
		}
	}
	if (bHitX150Minus && HitResult.GetActor())
	{
		if (PathfindingSubsystem)
		{
			// UProceduralMeshComponent
			UE_LOG(LogTemp, Warning, TEXT("bHitX150Minus = True"));
		}
	}
	if (bHitY150Plus && HitResult.GetActor())
	{
		if (PathfindingSubsystem)
		{
			// UProceduralMeshComponent
			UE_LOG(LogTemp, Warning, TEXT("bHitY150Plus = True"));
		}
	}
	if (bHitY150Minus && HitResult.GetActor())
	{
		if (PathfindingSubsystem)
		{
			// UProceduralMeshComponent
			UE_LOG(LogTemp, Warning, TEXT("bHitY150Minus = True"));
		}
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
	else if (Action->UniqueName == "Action_MoveTowards_GoldSphere")
	{
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
	else if (Action->UniqueName == "Action_MoveTowards_BlueCylinder")
	{
		FVector BlueCylinderLocation = FVector(2740, 1610, 430);
		bool bIsGoalMet = MoveTowards(BlueCylinderLocation);
		if (bIsGoalMet)
		{
			UE_LOG(LogTemp, Warning, TEXT("bIsGoalMet = MoveTowards(BlueCylinderLocation)"));

			// This updated Action state should now
			// prompt for AgentWorldState's Goal's review.
			Action->bIsCompleted = true;
		}
	}
	else if (Action->UniqueName == "Action_MoveTowards_GreenCone")
	{
		FVector GreenConeLocation = FVector(1660, 1220, 300);
		bool bIsGoalMet = MoveTowards(GreenConeLocation);
		if (bIsGoalMet)
		{
			UE_LOG(LogTemp, Warning, TEXT("bIsGoalMet = MoveTowards(GreenConeLocation)"));

			// This updated Action state should now
			// prompt for AgentWorldState's Goal's review.
			Action->bIsCompleted = true;
		}
	}
	else if (Action->UniqueName == "Action_MoveTowards_ThreeDifferentColours")
	{
		UE_LOG(LogTemp, Warning, TEXT("Action_MoveTowards_ThreeDifferentColours->bIsCompleted = true"));

		Action->bIsCompleted = true;
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


	if (FVector::DistSquared(DesiredLocation, NPCLocation) <= 62500.0f) // 2.5m
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

		for (const TPair<FString, bool>& ActionOutcome : CurrentQueuedAction->ActionOutcomes.StatesMap)
		{
			AgentWorldState.StatesMap.Emplace(ActionOutcome.Key, ActionOutcome.Value);
		}

		CurrentQueuedAction->bIsCompleted = false;
		CurrentQueuedAction = nullptr;
		return; // Halt execution for this frame to let world state settle
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
				UE_LOG(LogTemp, VeryVerbose, TEXT("No viable GOAP plan found. Wandering."));
				for (TObjectPtr<UAction> Action : AgentActions)
				{
					if (Action && Action->UniqueName == "Action_Wander")
					{
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

	// 4. Execute the current action if valid
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
	if (CurrentNodesPath.IsEmpty())
	{
		if (PathfindingSubsystem)
		{
			CurrentNodesPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
		}

		// 
		CurrentAgentAction->bIsCompleted = true;
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


	FVector NextLocation = CurrentNodesPath[0];
	FVector Direction = NextLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);

	if (FVector::DistSquared(NextLocation, GetActorLocation()) <= PathingLocationThreshold * PathingLocationThreshold)
	{
		CurrentNodesPath.RemoveAt(0);
		// UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentNodesPath.RemoveAt(0);"));

		if (CurrentNodesPath.IsEmpty())
		{
			CurrentAgentAction->bIsCompleted = true;
			UE_LOG(LogTemp, Warning, TEXT("AgentMoveAlongPath() CurrentAction->bIsCompleted = true;"));
		}
	}
}

void AAgentCharacter::ResetGoldBlueGreenGoals()
{
	UE_LOG(LogTemp, Warning, TEXT("AAgentCharacter::ResetGoldBlueGreenGoals()"));

	AgentWorldState.StatesMap.Emplace(TEXT("bHasBeenAtGoldSphere"), false);
	AgentWorldState.StatesMap.Emplace(TEXT("bHasBeenAtBlueCylinder"), false);
	AgentWorldState.StatesMap.Emplace(TEXT("bHasBeenAtGreenCone"), false);
}

void AAgentCharacter::InitBlueGreenGoals()
{
		// For MultiplayerMap.umap NPC examples (without rebuilding existing terrain)
	// using predetermined static targets (that aren't too high or low off ground)

	// Gold Sphere goal and action(s)
	AgentWorldState.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), false);

	TObjectPtr<UGoal> GoalBeAtGoldSphere = NewObject<UGoal>(this);
	GoalBeAtGoldSphere->GoalWorldState.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), true);
	GoalBeAtGoldSphere->GoalPriority = 2.0f;
	AgentGoals.Add(GoalBeAtGoldSphere);

	TObjectPtr<UAction> ActionMoveTowardsGoldSphere = NewObject<UAction>(this);
	ActionMoveTowardsGoldSphere->UniqueName = "Action_MoveTowards_GoldSphere";
	ActionMoveTowardsGoldSphere->ActionCost = 0.2;
	ActionMoveTowardsGoldSphere->ActionOutcomes.StatesMap.Add(TEXT("bHasBeenAtGoldSphere"), true);
	AgentActions.Add(ActionMoveTowardsGoldSphere);


	// Blue Cylinder
	AgentWorldState.StatesMap.Add(TEXT("bHasBeenAtBlueCylinder"), false);

	TObjectPtr<UGoal> GoalBeAtBlueCylinder = NewObject<UGoal>(this);
	GoalBeAtBlueCylinder->GoalWorldState.StatesMap.Add(TEXT("bHasBeenAtBlueCylinder"), true);
	GoalBeAtBlueCylinder->GoalPriority = 3.0f;
	AgentGoals.Add(GoalBeAtBlueCylinder);

	TObjectPtr<UAction> ActionMoveTowardsBlueCylinder = NewObject<UAction>(this);
	ActionMoveTowardsBlueCylinder->UniqueName = "Action_MoveTowards_BlueCylinder";
	ActionMoveTowardsBlueCylinder->ActionCost = 0.3;
	ActionMoveTowardsBlueCylinder->ActionOutcomes.StatesMap.Add(TEXT("bHasBeenAtBlueCylinder"), true);
	AgentActions.Add(ActionMoveTowardsBlueCylinder);


	// Green Cone
	AgentWorldState.StatesMap.Add(TEXT("bHasBeenAtGreenCone"), false);

	TObjectPtr<UGoal> GoalBeAtGreenCone = NewObject<UGoal>(this);
	GoalBeAtGreenCone->GoalWorldState.StatesMap.Add(TEXT("bHasBeenAtGreenCone"), true);
	GoalBeAtGreenCone->GoalPriority = 4.0f;
	AgentGoals.Add(GoalBeAtGreenCone);

	TObjectPtr<UAction> ActionMoveTowardsGreenCone = NewObject<UAction>(this);
	ActionMoveTowardsGreenCone->UniqueName = "Action_MoveTowards_GreenCone";
	ActionMoveTowardsGreenCone->ActionCost = 0.4;
	ActionMoveTowardsGreenCone->ActionOutcomes.StatesMap.Add(TEXT("bHasBeenAtGreenCone"), true);
	AgentActions.Add(ActionMoveTowardsGreenCone);
}