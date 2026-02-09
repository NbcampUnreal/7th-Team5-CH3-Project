#include "PotatoProjectile.h"

APotatoProjectile::APotatoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APotatoProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void APotatoProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotatoProjectile::Launch(FVector Direction)
{

}

void APotatoProjectile::OnHit(AActor* HitActor)
{

}

void APotatoProjectile::Explode()
{

}