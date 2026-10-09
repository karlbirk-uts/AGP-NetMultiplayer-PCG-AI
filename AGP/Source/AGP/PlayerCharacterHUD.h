// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCharacterHUD.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class AGP_API UPlayerCharacterHUD : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, meta=(BindWidget))
	UProgressBar* HealthBar;
	
	UPROPERTY(EditAnywhere, meta=(BindWidget))
	UTextBlock* AmmoText;
	
public:
	void SetHealthBar(float HealthPercent);
	void SetAmmoText(int32 CurrentAmmoCount, int32 MaxAmmoCount);
};
