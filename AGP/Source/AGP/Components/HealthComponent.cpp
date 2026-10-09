// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	
	// SetIsReplicated(true); // CRASHING Editor hard.
	
	SetIsReplicatedByDefault(true);
	
	
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 100.0f;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, CurrentHealth);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Attach handling damage to the owning actor's TakeDamage event.
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleApplyDamage);
	}
	
	ResetHealth();
	
}

void UHealthComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	
	// clear out all delegate bindings to prevent null reference exceptions from occurring.
	
	OnCurrentHealthChangeEvent.Clear();
	OnCurrentHealthChangeEvent.RemoveAll(this);
	
	OnDeathEvent.Clear();
	OnDeathEvent.RemoveAll(this);
}


void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
}

void UHealthComponent::OnDeath()
{
	bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("The character has died!"));
	
	if (GetOwnerRole()!=ROLE_Authority) return;
	OnDeathEvent.Broadcast();
}

void UHealthComponent::OnHealthChange()
{
	OnCurrentHealthChangeEvent.Broadcast(GetCurrentHealthPercentage());
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UHealthComponent::GetCurrentHealthPercentage() const
{
	return CurrentHealth/MaxHealth;
}

void UHealthComponent::ApplyDamage(const float DamageAmount)
{
	if (bIsDead) return;
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		OnDeath();
	}
	OnHealthChange();
}

void UHealthComponent::ApplyHealing(const float HealingAmount)
{
	if (bIsDead) return;
	CurrentHealth += HealingAmount;
	CurrentHealth = FMath::Clamp(CurrentHealth,0.0f,MaxHealth);
	OnHealthChange();
}

void UHealthComponent::HandleApplyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage<0)
	{
		ApplyHealing(FMath::Abs(Damage));
	}
	else
	{
		ApplyDamage(Damage);
	}
}

