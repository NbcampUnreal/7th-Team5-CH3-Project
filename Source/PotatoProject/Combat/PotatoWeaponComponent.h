#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/PotatoEnums.h"
#include "PotatoWeaponComponent.generated.h"

class UPotatoWeaponData;
class APotatoWeapon;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponChangedDelegate, const UPotatoWeaponData* /*NewWeaponData*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedDelegate, int32 /*CurrentAmmo*/, int32 /*ReserveAmmo*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyHitDelegate, bool /*bIsKill*/);

USTRUCT(BlueprintType)
struct FWeaponAmmoState
{
    GENERATED_BODY()

    /** 현재 탄창에 장전된 탄약 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 CurrentAmmo = 0;

    /** 예비 탄약 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ReserveAmmo = 0;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPotatoWeaponComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // =================================================================
    // Weapon Inventory & Equipment
    // =================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EquipWeapon(int32 SlotIndex);

public:
    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    TArray<TObjectPtr<UPotatoWeaponData>> WeaponSlots;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    int32 CurrentWeaponIndex = 0;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<UPotatoWeaponData> CurrentWeaponData;

    /** 현재 들고 있는 무기 액터 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<APotatoWeapon> CurrentWeaponActor;

    // =================================================================
    // Combat Interface & Settings (Public)
    // =================================================================
public:
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Fire();
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartReload();
    
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool CanFire() const;
    
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsReloading() const;
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsInCombatStance() const;
    
    UFUNCTION(BlueprintPure, Category = "Combat")
    float GetLastFireTime() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    EWeaponState CurrentState = EWeaponState::Idle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    float ReloadWalkSpeedScale = 0.7;
    
    void UpdateCachedWalkSpeed(float NewSpeed);
    
    /** 현재 탄약 상태를 브로드캐스트하기 위한 헬퍼 함수 */
    void BroadcastAmmoState();
    
    /** 현재 이동/점프/발사 상태가 반영된 실제 탄착군 분산 각도(degree) */
    float GetCurrentSpreadAngle() const;

    FOnWeaponChangedDelegate OnWeaponChanged;
    FOnAmmoChangedDelegate OnAmmoChanged;
    FOnEnemyHitDelegate OnEnemyHit;
    
    // =================================================================
    // Ammo System
    // =================================================================
public:
    /** DataAsset(Key) -> 탄약 상태(Value) 매핑 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo")
    TMap<TObjectPtr<UPotatoWeaponData>, FWeaponAmmoState> AmmoMap;
    
    /** 현재 무기에 예비 탄약 추가: 탄약 텍스트 업데이트를 위해 추가 후 예비 탄약 개수 반환 */
    UFUNCTION(BlueprintCallable, Category = "Ammo")
    void AddAmmoToWeapon(UPotatoWeaponData* TargetWeapon, int32 Amount);

private:
    void InitializeAmmoMap();

    // =================================================================
    // Internal Combat Logic & Implementation
    // =================================================================
protected:
    void FinishReload();
    void CancelReload();

private:
    void SpawnWeapon(TSubclassOf<APotatoWeapon> NewClass);

    /** 물리 투사체 발사 로직: 감자, 옥수수, 호박 */
    void FireProjectile(const FVector& TargetLocation);

    /** 탄도학 적용 발사 로직 */
    void FireProjectileWithBallistics(const FVector& TargetLocation);

    /** 히트스캔 발사 로직: 당근 */
    void FireHitscan(const FVector& TargetLocation);

    /** 히트스캔 시각 효과 스폰: 당근 */
    void SpawnHitscanVisual(const FHitResult& HitResult, const FVector& ShotDirection);
    
    /** 사운드, 파티클, 카메라 흔들림 및 반동 처리 */
    void PlayFireEffects();
    
    float GetRandomPitch(float SoundPitchRandomness);

    FVector GetMuzzleLocation() const;
    FVector GetCrosshairTargetLocation() const;

private:
    FTimerHandle ReloadTimerHandle;
    
    /** 마지막 발사 시간 추적 */
    float LastFireTime = -10.0f;
    
    float CachedWalkSpeed = 0.0f;
    
    /** ControlRotation Recoil: 남은 Pitch 반동량 */
    float TargetRecoilPitch = 0.0f;
    
    /** ControlRotation Recoil: 남은 Yaw 반동량 */
    float TargetRecoilYaw = 0.0f;
};
