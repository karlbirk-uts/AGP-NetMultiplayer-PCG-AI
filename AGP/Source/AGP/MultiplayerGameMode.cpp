// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerGameMode.h"

#include "Characters/PlayerCharacter.h"

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
