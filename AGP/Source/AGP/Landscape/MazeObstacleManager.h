#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeObstacleManager.generated.h"

class AProceduralMazeFacility;
class AObstacleBase;
class UMaterialInterface;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Obstacles")
	int32 NumberOfDistractions = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Obstacles")
	int32 NumberOfSpills = 5;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	float ObstacleHeightOffset = 0.f;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	TArray<TSubclassOf<AObstacleBase>> DistractionClasses;

	UPROPERTY(EditAnywhere, Category="Obstacles")
	TArray<TSubclassOf<AObstacleBase>> SpillClasses;

	UFUNCTION(BlueprintCallable)
	void SpawnObstacles(int32 Seed);
	
	UPROPERTY(EditAnywhere, Category = "Spills")
	UMaterialInterface* MudMaterial;

	UPROPERTY(EditAnywhere, Category = "Spills")
	UMaterialInterface* SlimeMaterial;

protected:
	void SpawnDistractions();
	void SpawnSpills();

	FIntPoint GetRandomFreeCell();

	FIntPoint GetBestSpillCell() const;
	int32 GetNearestSpillDistance(
		const FIntPoint& Candidate) const;

	TSet<FIntPoint> OccupiedCells;

	UPROPERTY()
	TArray<AActor*> SpawnedObstacles;

	TSet<FIntPoint> BlockedCells;

	TArray<FIntPoint> SpillCells;

	bool IsMazeStillReachable() const;
	
private:

	FRandomStream RandomStream;
};