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
    bool WallEast  = true;
    bool WallWest  = true;
    bool Visited   = false;
};

UCLASS(PrioritizeCategories="00 | Maze")
class AGP_API AProceduralMazeFacility : public AActor
{
    GENERATED_BODY()

public:
    AProceduralMazeFacility();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="00 | Maze", meta=(AllowPrivateAccess="true"))
    UProceduralMeshComponent* MazeMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="00 | Maze", meta=(AllowPrivateAccess="true"))
    UProceduralMeshComponent* WallMesh;

    UPROPERTY(EditAnywhere, Category="00 | Maze")
    int32 MazeWidth = 16;

    UPROPERTY(EditAnywhere, Category="00 | Maze")
    int32 MazeHeight = 16;

    UPROPERTY(EditAnywhere, Category="00 | Maze")
    float CellSize = 300.0f;

    UPROPERTY(EditAnywhere, Category="00 | Maze")
    UMaterialInterface* FloorMaterial;

    UPROPERTY(EditAnywhere, Category="00 | Maze")
    UMaterialInterface* WallMaterial;

    TArray<TArray<FMazeCell>> MazeCells;

    int32 CellsWidth  = 0; // MazeWidth  - 1
    int32 CellsHeight = 0; // MazeHeight - 1

    int32 StartCellX = 0;
    int32 StartCellY = 0;
    int32 EndCellX   = 0;
    int32 EndCellY   = 0;

public:
    UFUNCTION(BlueprintCallable, Category="00 | Maze")
    void GenerateMaze();

    UPROPERTY(EditAnywhere, Category = "00 | Maze")
    bool bShouldRegenerate = false;

    UFUNCTION(BlueprintCallable, Category="00 | Maze")
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

    
public: // for obstacles!

    const TArray<TArray<FMazeCell>>& GetMazeCells() const
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
    
    //cell-to-world helper
    UFUNCTION(BlueprintCallable)
    FVector GetCellWorldPosition(int32 CellX, int32 CellY) const;
    
    UPROPERTY(EditAnywhere, Category = "00 | Maze")
    AMazeObstacleManager* ObstacleManager;
    
    
    virtual bool ShouldTickIfViewportsOnly() const override;

};
