#include "PotatoProjectile.h"
#include "Components/StaticMeshComponent.h"

APotatoProjectile::APotatoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APotatoProjectile::BeginPlay()
{
	Super::BeginPlay();
	Mesh = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));
}

void APotatoProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector Location = GetActorLocation();

	//float Gravity = -980.f; // UE 기본 중력 (cm/s^2)
	Velocity.Z += -980.f * Gravity * DeltaTime;

	Location += Velocity * DeltaTime;

	SetActorLocation(Location);
}

void APotatoProjectile::Launch(FVector Direction)
{

	if (Mesh)
	{
		//FVector LaunchDirection = GetActorLocation() - GetActorLocation().RightVector;
		Direction.Normalize();
		Mesh->AddImpulse(Direction * Speed , NAME_None, true);
		//Mesh->SetLinearDamping(10.0f* Gravity);

	}
}

void APotatoProjectile::OnHit(AActor* HitActor)
{

}

void APotatoProjectile::Explode()
{

}