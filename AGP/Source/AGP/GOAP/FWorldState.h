// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldState.generated.h"

/**
 * 
 */
USTRUCT()
struct FWorldState
{
	GENERATED_BODY()

	// TMap logic will be used to "apply" changes to KVP values
	// quickly using .Add() / Emplace / Append. 
	// This should work, otherwise helper function to loop through
	// all KVPs manually. Responsibility of Apply() maybe should be 
	// within this or another dedicated manager, to keep this light 
	// weight. Currently, this is copied many times per GetPlan() and DoPlan()
	UPROPERTY(VisibleAnywhere, Category = "GOAP")
	TMap<FString, bool> StatesMap;


	// Check whether other FWorldState stores KVPs that match this
	// Note: Replicating behaviour from FGameplayTagContainer::HasAll ()
	// Note: May use FGameplayTag instead of FString in future.
	bool HasAll(const FWorldState& OtherWorldState) const
	{
		// Foreach KVP, see if there's a value mismatch
		for (const TPair<FString, bool>& OtherKVP : OtherWorldState.StatesMap)
		{
			// TMap::Find() ALWAYS returning success?????
			// bool bStatesMapKeyFound
			// 	= StatesMap.Find(OtherKVP.Key);
			// CONVERTS A POINTER TO A BOOL! 
			// SILENT FAIL POINT that breaks everything everywhere.
			// Hours. Grey hairs. Hair loss. FML.

			// Get pointer to this struct's KVP Value
			const bool* ThisKVPValue
				= this->StatesMap.Find(OtherKVP.Key);

			// If pointer-to-value found when using Find(Key)
			if (ThisKVPValue != nullptr)
			{
				// If ThisKVPValuePtr Value doesn't match
				if (*ThisKVPValue != OtherKVP.Value)
				{
					return false;
				}
			}
			else
			{
				// Find(Key) returned nullptr / Not found.

				// Only when comparison wants to compare OtherKVP's true value
				// will this flag false.
				if (OtherKVP.Value == true)
				{
					return false;
				}

				// else
				// continue iterating
			}
		}


		return true;
	}
};
