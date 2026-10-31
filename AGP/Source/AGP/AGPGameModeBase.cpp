// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGPGameModeBase.h"

#include "Blueprint/UserWidget.h"

void AAGPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (ProceduralWidgetClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), ProceduralWidgetClass);
		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
}


