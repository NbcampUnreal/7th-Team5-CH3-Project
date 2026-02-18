#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/PotatoEnums.h"
#include "PotatoWeaponComponent.generated.h"

class UPotatoWeaponData;
class APotatoWeapon;

USTRUCT(BlueprintType)
struct FWeaponAmmoState
{
	GENERATED_BODY()
	
	/** 현재 장전된 탄약 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentAmmo = 0;
	
	/** 농작물로 제작된 예비 탄약 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 ReserveAmmo = 0;
	
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedDelegate, int32, CurrentAmmo, int32, CurrentReserveAmmo);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UPotatoWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPotatoWeaponComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TObjectPtr<UPotatoWeaponData>> WeaponSlots;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentWeaponIndex = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UPotatoWeaponData> CurrentWeaponData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<APotatoWeapon> CurrentWeaponActor;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EWeaponState CurrentState = EWeaponState::Idle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	TMap<TObjectPtr<UPotatoWeaponData>, FWeaponAmmoState> AmmoState;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(int32 SlotIndex);
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool CanFire() const;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Fire();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoChangedDelegate OnAmmoChanged;
	
private:
	FVector GetMuzzleLocation() const;
	FVector GetCrosshairTargetLocation() const;
	
	/** 물리적인 투사체(감자, 옥수수, 호박)를 스폰하는 로직을 처리 */
	void FireProjectile(const FVector& TargetLocation);
	
	/** 히트스캔 로직(당근)을 처리 */
	void FireHitscan(const FVector& TargetLocation);
	
	/** 시각적 당근 Actor를 스폰 */
	void SpawnHitscanVisual(const FHitResult& HitResult, const FVector& ShotDirection);
	
	void SpawnWeapon(TSubclassOf<APotatoWeapon> NewClass);
};
