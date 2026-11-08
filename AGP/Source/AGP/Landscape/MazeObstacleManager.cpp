#include "MazeObstacleManager.h"
#include "Containers/Queue.h"
#include "ObstacleBase.h"
#include "ProceduralMazeFacility.h"
#include "Materials/MaterialInterface.h"

AMazeObstacleManager::AMazeObstacleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMazeObstacleManager::BeginPlay()
{
	Super::BeginPlay();
}

void AMazeObstacleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FIntPoint AMazeObstacleManager::GetRandomFreeCell()
{
	if (!MazeReference)
	{
		return FIntPoint(-1, -1);
	}

	int32 Attempts = 100;

    while (Attempts--)
    {
        int32 X =
            RandomStream.RandRange(
                0,
                MazeReference->GetCellsWidth() - 1);

        int32 Y =
            RandomStream.RandRange(
                0,
                MazeReference->GetCellsHeight() - 1);

		FIntPoint Cell(X, Y);

		if (Cell == MazeReference->GetStartCell())
		{
			continue;
		}

		if (Cell == MazeReference->GetEndCell())
		{
			continue;
		}

		if (!OccupiedCells.Contains(Cell))
		{
			return Cell;
		}
	}

	return FIntPoint(-1, -1);
}

int32 AMazeObstacleManager::GetNearestSpillDistance(
    const FIntPoint& Candidate) const
{
    if (SpillCells.IsEmpty())
    {
        return INT_MAX;
    }

    int32 BestDistance = INT_MAX;

    for (const FIntPoint& Existing : SpillCells)
    {
        const int32 Distance =
            FMath::Abs(Candidate.X - Existing.X) +
            FMath::Abs(Candidate.Y - Existing.Y);

        BestDistance =
            FMath::Min(
                BestDistance,
                Distance);
    }

    return BestDistance;
}

// finds the spillcell that is the furthest away from the previous spillcell
FIntPoint AMazeObstacleManager::GetBestSpillCell() const
{
    if (!MazeReference)
    {
        return FIntPoint(-1, -1);
    }

    TArray<FIntPoint> Candidates;

    for (int32 X = 0;
         X < MazeReference->GetCellsWidth();
         X++)
    {
        for (int32 Y = 0;
             Y < MazeReference->GetCellsHeight();
             Y++)
        {
            FIntPoint Cell(X, Y);

            if (OccupiedCells.Contains(Cell))
            {
                continue;
            }

            if (Cell == MazeReference->GetStartCell())
            {
                continue;
            }

            if (Cell == MazeReference->GetEndCell())
            {
                continue;
            }

            Candidates.Add(Cell);
        }
    }

    if (Candidates.IsEmpty())
    {
        return FIntPoint(-1, -1);
    }

    if (SpillCells.IsEmpty())
    {
        const int32 RandomIndex =
            RandomStream.RandRange(
                0,
                Candidates.Num() - 1);

        return Candidates[RandomIndex];
    }

    int32 FurthestDistance = -1;
    TArray<FIntPoint> BestCells;

    for (const FIntPoint& Cell : Candidates)
    {
        const int32 Distance =
            GetNearestSpillDistance(Cell);

        if (Distance > FurthestDistance)
        {
            FurthestDistance = Distance;

            BestCells.Empty();
            BestCells.Add(Cell);
        }
        else if (Distance == FurthestDistance)
        {
            BestCells.Add(Cell);
        }
    }

    const int32 RandomIndex =
        RandomStream.RandRange(
            0,
            BestCells.Num() - 1);

    return BestCells[RandomIndex];
}

void AMazeObstacleManager::SpawnDistractions()
{
	if (!MazeReference)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("No maze reference assigned."));
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Maze Width=%d Height=%d"),
		MazeReference->GetCellsWidth(),
		MazeReference->GetCellsHeight());

	if (DistractionClasses.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("No distraction classes assigned."));
		return;
	}

	for (int32 i = 0;
		 i < NumberOfDistractions;
		 i++)
	{
		FIntPoint Cell = GetRandomFreeCell();

		if (Cell.X < 0)
		{
			continue;
		}

		OccupiedCells.Add(Cell);

		int32 RandomIndex =
			RandomStream.RandRange(
				0,
				DistractionClasses.Num() - 1);

		TSubclassOf<AObstacleBase> ClassToSpawn =
			DistractionClasses[RandomIndex];

		FVector SpawnLocation =
			MazeReference->GetCellWorldPosition(
				Cell.X,
				Cell.Y);

		SpawnLocation.Z += ObstacleHeightOffset;
        
		//check :0000
		UE_LOG(LogTemp, Warning,
	TEXT("Spawn Location = %s"),
	*SpawnLocation.ToString());
        
		AObstacleBase* Spawned =
			GetWorld()->SpawnActor<AObstacleBase>(
				ClassToSpawn,
				SpawnLocation,
				FRotator(
					0.f,
					RandomStream.RandRange(0.f, 360.f),
					0.f));

		if (Spawned)
		{
			SpawnedObstacles.Add(Spawned);
			
			if (MazeReference)
			{
				Spawned->AttachToActor(MazeReference, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}


void AMazeObstacleManager::SpawnSpills()
{
	if (!MazeReference)
	{
		return;
	}

	if (SpillClasses.IsEmpty())
	{
		return;
	}

    int32 SpawnedCount = 0;
    int32 Attempts = NumberOfSpills * 10;

    while (SpawnedCount < NumberOfSpills &&
           Attempts-- > 0)
    {
        FIntPoint Cell = GetBestSpillCell();

        if (Cell.X < 0)
        {
            break;
        }

		OccupiedCells.Add(Cell);

        int32 RandomIndex =
            RandomStream.RandRange(
                0,
                SpillClasses.Num() - 1);

		TSubclassOf<AObstacleBase> ClassToSpawn =
			SpillClasses[RandomIndex];

		FVector SpawnLocation =
			MazeReference->GetCellWorldPosition(
				Cell.X,
				Cell.Y);

        SpawnLocation.Z += ObstacleHeightOffset;

		AObstacleBase* Spawned =
			GetWorld()->SpawnActor<AObstacleBase>(
				ClassToSpawn,
				SpawnLocation,
				FRotator::ZeroRotator);

        if (!Spawned)
        {
            OccupiedCells.Remove(Cell);
            continue;
        }

        const bool bMud = RandomStream.RandRange(0, 1) == 0;
        
        Spawned->AssignSpillType(
            bMud,
            MudMaterial,
            SlimeMaterial);

        if (!Spawned->IsPassable())
        {
            BlockedCells.Add(Cell);

            if (!IsMazeStillReachable())
            {
                BlockedCells.Remove(Cell);
                OccupiedCells.Remove(Cell);

                Spawned->Destroy();
                continue;
            }
        }

        SpawnedObstacles.Add(Spawned);
        SpillCells.Add(Cell);

        SpawnedCount++;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Spawned %s spill at (%d,%d)"),
            Spawned->IsMud()
                ? TEXT("Mud")
                : TEXT("Slime"),
            Cell.X,
            Cell.Y);
    	
    	
    	if (MazeReference)
    	{
    		Spawned->AttachToActor(MazeReference, FAttachmentTransformRules::KeepWorldTransform);
    	}
    }

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Spawned %d/%d spills"),
		SpawnedCount,
		NumberOfSpills);
}

void AMazeObstacleManager::SpawnObstacles(int32 Seed)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("SpawnObstacles() called with Seed %d"),
        Seed);
    
    UE_LOG(
    LogTemp,
    Warning,
    TEXT("Distractions=%d Spills=%d"),
    NumberOfDistractions,
    NumberOfSpills);

    RandomStream.Initialize(Seed);

    for (AActor* Obstacle : SpawnedObstacles)
    {
        if (Obstacle)
        {
            Obstacle->Destroy();
        }
    }

    SpawnedObstacles.Empty();
    OccupiedCells.Empty();
    BlockedCells.Empty();
    SpillCells.Empty();

    SpawnDistractions();
    SpawnSpills();
}

//make sure that no impassable obstacles are in the way of the maze path
bool AMazeObstacleManager::IsMazeStillReachable() const
{
	if (!MazeReference)
	{
		return false;
	}

	const auto& MazeCells =
		MazeReference->GetMazeCells();

	const FIntPoint Start =
		MazeReference->GetStartCell();

	const FIntPoint End =
		MazeReference->GetEndCell();

	TQueue<FIntPoint> Open;
	TSet<FIntPoint> Visited;

	Open.Enqueue(Start);
	Visited.Add(Start);

	while (!Open.IsEmpty())
	{
		FIntPoint Current;
		Open.Dequeue(Current);

		if (Current == End)
		{
			return true;
		}

        const FMazeCell& Cell =
            MazeCells[Current.Y][Current.X];
        
        auto TryVisit =
            [&](int32 NX, int32 NY){
                FIntPoint Next(NX, NY);

			if (!MazeReference->IsInBounds(NX, NY))
			{
				return;
			}

			if (BlockedCells.Contains(Next))
			{
				return;
			}

			if (Visited.Contains(Next))
			{
				return;
			}

			Visited.Add(Next);
			Open.Enqueue(Next);
		};

		if (!Cell.WallNorth)
		{
			TryVisit(Current.X,
			         Current.Y + 1);
		}

		if (!Cell.WallSouth)
		{
			TryVisit(Current.X,
			         Current.Y - 1);
		}

		if (!Cell.WallEast)
		{
			TryVisit(Current.X + 1,
			         Current.Y);
		}

		if (!Cell.WallWest)
		{
			TryVisit(Current.X - 1,
			         Current.Y);
		}
	}

    return false;
}
