#include "PotatoWeapon.h"
#include "DrawDebugHelpers.h"

APotatoWeapon::APotatoWeapon()
{
 	PrimaryActorTick.bCanEverTick = true;
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
	
	TargetKickLocation = FVector::ZeroVector;
	TargetKickRotation = FRotator::ZeroRotator;
	CurrentKickLocation = FVector::ZeroVector;
	CurrentKickRotation = FRotator::ZeroRotator;
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

void APotatoWeapon::PlayKick(const FVector& KickOffset, const FRotator& KickRotation, float RecoverySpeed)
{
	CurrentKickLocation = KickOffset;
	CurrentKickRotation = KickRotation;
	CurrentRecoverySpeed = RecoverySpeed;
}

void APotatoWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!CurrentKickLocation.IsNearlyZero() || !CurrentKickRotation.IsNearlyZero())
	{
		CurrentKickLocation = FMath::VInterpTo(CurrentKickLocation, FVector::ZeroVector, DeltaTime, CurrentRecoverySpeed);
		CurrentKickRotation = FMath::RInterpTo(CurrentKickRotation, FRotator::ZeroRotator, DeltaTime, CurrentRecoverySpeed);
		SetActorRelativeLocation(CurrentKickLocation);
		SetActorRelativeRotation(CurrentKickRotation);
	}
}
