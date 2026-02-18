#include "PotatoWeapon.h"
#include "DrawDebugHelpers.h"

APotatoWeapon::APotatoWeapon()
{
 	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void APotatoWeapon::BeginPlay()
{
	Super::BeginPlay();
}

FVector APotatoWeapon::GetMuzzleLocation() const
{
	if (WeaponMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		return WeaponMesh->GetSocketLocation(TEXT("Muzzle"));
	}
	return GetActorLocation() + GetActorForwardVector() * 50.0f; // Fallback
}