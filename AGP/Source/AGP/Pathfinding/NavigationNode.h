/**
 * ANavigationNode
 * @authors AGP subject coordinators, Karl Birk Student
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "NavigationNode.generated.h"

UCLASS(PrioritizeCategories="A4 Navigation")
class AGP_API ANavigationNode : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANavigationNode();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TArray<ANavigationNode*> GetConnectedNodes();
	void AddConnectedNode(ANavigationNode* Node);


	UFUNCTION(BlueprintPure, Category = "A4 Navigation")
	bool IsBlocked() const { return bIsBlocked; }

protected:
	UPROPERTY(EditAnywhere)
	TArray<ANavigationNode*> ConnectedNodes;

	// UPROPERTY(VisibleAnywhere)
	// USceneComponent* LocationComponent;

	// UPROPERTY(VisibleAnywhere)
	// UAIPerceptionStimuliSourceComponent* PerceptionSource;


public:
	// Testing with subsystem building all nodes pathways like AGP labs
	// but skipping nodes flagged as bIsBlocked==true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A4 Navigation")
	bool bIsBlocked = false;

	UPROPERTY(VisibleAnywhere, Category = "A4 Navigation")
	bool bIsJunction = false;

	UPROPERTY(VisibleAnywhere, Category = "A4 Navigation")
	bool bIsDeadend = false;

	UPROPERTY(VisibleAnywhere, Category = "A4 Navigation")
	bool bIsMud = false;

	UPROPERTY(VisibleAnywhere, Category = "A4 Navigation")
	bool bIsDistraction = false;

	UFUNCTION(BlueprintPure, Category = "A4 Navigation")
	FVector GetLocationComponentTransform();
	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* LocationComponent;
	
	
	// // The trigger zone (1 meter = 100 Unreal Units radius)
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	// USphereComponent* TriggerSphere;
	//
	// Counter to track how many times an NPC visited
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Analytics")
	int32 NPCVisitedCounter;
	//
	// // Function that triggers when something overlaps the sphere
	// UFUNCTION()
	// void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
