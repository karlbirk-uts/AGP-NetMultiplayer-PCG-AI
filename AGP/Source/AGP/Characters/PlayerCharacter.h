// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCharacter.generated.h"

class UPlayerCharacterHUD;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class AGP_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_Owner() override;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* JumpAction;
	
	UPROPERTY(EditDefaultsOnly)
	UInputAction* FireAction;
	
	UPROPERTY(EditDefaultsOnly)
	UInputAction* ReloadAction;
	
	// for player-to-player comms------------------------------
	UPROPERTY(EditDefaultsOnly)
	UInputAction* CommsWheelAction;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> CommsWheelClass;

	UPROPERTY()
	UUserWidget* CommsWheelWidget;

	UPROPERTY()
	bool bWheelOpen = false;
	//-------------------------------------------
	
	UPROPERTY(EditDefaultsOnly)
	UInputMappingContext* InputMappingContext;
	
	

	UPROPERTY(EditDefaultsOnly, meta = (UIMin = "0.0", UIMax = "1.0"))
	float LookSensitivity;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UPlayerCharacterHUD> PlayerHUDClass;
	
	UPROPERTY()
	UPlayerCharacterHUD* PlayerHUD;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION()
	void UpdateHealthBar(float HealthPercent);
	
	void UpdateAmmoText(int32 CurrentAmmoAmount, int32 MagazineSize);
	
	void DrawUI();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetMeshVisibility();
	
	bool IsDead() const;
	

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FireWeapon(const FInputActionValue& Value);
	void ReloadWeapon(const FInputActionValue& Value);
	
	//for player-to-player comms-------------
	void ToggleCommsWheel(const FInputActionValue& Value);

	void ShowCommsWheel(const FInputActionValue& Value);
	void HideCommsWheel(const FInputActionValue& Value);
	//-----------------------------------------
};
