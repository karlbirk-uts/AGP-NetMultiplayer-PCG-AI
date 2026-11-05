// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AGPGameModeBase.generated.h"

class AProceduralMazeFacility;
class APCGTerrainLandscape;

/**
 * ALL CODE HAS BEEN COPIED TO MULTIPLAYERGAMEMODE :)
 */
UCLASS()
class AGP_API AAGPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void GenerateMazeOnly();

	UFUNCTION(BlueprintCallable)
	void GenerateTerrainOnly();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Generation")
	AProceduralMazeFacility* MazeReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Generation")
	APCGTerrainLandscape* LandscapeReference;

	
};
