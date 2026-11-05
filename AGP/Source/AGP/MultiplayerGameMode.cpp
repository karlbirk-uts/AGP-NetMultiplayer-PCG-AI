// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerGameMode.h"

#include "Characters/PlayerCharacter.h"

#include "Kismet/GameplayStatics.h"
#include "Landscape/MazeObstacleManager.h"
#include "Landscape/PCGTerrainLandscape.h"
#include "Landscape/ProceduralMazeFacility.h"

/* --UNCOMMENT IF USING MULTIPLAYERGAMEMODE FOR PCG
void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	MazeReference = 
		Cast<AProceduralMazeFacility>(
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				AProceduralMazeFacility::StaticClass()));

	LandscapeReference =
		Cast<APCGTerrainLandscape>(
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				APCGTerrainLandscape::StaticClass()));
	
	UE_LOG(LogTemp, Warning,
	TEXT("Maze Ref = %s"),
	*GetNameSafe(MazeReference));

	UE_LOG(LogTemp, Warning,
		TEXT("Landscape Ref = %s"),
		*GetNameSafe(LandscapeReference));
}

void AMultiplayerGameMode::GenerateMazeOnly()
{
	if (MazeReference)
	{
		const int32 Seed =
			FMath::Rand();

		MazeReference->MulticastRegenerateMaze(
			Seed,
			MazeReference->ObstacleManager->NumberOfSpills,
			MazeReference->ObstacleManager->NumberOfDistractions);
	}
}

void AMultiplayerGameMode::GenerateTerrainOnly()
{
	if (LandscapeReference)
	{
		const int32 Seed = FMath::Rand();
		
		LandscapeReference->MulticastRegenerateTerrain(
			Seed,
			LandscapeReference->PerlinScale,
			LandscapeReference->PerlinRoughness);
	}
}
*/

void AMultiplayerGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller) return;
	if (APlayerCharacter* CurrentlyPossessedPlayerCharacter = Cast<APlayerCharacter>(Controller->GetPawn()))
	{
		Controller->UnPossess();
		CurrentlyPossessedPlayerCharacter->Destroy();
		
		// This will spawn a new player character at their start position and then possess them!
		RestartPlayer(Controller);
	}
}
