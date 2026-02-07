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

