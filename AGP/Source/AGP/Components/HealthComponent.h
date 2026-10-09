// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


// Custom events for informing player when health changes or when death occurs.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentHealthChange, float, NewHealthValuePercentage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
	UPROPERTY(Replicated)
	float MaxHealth;
	UPROPERTY(Replicated, ReplicatedUsing=OnHealthChange)
	float CurrentHealth;
	bool bIsDead;
	
	void ResetHealth();
	void OnDeath();
	
	UFUNCTION()
	void OnHealthChange();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool IsDead() const;
	float GetCurrentHealth() const;
	float GetCurrentHealthPercentage() const;
	void ApplyDamage(float DamageAmount);
	void ApplyHealing(float HealingAmount);
	
	UPROPERTY()
	FOnCurrentHealthChange OnCurrentHealthChangeEvent;
	
	UPROPERTY()
	FOnDeath OnDeathEvent;
	
	UFUNCTION()
	void HandleApplyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

		
};
