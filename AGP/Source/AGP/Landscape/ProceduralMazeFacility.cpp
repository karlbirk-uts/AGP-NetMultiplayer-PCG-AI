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
}

void AProceduralMazeFacility::BeginPlay()
{
	Super::BeginPlay();
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


	// TESTING pathfinding NavNodes emplacement
	// this one spawns inside the walls
	if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	{
	    UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility UPathfindingSubsystem new nodes"));
	    PathfindingSubsystem->
	        MazePCGPlaceProceduralNodes(
	            Vertices,
	            MazeWidth,
	            MazeHeight,
	            this);
	}
	else
	{
	    UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility couldn't see UPathfindingSubsystem"));
	}
	

	// Spawning navnodes in between walls ground vertices
	// TArray<FVector> CentredNavNodes;
	// CentredNavNodes.Reserve((MazeWidth - 1) * (MazeHeight - 1));
	//
	// for (int32 i = 0; i < MazeHeight - 1; i++)
	// {
	// 	for (int32 j = 0; j < MazeWidth - 1; j++)
	// 	{
	// 		int32 SW = i * MazeWidth + j;
	// 		int32 SE = SW + 1;
	// 		int32 NW = (i + 1) * MazeWidth + j;
	// 		int32 NE = NW + 1;
	//
	// 		// Middle
	// 		FVector CenterPos =
	// 			(Vertices[SW] +
	// 				Vertices[SE] +
	// 				Vertices[NW] +
	// 				Vertices[NE])
	// 			* 0.25f;
	//
	// 		CentredNavNodes.Add(CenterPos);
	// 	}
	// }
	

	// UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility GenerateFloorGrid Vertices.Num(): %d"), Vertices.Num());
	// UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility GenerateFloorGrid CentredNavNodes.Num(): %d"), CentredNavNodes.Num());

	
	// TESTING Centred+Vertices
	// TArray<FVector> CentredAndVerticesNavNodes;
	// CentredAndVerticesNavNodes.Reserve(Vertices.Num()+CentredNavNodes.Num());
	//
	//
	//
	// CentredAndVerticesNavNodes.Append(Vertices);
	// CentredAndVerticesNavNodes.Append(CentredNavNodes);
	//
	// if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	// {
	// 	PathfindingSubsystem->PlaceProceduralNodes(
	// 		CentredNavNodes,
	// 		MazeWidth - 1,
	// 		MazeHeight - 1,
	// 		this
	// 	);
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("AProceduralMazeFacility couldn't see UPathfindingSubsystem"));
	// }
}

void AProceduralMazeFacility::InitializeMazeCells()
{
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

	MazeCells[StartCellY][StartCellX].WallWest = false;
	MazeCells[EndCellY][EndCellX].WallEast = false;
}

void AProceduralMazeFacility::GenerateMaze()
{
	GenerateFloorGrid();
	InitializeMazeCells();

	CarveFrom(StartCellX, StartCellY);

	GenerateWallMeshes();

	if (ObstacleManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("obstacle manager exists"));
		ObstacleManager->SpawnObstacles();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("no obstacle manager"));
	}
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

	TArray<FIntPoint> Directions = {
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (int32 i = 0; i < Directions.Num(); i++)
	{
		int32 SwapIndex = FMath::RandRange(0, Directions.Num() - 1);
		Directions.Swap(i, SwapIndex);
	}

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


	// if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	// {
	//     PathfindingSubsystem->
	//         PlaceProceduralNodes(
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

FVector AProceduralMazeFacility::GetCellWorldPosition(int32 CellX, int32 CellY) const
{
	return GetActorLocation() + FVector(
		(CellX + 0.5f) * CellSize,
		(CellY + 0.5f) * CellSize,
		0.0f
	);
}

bool AProceduralMazeFacility::ShouldTickIfViewportsOnly() const
{
	return true;
}
