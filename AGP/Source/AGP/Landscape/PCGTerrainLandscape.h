#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PCGTerrainLandscape.generated.h"

UCLASS()
class AGP_API APCGTerrainLandscape : public AActor
{
	GENERATED_BODY()

public:
	APCGTerrainLandscape();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere, Category="Landscape")
	int32 Width = 10;

	UPROPERTY(EditAnywhere, Category="Landscape")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, Category="Landscape")
	float VertexSpacing = 500.0f;

	UPROPERTY(EditAnywhere, Category="Landscape")
	float PerlinScale = 1000.0f;

	UPROPERTY(EditAnywhere, Category="Landscape")
	float PerlinRoughness = 0.00012f;

	UPROPERTY(EditAnywhere, Category="Landscape")
	bool bShouldRegenerate = false;

	UPROPERTY(EditAnywhere, Category="Fence")
	UMaterialInterface* FenceMaterial;

	UPROPERTY(EditAnywhere, Category="Landscape")
	UMaterialInterface* TerrainMaterial;

	void GenerateTerrain(float XOffset, int32 SectionIndex, float& OutPerlinOffset);

	void GenerateFence(float XOffset, int32 SectionIndex, float PerlinOffset, bool bRemoveLeft, bool bRemoveRight);

	void ClearLandscape();

public:
	virtual void Tick(float DeltaTime) override;
	//virtual bool ShouldTickIfViewportsOnly() const override;
    
	TArray<float> TerrainHeights;


	UFUNCTION(BlueprintCallable, Category="Procedural Landscape")
	void GenerateLandscape();

	UFUNCTION(BlueprintCallable, Category="Procedural Landscape")
	void RequestRegenerate()
	{
		bShouldRegenerate = true;
	}
};
