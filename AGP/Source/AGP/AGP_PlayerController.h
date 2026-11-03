// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AGP_PlayerController.generated.h"

/**
 * 
 */
UCLASS()
class AGP_API AAGP_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UUserWidget> ProceduralWidgetClass;
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerGenerateMaze();
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerGenerateTerrain();
	
	//---TERRAIN VARIABLES
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerSetPerlinScale(float NewValue);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerSetPerlinRoughness(float NewValue);
	
	//--OBSTACLEGEN VARIABLES
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerSetNumberOfSpills(int32 NewValue);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerSetNumberOfDistractions(int32 NewValue);
};
