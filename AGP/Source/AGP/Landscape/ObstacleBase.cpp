#include "ObstacleBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

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

void AObstacleBase::AssignSpillType(
	bool bMud,
	UMaterialInterface* MudMaterial,
	UMaterialInterface* SlimeMaterial)
{
	bIsMud = bMud;
	bIsSlime = !bMud;
	bIsPassable = !bMud;

	if (bMud)
	{
		if (MudMaterial)
		{
			Mesh->SetMaterial(0, MudMaterial);
		}
	}
	else
	{
		if (SlimeMaterial)
		{
			Mesh->SetMaterial(0, SlimeMaterial);
		}
	}
}
