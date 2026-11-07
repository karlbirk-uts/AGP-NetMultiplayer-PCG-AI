#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EObstacleType : uint8 //assign objects to the distraction or spill type in the editor, they have their own features
{
	Distraction UMETA(DisplayName = "Distraction"),
	Spill       UMETA(DisplayName = "Spill")
};

UCLASS(PrioritizeCategories="Transform Obstacle")
class AGP_API AObstacleBase : public AActor
{
	GENERATED_BODY()

public:
	AObstacleBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	EObstacleType ObstacleType;

	UPROPERTY(BlueprintReadOnly, Category = "Spill")
	bool bIsMud = false;

	UPROPERTY(BlueprintReadOnly, Category = "Spill")
	bool bIsSlime = false;

	UPROPERTY(BlueprintReadOnly, Category = "Spill")
	bool bIsPassable = false;

	UFUNCTION(BlueprintCallable, Category = "Spill")
	void AssignSpillType(
		bool bMud,
		UMaterialInterface* MudMaterial,
		UMaterialInterface* SlimeMaterial);

	UFUNCTION(BlueprintPure, Category = "Spill")
	bool IsPassable() const
	{
		return bIsPassable;
	}

	UFUNCTION(BlueprintPure, Category = "Spill")
	bool IsMud() const
	{
		return bIsMud;
	}

	UFUNCTION(BlueprintPure, Category = "Spill")
	bool IsSlime() const
	{
		return bIsSlime;
	}

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	UStaticMeshComponent* GetMesh() const
	{
		return Mesh;
	}
};