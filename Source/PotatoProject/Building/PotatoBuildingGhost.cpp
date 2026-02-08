#include "PotatoBuildingGhost.h"

APotatoBuildingGhost::APotatoBuildingGhost()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void APotatoBuildingGhost::BeginPlay()
{
	Super::BeginPlay();
}

void APotatoBuildingGhost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APotatoBuildingGhost::UpdateLocation(FVector NewLocation)
{

}

bool APotatoBuildingGhost::CheckValidPlacement()
{
	return false;
}

void APotatoBuildingGhost::SetGhostColor(bool IsValid) 
{

}