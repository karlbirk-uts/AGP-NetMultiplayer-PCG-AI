#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeObstacleManager.generated.h"

class AProceduralMazeFacility;
class AObstacleBase;

UCLASS()
class AGP_API AMazeObstacleManager : public AActor
{
	GENERATED_BODY()

public:
	AMazeObstacleManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Maze")
	AProceduralMazeFacility* MazeReference;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	int32 NumberOfDistractions = 3;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	int32 NumberOfSpills = 5;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	float ObstacleHeightOffset = 0.f;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	TArray<TSubclassOf<AObstacleBase>> DistractionClasses;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	TArray<TSubclassOf<AObstacleBase>> SpillClasses;

	UFUNCTION(BlueprintCallable)
	void SpawnObstacles();

protected:

	void SpawnDistractions();

	void SpawnSpills();

	FIntPoint GetRandomFreeCell();

	TSet<FIntPoint> OccupiedCells;

	UPROPERTY()
	TArray<AActor*> SpawnedObstacles;
    

    
	TSet<FIntPoint> BlockedCells;
    
	bool IsMazeStillReachable() const;
};