// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldState.h"
#include "UObject/NoExportTypes.h"
#include "Goal.generated.h"

/**
 * Light weight, OOP-style, for GOAP planning nodes
 * and NPC DoPlan(). No private nor immutable values.
 */
UCLASS()
class AGP_API UGoal : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FWorldState GoalWorldState;

	// 1.0f is a higher priority than 10.0f
	UPROPERTY()
	float GoalPriority = 1.0f;
};
