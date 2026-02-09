#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoWeapon.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoWeapon();

protected:
	virtual void BeginPlay() override;

public:	
	EWeaponType Type;
	int Damage;
	int MagazineSize;
	int CurrentAmmo;
	int CropCostPerShot;
	float FireRate;

	virtual void Tick(float DeltaTime) override;

	void Fire();
	bool Reload();
	bool CanFire();
};
