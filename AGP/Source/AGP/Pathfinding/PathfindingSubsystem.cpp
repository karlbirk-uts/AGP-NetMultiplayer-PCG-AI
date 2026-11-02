// Fill out your copyright notice in the Description page of Project Settings.


#include "PathfindingSubsystem.h"
#include "NavigationNode.h"
#include "EngineUtils.h"
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
			if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>(Position, FRotator::ZeroRotator))
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

			/*
			// UE_LOG(LogTemp, Display, TEXT("Node at : %s"), *Node->GetActorLocation().ToString());
			*/
			// WILL ALWAYS PRINT.
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


void UPathfindingSubsystem::PlaceProceduralNodes5(
	const TArray<FVector>& VerticalWallsVertices,
	const int32 Width,
	const int32 Height,
	AActor* ParentActor)
{
	// Clear existing tracking arrays
	RemoveAllNodes();

	
	// VerticalWallsVertices are locations of impassable walls
	// CentredNavNodeVertices are for pathways
	
	
	// Calculate centred NavNode positions
	TArray<FVector> CentredNavNodeVertices;
	const int32 CentreWidth = Width - 1;
	const int32 CentreHeight = Height - 1;
	CentredNavNodeVertices.Reserve(CentreWidth * CentreHeight);

	for (int32 i = 0; i < CentreHeight; i++)
	{
		for (int32 j = 0; j < CentreWidth; j++)
		{
			int32 SW = i * Width + j;
			int32 SE = SW + 1;
			int32 NW = (i + 1) * Width + j;
			int32 NE = NW + 1;

			FVector CenterPos =
				(
					VerticalWallsVertices[SW] +
					VerticalWallsVertices[SE] +
					VerticalWallsVertices[NW] +
					VerticalWallsVertices[NE])
				* 0.25f;
			CentredNavNodeVertices.Add(CenterPos);
		}
	}
	

	
	
	
	// Spawn VerticalWallsVertices Nodes
	TArray<ANavigationNode*> VertexNodes;
	VertexNodes.SetNumZeroed(Width * Height);

	for (int32 j = 0; j < Height; ++j)
	{
		for (int32 i = 0; i < Width; ++i)
		{
			FVector Position = VerticalWallsVertices[i + j * Width];
			if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>(Position, FRotator::ZeroRotator))
			{
				VertexNodes[i + j * Width] = Node;
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

	// Spawn CentredNavNodeVertices NavNodes
	TArray<ANavigationNode*> CentreNodes;
	CentreNodes.SetNumZeroed(CentreWidth * CentreHeight);

	for (int32 j = 0; j < CentreHeight; ++j)
	{
		for (int32 i = 0; i < CentreWidth; ++i)
		{
			FVector Position = CentredNavNodeVertices[i + j * CentreWidth];
			
			if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>(Position, FRotator::ZeroRotator))
			{
				NavNodes.Add(Node);
				CentreNodes.Add(Node);
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

	// 5. Connect Vertex Nodes to their structural Vertex neighbours (8-way grid)
	for (int32 j = 0; j < Height; ++j)
	{
		for (int32 i = 0; i < Width; ++i)
		{
			ANavigationNode* CurrentNode = VertexNodes[i + j * Width];
			if (!CurrentNode) continue;

			const bool bAddLeft = i < Width - 1;
			const bool bAddRight = i > 0;
			const bool bAddUp = j < Height - 1;
			const bool bAddDown = j > 0;

			if (bAddUp) CurrentNode->AddConnectedNode(VertexNodes[i + (j + 1) * Width]);
			if (bAddDown) CurrentNode->AddConnectedNode(VertexNodes[i + (j - 1) * Width]);
			if (bAddLeft) CurrentNode->AddConnectedNode(VertexNodes[(i + 1) + j * Width]);
			if (bAddRight) CurrentNode->AddConnectedNode(VertexNodes[(i - 1) + j * Width]);
			if (bAddUp && bAddRight) CurrentNode->AddConnectedNode(VertexNodes[(i - 1) + (j + 1) * Width]);
			if (bAddUp && bAddLeft) CurrentNode->AddConnectedNode(VertexNodes[(i + 1) + (j + 1) * Width]);
			if (bAddDown && bAddRight) CurrentNode->AddConnectedNode(VertexNodes[(i - 1) + (j - 1) * Width]);
			if (bAddDown && bAddLeft) CurrentNode->AddConnectedNode(VertexNodes[(i + 1) + (j - 1) * Width]);
		}
	}

	// 6. Cross-Connect Centre Nodes to their 4 surrounding Corner Nodes
	for (int32 j = 0; j < CentreHeight; ++j)
	{
		for (int32 i = 0; i < CentreWidth; ++i)
		{
			ANavigationNode* CenterNode = CentreNodes[i + j * CentreWidth];
			if (!CenterNode) continue;

			// Map centre cell index to its 4 corner vertex coordinates
			int32 SW_Idx = i + j * Width;
			int32 SE_Idx = (i + 1) + j * Width;
			int32 NW_Idx = i + (j + 1) * Width;
			int32 NE_Idx = (i + 1) + (j + 1) * Width;

			// Connect centre node outward to corners, and corners back inward to centre
			if (ANavigationNode* SW = VertexNodes[SW_Idx])
			{
				CenterNode->AddConnectedNode(SW);
				SW->AddConnectedNode(CenterNode);
			}
			if (ANavigationNode* SE = VertexNodes[SE_Idx])
			{
				CenterNode->AddConnectedNode(SE);
				SE->AddConnectedNode(CenterNode);
			}
			if (ANavigationNode* NW = VertexNodes[NW_Idx])
			{
				CenterNode->AddConnectedNode(NW);
				NW->AddConnectedNode(CenterNode);
			}
			if (ANavigationNode* NE = VertexNodes[NE_Idx])
			{
				CenterNode->AddConnectedNode(NE);
				NE->AddConnectedNode(CenterNode);
			}
		}
	}
}
