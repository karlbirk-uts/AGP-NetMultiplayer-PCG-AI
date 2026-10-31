#pragma once

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EObstacleType : uint8
{
	Distraction UMETA(DisplayName = "Distraction"),
	Spill       UMETA(DisplayName = "Spill")
};

UCLASS()
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

	UFUNCTION(BlueprintPure, Category = "Obstacle")
	UStaticMeshComponent* GetMesh() const
	{
		return Mesh;
	}
};