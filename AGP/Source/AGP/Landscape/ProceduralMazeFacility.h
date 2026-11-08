#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ProceduralMazeFacility.generated.h"

class AMazeObstacleManager;

USTRUCT()
struct FMazeCell
{
	GENERATED_BODY()

	bool WallNorth = true;
	bool WallSouth = true;
	bool WallEast = true;
	bool WallWest = true;
	bool Visited = false;
};

UCLASS(PrioritizeCategories = "Maze")
class AGP_API AProceduralMazeFacility : public AActor
{
	GENERATED_BODY()

public:
	AProceduralMazeFacility();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent *MazeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Maze", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent *WallMesh;

	UPROPERTY(EditAnywhere, Category = "Maze")
	int32 MazeWidth = 16;

	UPROPERTY(EditAnywhere, Category = "Maze")
	int32 MazeHeight = 16;

	UPROPERTY(EditAnywhere, Category = "Maze")
	float CellSize = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Maze")
	UMaterialInterface *FloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Maze")
	UMaterialInterface *WallMaterial;

	TArray<TArray<FMazeCell>> MazeCells;

	int32 CellsWidth = 0;  // MazeWidth  - 1
	int32 CellsHeight = 0; // MazeHeight - 1

	UPROPERTY(VisibleAnywhere, Category = "Maze Data")
	int32 StartCellX = 0;
	UPROPERTY(VisibleAnywhere, Category = "Maze Data")
	int32 StartCellY = 0;
	UPROPERTY(VisibleAnywhere, Category = "Maze Data")
	int32 EndCellX = 0;
	UPROPERTY(VisibleAnywhere, Category = "Maze Data")
	int32 EndCellY = 0;

public:
	UFUNCTION(BlueprintCallable, Category = "Maze")
	void GenerateMaze();

	UPROPERTY(EditAnywhere, Category = "Maze")
	bool bShouldRegenerate = false;

	UFUNCTION(BlueprintCallable, Category = "Maze")
	void RequestRegenerate()
	{
		bShouldRegenerate = true;
	}

	void ClearMaze();
	bool IsInBounds(int32 CellX, int32 CellY) const;

	void CarveFrom(int32 CellX, int32 CellY);

private:
	void GenerateFloorGrid();
	void InitializeMazeCells();
	void GenerateWallMeshes();

	FRandomStream RandomStream;

	void GenerateMazeFromSeed(int32 Seed);

	int32 CurrentSeed = 0;

	// void GenerateFloorGrid();
	// void InitializeMazeCells();
	// void GenerateWallMeshes();

	void SpawnNavNodesAndPathing();

public: // for obstacles!
	const TArray<TArray<FMazeCell>> &GetMazeCells() const
	{
		return MazeCells;
	}

	int32 GetCellsWidth() const
	{
		return CellsWidth;
	}

	int32 GetCellsHeight() const
	{
		return CellsHeight;
	}

	FIntPoint GetStartCell() const
	{
		return FIntPoint(StartCellX, StartCellY);
	}

	FIntPoint GetEndCell() const
	{
		return FIntPoint(EndCellX, EndCellY);
	}

	float GetCellSize() const
	{
		return CellSize;
	}

	// cell-to-world helper
	UFUNCTION(BlueprintCallable)
	FVector GetCellWorldPosition(int32 CellX, int32 CellY) const;

	UPROPERTY(EditAnywhere, Category = "Maze")
	AMazeObstacleManager *ObstacleManager;

	// ---------------multicast to all machines------------
	// having the spills be sent from the mazegenerator makes the most sense logically as spills
	// cannot be called independently of maze generation
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRegenerateMaze(
		int32 Seed,
		int32 InSpills,
		int32 InDistractions);
	

	virtual bool ShouldTickIfViewportsOnly() const override;

	FVector GetMazeStartCellWorldPosition();
	FVector GetMazeEndCellWorldPosition();
};
