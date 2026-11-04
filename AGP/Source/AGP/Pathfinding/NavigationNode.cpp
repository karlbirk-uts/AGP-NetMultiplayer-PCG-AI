// Fill out your copyright notice in the Description page of Project Settings.


#include "NavigationNode.h"

#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Prediction.h"
#include "Perception/AISense_Team.h"
#include "Perception/AISense_Touch.h"

// Sets default values
ANavigationNode::ANavigationNode()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bNetLoadOnClient = false;

	LocationComponent = CreateDefaultSubobject<USceneComponent>("Location Component");
	if (LocationComponent)
	{
		SetRootComponent(LocationComponent);
	}

	PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("Perception Source");
}

// Called when the game starts or when spawned
void ANavigationNode::BeginPlay()
{
	Super::BeginPlay();

	DrawDebugSphere(
		GetWorld(), 
		GetActorLocation(), 
		50.0f, 
		16, FColor::Yellow, true, -1, 0, 2.0f);
	for (ANavigationNode* Node : ConnectedNodes)
	{
		if (!Node) continue;
		FColor LineColor = FColor::Red;
		float LineThickness = 8.0f;
		for (ANavigationNode* NeighbourNode : Node->ConnectedNodes)
		{
			if (!NeighbourNode) continue;
			if (NeighbourNode == this)
			{
				LineColor = FColor::Blue;
				LineThickness = 8.0f;
				break;
			}
		}
		
		FVector LineStart = FVector(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 100.0f);
		FVector LineEnd = FVector(Node->GetActorLocation().X, Node->GetActorLocation().Y, Node->GetActorLocation().Z + 100.0f);
		
		DrawDebugLine(
			GetWorld(), 
			LineStart, 
			LineEnd, 
			LineColor, 
			true, 
			-1, 
			0,
			LineThickness);
	}

	

	
}

// Called every frame
void ANavigationNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TArray<ANavigationNode*> ANavigationNode::GetConnectedNodes()
{
	return ConnectedNodes;
}

void ANavigationNode::AddConnectedNode(ANavigationNode* Node)
{
	if (ConnectedNodes.Contains(Node))
	{
		return;
	}
	ConnectedNodes.Add(Node);
}
