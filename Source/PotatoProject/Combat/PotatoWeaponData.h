#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PotatoWeaponData.generated.h"

class APotatoProjectile;
class APotatoWeapon;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EWeaponFireType : uint8
{
	Projectile UMETA(DisplayName = "Projectile"),
	Hitscan UMETA(DisplayName = "Hitscan")
};

UENUM(BlueprintType)
enum class ECrosshairType : uint8
{
	ArcSpread UMETA(DisplayName = "ArcSpread"), // 감자
	LineSpread UMETA(DisplayName = "LineSpread"), // 옥수수
	Circle UMETA(DisplayName = "Circle"), // 당근
	Static UMETA(DisplayName = "Static"), // 호박
};

UCLASS()
class POTATOPROJECT_API UPotatoWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText WeaponName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* Icon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	ECrosshairType CrosshairType;
	
	// =================================================================
	// 공통 스탯
	// =================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float BaseDamage = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	int32 MaxAmmoSize = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float FireRate = 0.5f;
	
	/** 재장전에 걸리는 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float ReloadTime = 1.5f;
	
	/** 1개의 탄약을 만들기 위해 몇 개의 농작물이 필요한가? (e.g. 호박: 3) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	int32 AmmoCraftingCost = 1;
	
	/** 발사 타입: 투사체 또는 히트스캔 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	EWeaponFireType FireType;
	
	/** 스폰할 실제 액터: BP_FarmCannon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<APotatoWeapon> WeaponActorClass;
	
	// =================================================================
	// Visuals
	// =================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* FireMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* ReloadMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* EquipMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* FireSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* ReloadSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* EquipSound;
	
	/** 발사 사운드 랜덤 피치 변화 범위: 0.0은 변화 없음, 0.1은 0.9에서 1.1사이 무작위 값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireSoundPitchRandomness = 0.1f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EquipSoundPitchRandomness = 0.1f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* MuzzleFlash;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UCameraShakeBase> FireCameraShake;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* ImpactEffect;

	/** 나이아가라 파티클 크기 스케일링: 2.0 = 두 배 커짐 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FVector ImpactEffectScale = FVector(1.0f, 1.0f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* ImpactSound;
	
	// =================================================================
	// Game Feel
	// =================================================================

	/** 수직 반동: 카메라가 위로 튀어오르는 정도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float RecoilPitch = 0.5f;
	
	/** 수평 반동 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float RecoilYaw = 0.1f;
	
	/** 반동 속도: 높을수록 빠름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float RecoilSpeed = 40.0;
	
	/** 발사 시 무기 모델이 뒤로 밀리는 정도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	FVector WeaponKickOffset = FVector(-15.0f, 0.0f, 0.0f);
	
	/** 발사 시 무기 모델이 위로 들리는(Pitch 회전) 정도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	FRotator WeaponKickRotation = FRotator(10.0f, 0.0f, 0.0f);
	
	/** 발사 시 무기 모델이 원래 위치로 돌아오는 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float WeaponKickRecoverySpeed = 10.0f;
	
	// =================================================================
	// 투사체 설정: FireType이 Projectile일 경우 사용됨
	// =================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
	TSubclassOf<APotatoProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
	float ProjectileSpeed;
	
	/** 옥수수용: 몇 명의 적을 관통할지 결정 (기본 0 = 관통 없음) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
	int32 MaxPierceCount = 0;
	
	/** 호박용: 0보다 큰 값일 경우 접촉 시 폭발함 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
	float ExplosionRadius = 0.0f;
	
	/** 중력 스케일: 0.0 = 중력 없음, 1.0 = 표준 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
	float ProjectileGravityScale = 1.0f;
    
    /** 투사체 최대 사거리: 이 거리를 초과하면 최대 사거리 지점까지만 포물선 계산 후 낙차 (0.0 = 제한 없음) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "FireType == EWeaponFireType::Projectile", EditConditionHides))
    float ProjectileMaxRange = 3000.0f;

	// =================================================================
	// 히트스캔 설정: FireType이 Hitscan일 경우 사용됨
	// =================================================================
	
	/** 히트스캔 라인트레이스 길이 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hitscan", meta = (EditCondition = "FireType == EWeaponFireType::Hitscan", EditConditionHides))
	float EffectiveRange = 5000.0f;
	
	/** 발사 당 산탄 개수: 당근 = 8 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hitscan", meta = (EditCondition = "FireType == EWeaponFireType::Hitscan", EditConditionHides))
	int32 PelletCount = 1;
	
	/** 산탄 분산 각도(degree): 당근 = 15.0 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hitscan", meta = (EditCondition = "FireType == EWeaponFireType::Hitscan", EditConditionHides))
	float SpreadAngle = 0.0f;
	
	/** 히트스캔 적중 시 스폰할 시각적 액터 (e.g. 벽에 박히는 당근) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hitscan", meta = (EditCondition = "FireType == EWeaponFireType::Hitscan", EditConditionHides))
	TSubclassOf<AActor> HitscanActorClass;
};