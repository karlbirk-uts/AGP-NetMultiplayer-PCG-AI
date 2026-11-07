/**
* AGP class subsystem example originally.
 * Now with PCGMaze edits to customise NavigationNode connectedness
 * in cardinal directions.
 * This connectedness is used by NPC to sense the PCGMaze, cell by cell
 * 
 * Author: Karl Birk
 * UTS AGP 2026 Group 10
 */

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#pragma once

#include "CoreMinimal.h"
#include "AGP/Landscape/ProceduralMazeFacility.h"
#include "Subsystems/WorldSubsystem.h"
#include "PathfindingSubsystem.generated.h"

struct FAStarNodeData;
class ANavigationNode;

UCLASS()
class AGP_API UPathfindingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FVector MazePCGStartNodePosition = FVector(0.0f, 0.0f, 0.0f);
	FVector MazePCGEndNodePosition = FVector(0.0f, 0.0f, 0.0f);

	virtual void OnWorldBeginPlay(UWorld& World) override;

	TArray<FVector> GetRandomPath(const FVector& StartLocation);

	TArray<FVector> GetPath(const FVector& StartLocation, const FVector& TargetLocation);
	TArray<FVector> MazeGetPath(const FVector& StartLocation, const FVector& TargetLocation);

	TArray<FVector> GetPathAway(const FVector& StartLocation, const FVector& TargetLocation);

	void PlaceProceduralNodes(
		const TArray<FVector>& Vertices, 
		const int32 Width, 
		const int32 Height,
		AActor* ParentActor);
	
	void MazePCGPlaceProceduralNodes(
		const TArray<FVector>& VerticalWallsVertices,
		const int32 Width,
		const int32 Height,
		AActor* ParentActor,
		const TArray<TArray<FMazeCell>>& MazeCells,
		const FVector& MazeStartPosition,
		const FVector& MazeEndPosition);
	TArray<FVector> MazeGetPath(ANavigationNode* StartNode, ANavigationNode* EndNode);


	TArray<FVector> GetWaypointPositions();

protected:
	UPROPERTY()
	TArray<ANavigationNode*> NavNodes;

protected:
	void PopulateNodes();
	void RemoveAllNodes();

private:
	ANavigationNode* GetRandomNode();

	ANavigationNode* FindFurthestNode(const FVector& TargetLocation);

	TArray<FVector> GetPath(ANavigationNode* StartNode, ANavigationNode* EndNode);

	TArray<FVector> ReconstructPath(TMap<ANavigationNode*, FAStarNodeData*> NodeMap, const ANavigationNode* EndNode);

	void DrawDebugPath(TArray<FVector> Path) const;
	
public:
	ANavigationNode* FindNearestNode(const FVector& TargetLocation);

};
