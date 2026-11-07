#include "ProceduralMazeFacility.h"

#include "MazeObstacleManager.h"

#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"


AProceduralMazeFacility::AProceduralMazeFacility()
{
	MazeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MazeMesh"));
	SetRootComponent(MazeMesh);

    WallMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WallMesh"));
    WallMesh->SetupAttachment(RootComponent);
    
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    bReplicates = true;
	// WallMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WallMesh"));
	// WallMesh->SetupAttachment(RootComponent);

	// PrimaryActorTick.bCanEverTick = true;
	// PrimaryActorTick.bStartWithTickEnabled = true;
}

void AProceduralMazeFacility::BeginPlay()
{
	Super::BeginPlay();
	
	
	// If we have valid cell data duplicated from the Editor, rebuild the visuals/systems
	// if (MazeCells.Num() > 0)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("PIE: Found duplicated layout. Rebuilding meshes..."));
 //        
	// 	// Clear old PIE instance components just in case
	// 	if (MazeMesh) MazeMesh->ClearMeshSection(0);
	// 	if (WallMesh) WallMesh->ClearMeshSection(0);
	//
	// 	// Build the geometry and pass data to pathfinding using the PRESERVED data
	// 	GenerateFloorGrid();
	// 	GenerateWallMeshes();
 //        
	// 	if (ObstacleManager)
	// 	{
	// 		// ObstacleManager->SpawnObstacles();
	// 		ObstacleManager->SpawnObstacles();
	//
	// 	}
	//
	// 	SpawnNavNodesAndPathing();
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility PIE: Found no layout. Rebuilding meshes..."));
	//
	// 	// Fallback if no data was generated in editor beforehand
	// 	// GenerateMaze();
	// }

}

void AProceduralMazeFacility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShouldRegenerate)
	{
		UE_LOG(LogTemp, Warning, TEXT("Maze regenerate triggered"));
		ClearMaze();
		GenerateMaze();
		bShouldRegenerate = false;
	}
}

void AProceduralMazeFacility::GenerateFloorGrid()
{
	if (MazeWidth < 2 || MazeHeight < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeWidth or MazeHeight too small!"));
		return;
	}

	if (!MazeMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeMesh is NULL!"));
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;

	// floor vertices
	for (int32 Y = 0; Y < MazeHeight; Y++)
	{
		for (int32 X = 0; X < MazeWidth; X++)
		{
			FVector Pos(X * CellSize, Y * CellSize, 0.0f);
			Vertices.Add(Pos);

			UVs.Add(FVector2D(
				(float)X / (MazeWidth - 1),
				(float)Y / (MazeHeight - 1)
			));
		}
	}

	// floor triangles
	for (int32 Y = 0; Y < MazeHeight - 1; Y++)
	{
		for (int32 X = 0; X < MazeWidth - 1; X++)
		{
			int32 BL = Y * MazeWidth + X;
			int32 BR = BL + 1;
			int32 TL = (Y + 1) * MazeWidth + X;
			int32 TR = TL + 1;

			Triangles.Add(BL);
			Triangles.Add(TL);
			Triangles.Add(BR);
			Triangles.Add(BR);
			Triangles.Add(TL);
			Triangles.Add(TR);
		}
	}

	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
		Vertices, Triangles, UVs, Normals, Tangents);

	MazeMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		TArray<FColor>(),
		Tangents,
		true
	);

	if (FloorMaterial)
		MazeMesh->SetMaterial(0, FloorMaterial);


	// // TESTING pathfinding NavNodes emplacement
	// // this one spawns inside the walls
	// if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	// {
	//     UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility UPathfindingSubsystem new nodes"));
	//     PathfindingSubsystem->
	//         MazePCGPlaceProceduralNodes(
	//             Vertices,
	//             MazeWidth,
	//             MazeHeight,
	//             this);
	// }
	// else
	// {
	//     UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility couldn't see UPathfindingSubsystem"));
	// }
}

void AProceduralMazeFacility::InitializeMazeCells()
{
	// turning the mesh vertex grid into a grid of cells (basically referencing the midpoint of any 4 vertices in a quad, not the vertices themselves...)
	CellsWidth = MazeWidth - 1;
	CellsHeight = MazeHeight - 1;

	MazeCells.SetNum(CellsHeight);
	for (int32 Y = 0; Y < CellsHeight; Y++)
	{
		MazeCells[Y].SetNum(CellsWidth);
		for (int32 X = 0; X < CellsWidth; X++)
		{
			MazeCells[Y][X] = FMazeCell();
		}
	}

	int32 MidCellY = CellsHeight / 2;

	StartCellX = 0;
	StartCellY = MidCellY;

	EndCellX = CellsWidth - 1;
	EndCellY = MidCellY;
	
// make sure the start and end cells are always open i.e. there's always an entry/exit point in the maze
	MazeCells[StartCellY][StartCellX].WallWest = false;
	MazeCells[EndCellY][EndCellX].WallEast = false;
}

void AProceduralMazeFacility::GenerateMaze()
{
	GenerateFloorGrid();
	InitializeMazeCells();

	//carves paths using DFS traversal
	CarveFrom(StartCellX, StartCellY);

	GenerateWallMeshes();

	if (ObstacleManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("obstacle manager exists"));
		ObstacleManager->SpawnObstacles(CurrentSeed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("no obstacle manager"));
	}
	



	
	// Custom pathfinding NavNodes emplacement - by karl
	SpawnNavNodesAndPathing();

	
}

void AProceduralMazeFacility::ClearMaze()
{
	if (MazeMesh)
		MazeMesh->ClearMeshSection(0);

	if (WallMesh)
		WallMesh->ClearMeshSection(0);

	MazeCells.SetNum(0);

	UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
}

bool AProceduralMazeFacility::IsInBounds(int32 CellX, int32 CellY) const
{
	return CellX >= 0 && CellX < CellsWidth &&
		CellY >= 0 && CellY < CellsHeight;
}

void AProceduralMazeFacility::CarveFrom(int32 CellX, int32 CellY)
{
	FMazeCell& Cell = MazeCells[CellY][CellX];
	Cell.Visited = true;
	// define directions which the search can go
	TArray<FIntPoint> Directions = {
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

    for (int32 i = 0; i < Directions.Num(); i++)
    {
        int32 SwapIndex = RandomStream.RandRange(0, Directions.Num() - 1);
        Directions.Swap(i, SwapIndex);
    }
	
	//the main traversal logic: pick random direction, if unvisited mark as visited, carve out wall and continue, 
	//if visited try new direction, if all neighbours are visited retreat until next unvisited cell
	for (const FIntPoint& Dir : Directions)
	{
		int32 NX = CellX + Dir.X;
		int32 NY = CellY + Dir.Y;

		if (!IsInBounds(NX, NY))
			continue;

		FMazeCell& Neighbor = MazeCells[NY][NX];
		if (Neighbor.Visited)
			continue;

		if (Dir.X == 1)
		{
			Cell.WallEast = false;
			Neighbor.WallWest = false;
		}
		else if (Dir.X == -1)
		{
			Cell.WallWest = false;
			Neighbor.WallEast = false;
		}
		else if (Dir.Y == 1)
		{
			Cell.WallNorth = false;
			Neighbor.WallSouth = false;
		}
		else if (Dir.Y == -1)
		{
			Cell.WallSouth = false;
			Neighbor.WallNorth = false;
		}

		CarveFrom(NX, NY);
	}
}

void AProceduralMazeFacility::GenerateWallMeshes()
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;

	const float WallHeight = 300.0f;

	auto AddWallQuad = [&](FVector A, FVector B)
	{
		FVector Up(0, 0, WallHeight);

		int32 IndexStart = Vertices.Num();

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(B + Up);
		Vertices.Add(A + Up);

		Triangles.Add(IndexStart + 0);
		Triangles.Add(IndexStart + 1);
		Triangles.Add(IndexStart + 2);

		Triangles.Add(IndexStart + 0);
		Triangles.Add(IndexStart + 2);
		Triangles.Add(IndexStart + 3);

		UVs.Add(FVector2D(0, 0));
		UVs.Add(FVector2D(1, 0));
		UVs.Add(FVector2D(1, 1));
		UVs.Add(FVector2D(0, 1));
	};
	
	//create maze based on cells with walls remaining
	for (int32 Y = 0; Y < CellsHeight; Y++)
	{
		for (int32 X = 0; X < CellsWidth; X++)
		{
			const FMazeCell& Cell = MazeCells[Y][X];

			float X0 = X * CellSize;
			float X1 = (X + 1) * CellSize;
			float Y0 = Y * CellSize;
			float Y1 = (Y + 1) * CellSize;
	
			if (Cell.WallWest)
				AddWallQuad(FVector(X0, Y0, 0), FVector(X0, Y1, 0));

			if (Cell.WallEast)
				AddWallQuad(FVector(X1, Y0, 0), FVector(X1, Y1, 0));

			if (Cell.WallSouth)
				AddWallQuad(FVector(X0, Y0, 0), FVector(X1, Y0, 0));

			if (Cell.WallNorth)
				AddWallQuad(FVector(X0, Y1, 0), FVector(X1, Y1, 0));
		}
	}

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
		Vertices, Triangles, UVs, Normals, Tangents);

	WallMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		TArray<FColor>(),
		Tangents,
		true
	);

	if (WallMaterial)
		WallMesh->SetMaterial(0, WallMaterial);



}

// Custom pathfinding NavNodes emplacement
void AProceduralMazeFacility::SpawnNavNodesAndPathing()
{
	TArray<FVector> CellVertices;
	
	for (int32 Y = 0; Y < CellsHeight; Y++)
	{
		for (int32 X = 0; X < CellsWidth; X++)
		{

			CellVertices.Add(GetCellWorldPosition(X,Y));
		}
	}


	int32 MidCellY = CellsHeight / 2;

	int32 CellStartX = 0;
	int32 CellStartY = MidCellY;

	int32 CellEndX = CellsWidth - 1;
	int32 CellEndY = MidCellY;

	FVector MazeStartPosition = GetCellWorldPosition(CellStartX, CellStartY);
	FVector MazeEndPosition = GetCellWorldPosition(CellEndX, CellEndY);
	
	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility::SpawnNavNodesAndPathing MazeStartPosition: %s"), *MazeStartPosition.ToString());
	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility::SpawnNavNodesAndPathing MazeEndPosition: %s"), *MazeEndPosition.ToString());



	if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	{
	    UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility UPathfindingSubsystem new nodes"));
	    PathfindingSubsystem->
	        MazePCGPlaceProceduralNodes(
	            CellVertices,
	            CellsWidth,
	            CellsHeight,
	            this,
	            MazeCells,
	            MazeStartPosition,
	            MazeEndPosition
	            );
	}
	else
	{
	    UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility couldn't see UPathfindingSubsystem"));
	}

}


// ----------MULTIPLAYER REPLICATION--------
// this section has all of the parts for a server remote procedure call to generate the maze at runtime
// all 'randomness' was replaced by a randomstream using a seed that is shared by all clients + server
// on the UI side, multicastregeneratemaze is called from the playercontroller, which is then passed on
// to the gamemodebase to reflect onto all clients.

FVector AProceduralMazeFacility::GetCellWorldPosition(int32 CellX, int32 CellY) const
{
	return GetActorLocation() + FVector(
		(CellX + 0.5f) * CellSize,
		(CellY + 0.5f) * CellSize,
		0.0f
	);
}


void AProceduralMazeFacility::MulticastRegenerateMaze_Implementation(
    int32 Seed,
    int32 InSpills,
    int32 InDistractions)
{
    if (ObstacleManager)
    {
        ObstacleManager->NumberOfSpills =
            InSpills;

        ObstacleManager->NumberOfDistractions =
            InDistractions;
    }

    GenerateMazeFromSeed(Seed);
}

void AProceduralMazeFacility::GenerateMazeFromSeed(
    int32 Seed)
{
	CurrentSeed = Seed;

	RandomStream.Initialize(Seed);

	ClearMaze();
	GenerateMaze();
}
bool AProceduralMazeFacility::ShouldTickIfViewportsOnly() const
{
	return true;
}

//some helpers
FVector AProceduralMazeFacility::GetMazeStartCellWorldPosition() 
{
	FVector MazeStartPosition = GetCellWorldPosition(StartCellX, StartCellY);
	
	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility MazeStartPosition %s"), *MazeStartPosition.ToString());
	
	return MazeStartPosition;
}

FVector AProceduralMazeFacility::GetMazeEndCellWorldPosition() 
{
	FVector MazeEndPosition = GetCellWorldPosition(EndCellX, EndCellY);

	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility MazeEndPosition %s"), *MazeEndPosition.ToString());

	return MazeEndPosition;
}