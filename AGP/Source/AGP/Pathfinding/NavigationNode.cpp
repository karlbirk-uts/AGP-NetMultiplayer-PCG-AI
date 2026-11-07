// ReSharper disable CppTooWideScope
#include "NavigationNode.h"

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

	// PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("Perception Source");
	

	
	
	// NPCVisitedCounter = 0;

	
	// SECOND ROOT COMPONENT BREAKS THE COORDS FOR OTHER CODE...
	// Create the sphere component
	// TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	// RootComponent = TriggerSphere;
	//
	// // Set radius to 1 meter (100 UU)
	// TriggerSphere->SetSphereRadius(100.0f);
	//
	// // Configure collision settings to look for Pawns
	// TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));

}

// Called when the game starts or when spawned
void ANavigationNode::BeginPlay()
{
	Super::BeginPlay();

	if (bIsJunction)
	{
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			50.0f,
			4, // lowered this to make it easier on my computer xp -ivy
			FColor::Orange,
			true,
			-1,
			0,
			2.0f);
	}
	else if (bIsDeadend)
	{
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			50.0f,
			4,
			FColor::Red,
			true,
			-1,
			0,
			2.0f);
	}
	else
	{
		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			50.0f,
			4,
			FColor::Yellow,
			true,
			-1,
			0,
			2.0f);
	}


	// Draw connecting lines
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
		FVector LineEnd = FVector(Node->GetActorLocation().X, Node->GetActorLocation().Y,
		                          Node->GetActorLocation().Z + 100.0f);

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
	
	
	
	// Bind the overlap function
	// TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ANavigationNode::OnOverlapBegin);

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

FVector ANavigationNode::GetLocationComponentTransform()
{
	return LocationComponent->GetComponentLocation();
}

// void ANavigationNode::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
// 	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
// {
// 	if (OtherActor && OtherActor != this)
// 	{
// 		// Check if the overlapping actor is a Pawn (or cast to your specific AAgentCharacter)
// 		APawn* OverlappingPawn = Cast<APawn>(OtherActor);
// 		if (OverlappingPawn)
// 		{
// 			NPCVisitedCounter++;
//             
// 			// Optional debug message to verify it works
// 			// UE_LOG(LogTemp, Warning, TEXT("%s visited! Counter: %d"), *OtherActor->GetName(), NPCVisitedCounter);
// 		}
// 	}
// }
