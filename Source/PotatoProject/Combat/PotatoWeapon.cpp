#include "PotatoWeapon.h"

APotatoWeapon::APotatoWeapon()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void APotatoWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void APotatoWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotatoWeapon::Fire()
{

}

bool APotatoWeapon::Reload()
{
	return false;
}

bool APotatoWeapon::CanFire()
{
	return false;
}