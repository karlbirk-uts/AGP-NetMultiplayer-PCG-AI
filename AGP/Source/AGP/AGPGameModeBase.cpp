// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGPGameModeBase.h"

#include "Kismet/GameplayStatics.h"
#include "Landscape/MazeObstacleManager.h"
#include "Landscape/PCGTerrainLandscape.h"
#include "Landscape/ProceduralMazeFacility.h"

void AAGPGameModeBase::BeginPlay()
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

void AAGPGameModeBase::GenerateMazeOnly()
{
	if (MazeReference)
	{
		const int32 Seed =
			FMath::Rand();
		
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Maze Seed = %d"),
			Seed);
		UE_LOG(
		LogTemp,
		Warning,
		TEXT("Maze FMath::Rand() = %d"),
		FMath::Rand());
		
		UE_LOG(
		LogTemp,
		Warning,
		TEXT("Maze FMath::RandRange() = %d"),
		FMath::RandRange(0, INT_MAX));

		MazeReference->MulticastRegenerateMaze(
			Seed,
			MazeReference->ObstacleManager->NumberOfSpills,
			MazeReference->ObstacleManager->NumberOfDistractions);
	}
}

void AAGPGameModeBase::GenerateTerrainOnly()
{
	if (LandscapeReference)
	{
		const int32 Seed = FMath::Rand();

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Terrain Seed = %d"),
			Seed);
		UE_LOG(
		LogTemp,
		Warning,
		TEXT("FMath::Rand() = %d"),
		FMath::Rand());
		
		UE_LOG(
		LogTemp,
		Warning,
		TEXT("FMath::RandRange() = %d"),
		FMath::RandRange(0, INT_MAX));
		
		LandscapeReference->MulticastRegenerateTerrain(
			Seed,
			LandscapeReference->PerlinScale,
			LandscapeReference->PerlinRoughness);
	}
}



