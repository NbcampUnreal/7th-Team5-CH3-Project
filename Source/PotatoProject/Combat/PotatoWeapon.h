#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoWeapon.generated.h"

class APotatoProjectile;
class UPotatoWeaponSystem;

UCLASS()
class POTATOPROJECT_API APotatoWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoWeapon();

protected:
	virtual void BeginPlay() override;

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType Type;
	//공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage;
	//최대탄약수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int MagazineSize;
	//현재 장전 총알 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int CurrentAmmo;
	//샷1번당 소모량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int CropCostPerShot;
	//발사속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate;
	//곡사 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Fireangle;



	//원본 투사체
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	APotatoProjectile* Projectile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<TSubclassOf<APotatoProjectile>> ProjectileOrigins;
	UPROPERTY()
	UPotatoWeaponSystem* WeaponSystem;

	//유효사거리
	float ValidDistance = 5000.0f;

	virtual void Tick(float DeltaTime) override;

	void Fire();
	bool Reload();
	bool CanFire();
	void ChangeWeapon(int index);
	void SetProjectileType(APotatoProjectile* Projectile);
};
