// Fill out your copyright notice in the Description page of Project Settings.


#include "GOAPGoal.h"

// Sets default values
AGOAPGoal::AGOAPGoal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGOAPGoal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGOAPGoal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * REFERENCES:
 * Studying free Godot downloadable project that demonstrates GOAP
 * https://github.com/viniciusgerevini/godot-goap
 * 
 * 
 */