// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterHUD.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerCharacterHUD::SetHealthBar(float HealthPercent)
{
	if (!HealthBar) return;
	HealthBar->SetPercent(HealthPercent);
}

void UPlayerCharacterHUD::SetAmmoText(int32 CurrentAmmoCount, int32 MaxAmmoCount)
{
	if (!AmmoText) return;
	const FString TextToShow = FString::FromInt(CurrentAmmoCount) + "/" + FString::FromInt(MaxAmmoCount);
	AmmoText->SetText(FText::FromString(TextToShow));
}
