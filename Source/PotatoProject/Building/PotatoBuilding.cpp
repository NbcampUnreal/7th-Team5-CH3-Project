#include "PotatoBuilding.h"

APotatoBuilding::APotatoBuilding()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void APotatoBuilding::BeginPlay()
{
	Super::BeginPlay();
}

void APotatoBuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APotatoBuilding::Construct()
{
}

void APotatoBuilding::TakeDamage(float Damage)
{

}

void APotatoBuilding::Repair()
{
}

void APotatoBuilding::Destroy()
{

}