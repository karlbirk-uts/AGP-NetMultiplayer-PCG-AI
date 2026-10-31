// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldState.h"
#include "UObject/NoExportTypes.h"
#include "Action.generated.h"

/**
 * Light weight, OOP-style, for GOAP planning nodes
 * and NPC DoPlan(). No private nor immutable values.
 */
UCLASS()
class AGP_API UAction: public UObject
{
	GENERATED_BODY()

public:
	
	// 1.0f preferred over 10.0f by UPlanner::GetPlan()
	float ActionCost = MAX_flt; // i changed it to this so it would compile, feel free to change it back - ivy

	// Unique, comparable name.
	FString UniqueName = "";

	// Prerequisite for Action
	FWorldState ActionPrereqs;
	
	// Effects of Action (TMaps can override values)
	FWorldState ActionOutcomes;

	// Implementation of actual NPC actions, e.g. Evade(), 
	// will be elsewhere, for "Agent" to call its own
	// DoPlan() with DoAction(UAction).

	
	// DoPlan() will run through a TQueue of UAction and track completion.
	bool bIsCompleted = false;

};
