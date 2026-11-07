/**
 * AGP class subsystem example originally.
 * Now with PCGMaze edits to customise NavigationNode connectedness
 * in cardinal directions.
 * This connectedness is used by NPC to sense the PCGMaze, cell by cell
 * 
 * Author: Karl Birk
 * UTS AGP 2026 Group 10
 */

#include "PathfindingSubsystem.h"
#include "NavigationNode.h"
#include "EngineUtils.h"
#include "AGP/Landscape/ProceduralMazeFacility.h"
#include "Algo/Reverse.h"

USTRUCT()
struct FAStarNodeData
{
	float GScore = MAX_FLT;
	float HScore = 0;
	float GetFScore() const { return GScore + HScore; }

	UPROPERTY()
	ANavigationNode* ParentNode = nullptr;
};

void UPathfindingSubsystem::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		PopulateNodes();
	}
}

TArray<FVector> UPathfindingSubsystem::GetRandomPath(const FVector& StartLocation)
{
	return GetPath(FindNearestNode(StartLocation), GetRandomNode());
}


TArray<FVector> UPathfindingSubsystem::GetPath(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindNearestNode(TargetLocation));
}

TArray<FVector> UPathfindingSubsystem::MazeGetPath(const FVector& StartLocation, const FVector& TargetLocation)
{
	return MazeGetPath(FindNearestNode(StartLocation), FindNearestNode(TargetLocation));
}



TArray<FVector> UPathfindingSubsystem::GetPathAway(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindFurthestNode(TargetLocation));
}

void UPathfindingSubsystem::PlaceProceduralNodes(
	const TArray<FVector>& Vertices,
	const int32 Width,
	const int32 Height,
	AActor* ParentActor)
{
	//Clear nodes
	RemoveAllNodes();


	// SPAWN NODES; ADD TO NavNodes array.
	for (int i = 0; i < Width; ++i)
	{
		for (int j = 0; j < Height; ++j)
		{
			FVector Position = Vertices[i + j * Width];
			if (ANavigationNode* Node =
				GetWorld()->
				SpawnActor<ANavigationNode>(
					Position,
					FRotator::ZeroRotator))
			{
				NavNodes.Add(Node);

				// Attach to supplied parent actor to nest nodes to mesh and keep them self-contained.
				if (ParentActor)
				{
					Node->AttachToActor(ParentActor, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Unable to spawn a node for some reason. This is bad!"))
			}
		}
	}

	// Connect Nodes to their neighbours.
	for (int i = 0; i < Width; ++i)
	{
		for (int j = 0; j < Height; ++j)
		{
			ANavigationNode* CurrentNode = NavNodes[i + j * Width];

			if (!CurrentNode) continue;

			// conditions for valid neighbours.
			const bool bAddLeft = i < Width - 1;
			const bool bAddRight = i > 0;
			const bool bAddUp = j < Height - 1;
			const bool bAddDown = j > 0;

			if (bAddUp)
			{
				CurrentNode->AddConnectedNode(NavNodes[i + (j + 1) * Width]);
			}
			if (bAddDown)
			{
				CurrentNode->AddConnectedNode(NavNodes[i + (j - 1) * Width]);
			}
			if (bAddLeft)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i + 1) + j * Width]);
			}
			if (bAddRight)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i - 1) + j * Width]);
			}
			if (bAddUp && bAddRight)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i - 1) + (j + 1) * Width]);
			}
			if (bAddUp && bAddLeft)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i + 1) + (j + 1) * Width]);
			}
			if (bAddDown && bAddRight)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i - 1) + (j - 1) * Width]);
			}
			if (bAddDown && bAddLeft)
			{
				CurrentNode->AddConnectedNode(NavNodes[(i + 1) + (j - 1) * Width]);
			}
		}
	}
}

TArray<FVector> UPathfindingSubsystem::GetWaypointPositions()
{
	TArray<FVector> WaypointPositions;
	for (const ANavigationNode* Node : NavNodes)
	{
		if (!Node) continue;
		WaypointPositions.Add(Node->GetActorLocation());
	}
	return WaypointPositions;
}

void UPathfindingSubsystem::PopulateNodes()
{
	NavNodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		if (ANavigationNode* Node = *It)
		{
			NavNodes.Add(Node);
		}
	}
}

void UPathfindingSubsystem::RemoveAllNodes()
{
	// Destroy all Nodes in the world.
	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		if (ANavigationNode* Node = *It)
		{
			Node->Destroy(true);
		}
	}
	// Clear out the list of nodes as pointers are now null.
	NavNodes.Empty();
}

ANavigationNode* UPathfindingSubsystem::GetRandomNode()
{
	if (NavNodes.IsEmpty())
	{
		return nullptr;
	}

	return NavNodes[FMath::RandRange(0, NavNodes.Num() - 1)];
}

ANavigationNode* UPathfindingSubsystem::FindNearestNode(const FVector& TargetLocation)
{
	if (NavNodes.IsEmpty())
	{
		return nullptr;
	}

	float ClosestDistance = FLT_MAX;
	ANavigationNode* ClosestNode = nullptr;

	for (ANavigationNode* Node : NavNodes)
	{
		if (!Node) continue;
		if (const float Distance = FVector::Distance(Node->GetActorLocation(), TargetLocation); Distance <
			ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestNode = Node;
		}
	}
	return ClosestNode;
}

ANavigationNode* UPathfindingSubsystem::FindFurthestNode(const FVector& TargetLocation)
{
	if (NavNodes.IsEmpty())
	{
		return nullptr;
	}

	float FurthestDistance = FLT_MIN;
	ANavigationNode* FurthestNode = nullptr;

	for (ANavigationNode* Node : NavNodes)
	{
		if (!Node) continue;
		if (const float Distance = FVector::Distance(Node->GetActorLocation(), TargetLocation); Distance >
			FurthestDistance)
		{
			FurthestDistance = Distance;
			FurthestNode = Node;
		}
	}
	return FurthestNode;
}

TArray<FVector> UPathfindingSubsystem::GetPath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	if (!StartNode || !EndNode)
	{
		return TArray<FVector>();
	}

	TArray<ANavigationNode*> OpenSet;
	TMap<ANavigationNode*, FAStarNodeData*> NodeMap;

	for (ANavigationNode* Node : NavNodes)
	{
		if (!Node) continue;
		NodeMap.Add(Node, new FAStarNodeData());
	}

	if (!NodeMap.Contains(StartNode) || !NodeMap.Contains(EndNode))
	{
		return TArray<FVector>();
	}

	NodeMap[StartNode]->GScore = 0;
	NodeMap[StartNode]->HScore = FVector::DistSquared(StartNode->GetActorLocation(), EndNode->GetActorLocation());

	OpenSet.Add(StartNode);

	while (!OpenSet.IsEmpty())
	{
		// Get the lowest-score node (most promising) to search around.
		// This process could be more efficient using a Min-Heap approach to OpenSet, but it is more difficult.
		float LowestFScore = FLT_MAX;
		ANavigationNode* CurrentNode = nullptr;

		for (ANavigationNode* Node : OpenSet)
		{
			if (!Node) continue;
			if (NodeMap[Node]->GetFScore() < LowestFScore)
			{
				LowestFScore = NodeMap[Node]->GetFScore();
				CurrentNode = Node;
			}
		}

		if (!CurrentNode)
		{
			UE_LOG(LogTemp, Error, TEXT("No valid current node found in non-empty open set."))
			return TArray<FVector>();
		}

		OpenSet.Remove(CurrentNode);

		// return the path as we have reached the end-node!
		if (CurrentNode == EndNode)
		{
			return ReconstructPath(NodeMap, EndNode);
		}

		// Update values of surrounding nodes, and if improved, add to open set. 
		for (ANavigationNode* ConnectedNode : CurrentNode->GetConnectedNodes())
		{
			if (!ConnectedNode) continue;


			// Skip if either the target node or current node is blocked
			if (ConnectedNode->IsBlocked() || CurrentNode->IsBlocked()) continue;


			const float TentativeConnectedNodeGScore = NodeMap[CurrentNode]->GScore +
				FVector::DistSquared(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());

			if (TentativeConnectedNodeGScore < NodeMap[ConnectedNode]->GScore)
			{
				NodeMap[ConnectedNode]->ParentNode = CurrentNode;
				NodeMap[ConnectedNode]->GScore = TentativeConnectedNodeGScore;
				NodeMap[ConnectedNode]->HScore = FVector::DistSquared(ConnectedNode->GetActorLocation(),
				                                                      EndNode->GetActorLocation());

				if (!OpenSet.Contains(ConnectedNode))
				{
					OpenSet.Add(ConnectedNode);
				}
			}
		}
	}

	// If it all fails, return no path.
	UE_LOG(LogTemp, Error, TEXT("No valid path found!"))
	return TArray<FVector>();
}


void UPathfindingSubsystem::DrawDebugPath(TArray<FVector> Path) const
{
	if (!Path.IsEmpty())
	{
		DrawDebugBox(GetWorld(), Path[0], FVector(50, 50, 50), FColor::Emerald, false, 5, 0, 5);
		DrawDebugBox(GetWorld(), Path.Top(), FVector(50, 50, 75), FColor::Green, false, 5, 0, 5);

		FVector LastPosition = Path[0];
		for (FVector Position : Path)
		{
			if (Position != LastPosition)
			{
				DrawDebugLine(GetWorld(), LastPosition, Position, FColor::Green, false, 5, 0, 5);
				DrawDebugDirectionalArrow(GetWorld(), LastPosition, Position, 1000, FColor::Green, false, 5, 0, 5);
			}
			LastPosition = Position;
		}
	}
}

// Reconstruct and reverse path 
TArray<FVector> UPathfindingSubsystem::ReconstructPath(TMap<ANavigationNode*, FAStarNodeData*> NodeMap,
                                                       const ANavigationNode* EndNode)
{
	TArray<FVector> Path = TArray<FVector>();
	Path.Add(EndNode->GetActorLocation());
	const ANavigationNode* NextNode = NodeMap[EndNode]->ParentNode;
	while (NextNode)
	{
		Path.Add(NextNode->GetActorLocation());
		NextNode = NodeMap[NextNode]->ParentNode;
	}

	// Reverse Path as currently end --> start.
	Algo::Reverse(Path);

	DrawDebugPath(Path);

	return Path;
}


/**
 * Spawn navigationnodes with PCGMaze data.
 * @param CellVertices 
 * @param CellsWidth 
 * @param CellsHeight 
 * @param ParentActor 
 * @param MazeCells 
 * @param MazeStartPosition 
 * @param MazeEndPosition 
 */
void UPathfindingSubsystem::MazePCGPlaceProceduralNodes(
	const TArray<FVector>& CellVertices,
	const int32 CellsWidth,
	const int32 CellsHeight,
	AActor* ParentActor,
	const TArray<TArray<FMazeCell>>& MazeCells,
	const FVector& MazeStartPosition,
	const FVector& MazeEndPosition)
{
	// Clear existing tracking arrays
	RemoveAllNodes();

	// Starting positions for NPCs later
	// EDIT: "Unreal Engine runs multiple worlds simultaneously in the editor:
	// • Editor World (EWorldType::Editor): 
	// This is the world you see when modifying levels, placing actors, and 
	// where your editor-ticking actor is updating the variable.
	// • PIE World (EWorldType::PIE): 
	// This is a brand-new, isolated world created the moment you click "Play.""
	MazePCGStartNodePosition = MazeStartPosition;
	MazePCGEndNodePosition = MazeEndPosition;


	// Spawning nodes
	for (int32 row = 0; row < CellsHeight; ++row)
	{
		for (int32 column = 0; column < CellsWidth; ++column)
		{
			int32 Index = column + (row * CellsWidth); // Matches maze grid
			// int32 Index = row + (column * CellsWidth); // Flipped diagonally


			if (!CellVertices.IsValidIndex(Index))
			{
				continue;
			}

			FVector Position = CellVertices[Index];

			if (ANavigationNode* Node =
				GetWorld()->
				SpawnActor<ANavigationNode>(
					Position,
					FRotator::ZeroRotator))
			{
				NavNodes.Add(Node);

				// Attach to supplied parent actor to nest nodes to mesh and keep them self-contained.
				if (ParentActor)
				{
					Node->AttachToActor(ParentActor, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
			else
			{
				// Unable to spawn a node.
				UE_LOG(LogTemp, Error,
				       TEXT("UPathfindingSubsystem::MazePCGPlaceProceduralNodes Failed to spawn NavNode"));
			}
		}
	}


	// Connect nodes to neighbours, NESW, where MazeCells walls can prevent connections
	for (int row = 0; row < CellsWidth; ++row)
	{
		for (int column = 0; column < CellsHeight; ++column)
		{
			int32 Index = column + (row * CellsWidth); // Matches maze grid
			// int32 Index = row + (column * CellsWidth); // Flipped diagonally

			if (!CellVertices.IsValidIndex(Index)) continue;

			ANavigationNode* CurrentNode = NavNodes[Index];

			if (!CurrentNode) continue;

			bool bIsWallNorth = MazeCells[row][column].WallNorth;
			bool bIsWallEast = MazeCells[row][column].WallEast;
			bool bIsWallSouth = MazeCells[row][column].WallSouth;
			bool bIsWallWest = MazeCells[row][column].WallWest;


			// conditions for valid neighbours.
			const bool bAddNorth = row < CellsHeight - 1;
			const bool bAddSouth = row > 0;
			const bool bAddEast = column < CellsHeight - 1;
			const bool bAddWest = column > 0;


			bool bShouldConnectNorth = bAddNorth && !bIsWallNorth;
			bool bShouldConnectSouth = bAddSouth && !bIsWallSouth;
			bool bShouldConnectEast = bAddEast && !bIsWallEast;
			bool bShouldConnectWest = bAddWest && !bIsWallWest;


			if (bShouldConnectNorth)
			{
				int32 NorthNeighbourIndex = column + ((row + 1) * CellsWidth);
				if (NavNodes.IsValidIndex(NorthNeighbourIndex))
				{
					CurrentNode->AddConnectedNode(NavNodes[NorthNeighbourIndex]);
				}
			}

			if (bShouldConnectSouth)
			{
				int32 SouthNeighbourIndex = column + ((row - 1) * CellsWidth);
				if (NavNodes.IsValidIndex(SouthNeighbourIndex))
				{
					CurrentNode->AddConnectedNode(NavNodes[SouthNeighbourIndex]);
				}
			}

			if (bShouldConnectEast)
			{
				int32 EastNeighbourIndex = (column + 1) + (row * CellsWidth);
				if (NavNodes.IsValidIndex(EastNeighbourIndex))
				{
					CurrentNode->AddConnectedNode(NavNodes[EastNeighbourIndex]);
				}
			}

			if (bShouldConnectWest)
			{
				int32 WestNeighbourIndex = (column - 1) + (row * CellsWidth);
				if (NavNodes.IsValidIndex(WestNeighbourIndex))
				{
					CurrentNode->AddConnectedNode(NavNodes[WestNeighbourIndex]);
				}
			}


			int32 ConnectedCount = CurrentNode->GetConnectedNodes().Num();

			// UE_LOG(
			// 	LogTemp,
			// 	Warning,
			// 	TEXT("UPathfindingSubsystem NavigationNode ConnectedCount: %d"),
			// 	ConnectedCount);


			// Cardinal corners will be classified as junction
			CurrentNode->bIsJunction =
				(bShouldConnectEast || bShouldConnectWest)
				&&
				(bShouldConnectNorth || bShouldConnectSouth);

			// Deadends with 1 or less connections
			CurrentNode->bIsDeadend = ConnectedCount <= 1;
		}
	}
}


// Reducing desire to revisit same navigation nodes
TArray<FVector> UPathfindingSubsystem::MazeGetPath(ANavigationNode* StartNode, ANavigationNode* EndNode) {
    if (!StartNode || !EndNode) {
        return TArray<FVector>();
    }

    TArray<ANavigationNode*> OpenSet;
    TMap<ANavigationNode*, FAStarNodeData*> NodeMap;

    for (ANavigationNode* Node : NavNodes) {
        if (!Node) continue;
        NodeMap.Add(Node, new FAStarNodeData());
    }

    if (!NodeMap.Contains(StartNode) || !NodeMap.Contains(EndNode)) {
        return TArray<FVector>();
    }

    NodeMap[StartNode]->GScore = 0;
    NodeMap[StartNode]->HScore = FVector::DistSquared(StartNode->GetActorLocation(), EndNode->GetActorLocation());
    OpenSet.Add(StartNode);

    while (!OpenSet.IsEmpty()) {
        float LowestFScore = FLT_MAX;
        ANavigationNode* CurrentNode = nullptr;

        for (ANavigationNode* Node : OpenSet) {
            if (!Node) continue;
            if (NodeMap[Node]->GetFScore() < LowestFScore) {
                LowestFScore = NodeMap[Node]->GetFScore();
                CurrentNode = Node;
            }
        }

        if (!CurrentNode) {
            UE_LOG(LogTemp, Error, TEXT("No valid current node found in non-empty open set."))
            return TArray<FVector>();
        }

        OpenSet.Remove(CurrentNode);

        if (CurrentNode == EndNode) {
            return ReconstructPath(NodeMap, EndNode);
        }

        for (ANavigationNode* ConnectedNode : CurrentNode->GetConnectedNodes()) {
            if (!ConnectedNode) continue;
            if (ConnectedNode->IsBlocked() || CurrentNode->IsBlocked()) continue;

            // Calculate the base distance cost (using DistSquared to match your implementation)
            float DistanceCost = FVector::DistSquared(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());

            // NPC traffic penalty
            // Adjust the multiplier (e.g., 10000.f) depending on how strongly you want to avoid NPCs.
            // Note: Since you use DistSquared, your physical distance costs are very large numbers. 
            // Your penalty must be proportionally large to have a noticeable impact.
            float TrafficPenalty = ConnectedNode->NPCVisitedCounter * 100000.f; 

            // 3. Combine them into the final cost for this step
            const float TentativeConnectedNodeGScore = NodeMap[CurrentNode]->GScore + DistanceCost + TrafficPenalty;

            if (TentativeConnectedNodeGScore < NodeMap[ConnectedNode]->GScore) {
                NodeMap[ConnectedNode]->ParentNode = CurrentNode;
                NodeMap[ConnectedNode]->GScore = TentativeConnectedNodeGScore;
                NodeMap[ConnectedNode]->HScore = FVector::DistSquared(ConnectedNode->GetActorLocation(), EndNode->GetActorLocation());

                if (!OpenSet.Contains(ConnectedNode)) {
                    OpenSet.Add(ConnectedNode);
                }
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("No valid path found!"))
    return TArray<FVector>();
}
