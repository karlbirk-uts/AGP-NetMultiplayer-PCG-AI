// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"

class AProceduralMazeFacility;
class APCGTerrainLandscape;

/**
 * 
 */
UCLASS()
class AGP_API AMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()
	
	public:
	void RespawnPlayer(AController* Controller);
	
/*-----UNCOMMENT IF USING MULTIPLAYERGAMEMODE FOR PCG
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
	*/
};
