// Fill out your copyright notice in the Description page of Project Settings.

#include "AGP_PlayerController.h"

#include "AGPGameModeBase.h"
#include "MultiplayerGameMode.h"


#include "Landscape/PCGTerrainLandscape.h"
#include "Landscape/MazeObstacleManager.h"

#include "Blueprint/UserWidget.h"
#include "Landscape/ProceduralMazeFacility.h"

void AAGP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && ProceduralWidgetClass)
	{
		UUserWidget* Widget =
			CreateWidget<UUserWidget>(
				this,
				ProceduralWidgetClass);

		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
	
}

void AAGP_PlayerController::ServerGenerateMaze_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Server generate Maze"));
	
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM)
	{
		GM->GenerateMazeOnly();
	}
	
	UE_LOG(LogTemp, Warning,
	TEXT("Maze regenerated"));
}

void AAGP_PlayerController::ServerGenerateTerrain_Implementation()
{
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM)
	{
		GM->GenerateTerrainOnly();
	}
}

void AAGP_PlayerController::ServerSetPerlinScale_Implementation(
	float NewValue)
{
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM && GM->LandscapeReference)
	{
		GM->LandscapeReference->PerlinScale =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("PerlinScale updated to %f"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetPerlinRoughness_Implementation(
	float NewValue)
{
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM && GM->LandscapeReference)
	{
		GM->LandscapeReference->PerlinRoughness =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("PerlinRoughness updated to %f"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetNumberOfSpills_Implementation(
	int32 NewValue)
{
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM &&
		GM->MazeReference &&
		GM->MazeReference->ObstacleManager)
	{
		GM->MazeReference->ObstacleManager->NumberOfSpills =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("NumberOfSpills = %d"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetNumberOfDistractions_Implementation(
	int32 NewValue)
{
	AAGPGameModeBase* GM =
		Cast<AAGPGameModeBase>(
			GetWorld()->GetAuthGameMode());

	if (GM &&
		GM->MazeReference &&
		GM->MazeReference->ObstacleManager)
	{
		GM->MazeReference->ObstacleManager->NumberOfDistractions =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("NumberOfDistractions = %d"),
			NewValue);
	}
}


/* ---------- UNCOMMENT IF USING MULTIPLAYERGAMEMODE (AND ASSIGN BP_AGP_PLAYERCONTROLLER IN SETTINGS)
void AAGP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && ProceduralWidgetClass)
	{
		UUserWidget* Widget =
			CreateWidget<UUserWidget>(
				this,
				ProceduralWidgetClass);

		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
	
}

void AAGP_PlayerController::ServerGenerateMaze_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Server generate Maze"));
	
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM)
	{
		GM->GenerateMazeOnly();
	}
	
	UE_LOG(LogTemp, Warning,
	TEXT("Maze regenerated"));
}

void AAGP_PlayerController::ServerGenerateTerrain_Implementation()
{
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM)
	{
		GM->GenerateTerrainOnly();
	}
}

void AAGP_PlayerController::ServerSetPerlinScale_Implementation(
	float NewValue)
{
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM && GM->LandscapeReference)
	{
		GM->LandscapeReference->PerlinScale =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("PerlinScale updated to %f"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetPerlinRoughness_Implementation(
	float NewValue)
{
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM && GM->LandscapeReference)
	{
		GM->LandscapeReference->PerlinRoughness =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("PerlinRoughness updated to %f"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetNumberOfSpills_Implementation(
	int32 NewValue)
{
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM &&
		GM->MazeReference &&
		GM->MazeReference->ObstacleManager)
	{
		GM->MazeReference->ObstacleManager->NumberOfSpills =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("NumberOfSpills = %d"),
			NewValue);
	}
}

void AAGP_PlayerController::ServerSetNumberOfDistractions_Implementation(
	int32 NewValue)
{
	AMultiplayerGameMode* GM =
		Cast<AMultiplayerGameMode>(
			GetWorld()->GetAuthGameMode());

	if (GM &&
		GM->MazeReference &&
		GM->MazeReference->ObstacleManager)
	{
		GM->MazeReference->ObstacleManager->NumberOfDistractions =
			NewValue;

		UE_LOG(LogTemp, Warning,
			TEXT("NumberOfDistractions = %d"),
			NewValue);
	}
}
*/