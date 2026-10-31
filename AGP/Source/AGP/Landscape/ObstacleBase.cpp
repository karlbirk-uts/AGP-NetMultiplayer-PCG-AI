#include "ObstacleBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AObstacleBase::AObstacleBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(
		TEXT("Root"));

	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Mesh"));

	Mesh->SetupAttachment(Root);
}

void AObstacleBase::BeginPlay()
{
	Super::BeginPlay();
}

void AObstacleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}