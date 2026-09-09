// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppOverrideWithDifferentVisibility
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ConeCharacter.generated.h"

UCLASS()
class AGP_IVYKARL_NETPCGAI_API AConeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AConeCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
