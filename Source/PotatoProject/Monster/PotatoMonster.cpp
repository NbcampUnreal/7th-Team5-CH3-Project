#include "PotatoMonster.h"

APotatoMonster::APotatoMonster()
{
 	PrimaryActorTick.bCanEverTick = true;

}

void APotatoMonster::BeginPlay()
{
	Super::BeginPlay();
}

void APotatoMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotatoMonster::Move()
{

}

void APotatoMonster::Attack(AActor* Target)
{

}

void APotatoMonster::TakeDamage(float Damage)
{

}

void APotatoMonster::Die()
{

}

AActor* APotatoMonster::FindTarget()
{
	return nullptr;
}
