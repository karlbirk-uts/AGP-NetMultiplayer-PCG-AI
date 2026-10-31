#include "MazeObstacleManager.h"
#include "Containers/Queue.h"

#include "ObstacleBase.h"
#include "ProceduralMazeFacility.h"

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
        int32 X = FMath::RandRange(
            0,
            MazeReference->GetCellsWidth() - 1);

        int32 Y = FMath::RandRange(
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

void AMazeObstacleManager::SpawnDistractions()
{
    
    if (!MazeReference)
    {
        UE_LOG(LogTemp, Warning, TEXT("No maze reference assigned."));
        return;
    }
    
    UE_LOG(LogTemp, Warning,
    TEXT("Maze Width=%d Height=%d"),
    MazeReference->GetCellsWidth(),
    MazeReference->GetCellsHeight());
    

    if (DistractionClasses.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No distraction classes assigned."));
        return;
    }

    for (int32 i = 0; i < NumberOfDistractions; i++)
    {
        FIntPoint Cell = GetRandomFreeCell();

        if (Cell.X < 0)
        {
            continue;
        }

        OccupiedCells.Add(Cell);

        int32 RandomIndex =
            FMath::RandRange(
                0,
                DistractionClasses.Num() - 1);

        TSubclassOf<AObstacleBase> ClassToSpawn =
            DistractionClasses[RandomIndex];

        FVector SpawnLocation =
            MazeReference->GetCellWorldPosition(
                Cell.X,
                Cell.Y);

        SpawnLocation.Z += ObstacleHeightOffset;

        AObstacleBase* Spawned =
            GetWorld()->SpawnActor<AObstacleBase>(
                ClassToSpawn,
                SpawnLocation,
                FRotator(
                    0.f,
                    FMath::RandRange(0.f, 360.f),
                    0.f));

        if (Spawned)
        {
            SpawnedObstacles.Add(Spawned);
        }
    }
}

void AMazeObstacleManager::SpawnObstacles()
{
    UE_LOG(LogTemp, Warning,
        TEXT("SpawnObstacles() called"));

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

    SpawnDistractions();
    SpawnSpills();
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
    int32 Attempts = NumberOfSpills * 20;

    while (SpawnedCount < NumberOfSpills
        && Attempts-- > 0)
    {
        FIntPoint Cell =
            GetRandomFreeCell();

        if (Cell.X < 0)
        {
            continue;
        }

        BlockedCells.Add(Cell);

        if (!IsMazeStillReachable())
        {
            BlockedCells.Remove(Cell);
            continue;
        }

        OccupiedCells.Add(Cell);

        int32 RandomIndex =
            FMath::RandRange(
                0,
                SpillClasses.Num() - 1);

        TSubclassOf<AObstacleBase> ClassToSpawn =
            SpillClasses[RandomIndex];

        FVector SpawnLocation =
            MazeReference->GetCellWorldPosition(
                Cell.X,
                Cell.Y);

        SpawnLocation.Z +=
            ObstacleHeightOffset;

        AObstacleBase* Spawned =
            GetWorld()->SpawnActor<AObstacleBase>(
                ClassToSpawn,
                SpawnLocation,
                FRotator::ZeroRotator);

        if (Spawned)
        {
            SpawnedObstacles.Add(Spawned);

            SpawnedCount++;

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Spawned Spill (%d,%d)"),
                Cell.X,
                Cell.Y);
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Spawned %d/%d spills"),
        SpawnedCount,
        NumberOfSpills);
}

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
            [&](int32 NX,
                int32 NY)
            {
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
