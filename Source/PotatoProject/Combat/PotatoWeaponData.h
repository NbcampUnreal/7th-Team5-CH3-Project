#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PotatoWeaponData.generated.h"

class APotatoProjectile;
class APotatoWeapon;

UENUM(BlueprintType)
enum class EWeaponFireType : uint8
{
	Projectile UMETA(DisplayName = "Projectile"),
	Hitscan UMETA(DisplayName = "Hitscan")
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
	
	/** 발사 타입: 투사체 또는 히트스캔 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	EWeaponFireType FireType;
	
	/** 스폰할 실제 액터: BP_FarmCannon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<APotatoWeapon> WeaponActorClass;
	
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