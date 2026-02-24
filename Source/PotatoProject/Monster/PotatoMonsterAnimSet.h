#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "PotatoMonsterAnimSet.generated.h"

class UBlendSpace;
class UAnimMontage;
class USoundBase;

USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSFXSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX", meta = (ClampMin = "0.01"))
	float PitchMultiplier = 1.0f;

	// 거리 감쇠: 멀리 있는 소리 자동 감소/차단
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	TObjectPtr<USoundAttenuation> Attenuation = nullptr;

	// 동시재생 제한: 같은 계열 SFX가 너무 많이 울리지 않게
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	TObjectPtr<USoundConcurrency> Concurrency = nullptr;
};

// -----------------------------------------
// VFX 슬롯
// -----------------------------------------
USTRUCT(BlueprintType)
struct FPotatoVFXSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> Niagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UParticleSystem> Cascade = nullptr;

	// 기본 스케일 (에셋에서 조절)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FVector Scale = FVector(1.f, 1.f, 1.f);

	// ------------------------------------------------------------
	// AutoScale (per-slot)
	// ------------------------------------------------------------

	// 슬롯별 자동 스케일 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|AutoScale")
	bool bAutoScale = false;

	// Scale=1.0에 대응하는 "기준 Extent"
	// - 기본은 Mesh Bounds BoxExtent.Z 기준을 가정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|AutoScale",
		meta = (EditCondition = "bAutoScale", ClampMin = "1.0"))
	float RefExtent = 88.f;

	// 자동 스케일 클램프
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|AutoScale",
		meta = (EditCondition = "bAutoScale", ClampMin = "0.01"))
	float AutoScaleMin = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|AutoScale",
		meta = (EditCondition = "bAutoScale", ClampMin = "0.01"))
	float AutoScaleMax = 3.0f;

	// true면 Max3(X,Y,Z) extent를 사용, false면 Z만 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|AutoScale",
		meta = (EditCondition = "bAutoScale"))
	bool bUseMaxExtentXYZ = false;

	// 유틸용 편의 함수 (없어도 되지만 있으면 깔끔)
	FORCEINLINE bool HasAnyVFX() const
	{
		return (Niagara != nullptr) || (Cascade != nullptr);
	}
};


UCLASS(BlueprintType)
class POTATOPROJECT_API UPotatoMonsterAnimSet : public UDataAsset
{
	GENERATED_BODY()
public:
	// ---- Locomotion ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Locomotion")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace = nullptr;

	// ---- Attack ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Attack")
	TObjectPtr<UAnimMontage> BasicAttackMontage = nullptr;

	// "공속" (초 단위 공격 간격)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Attack", meta = (ClampMin = "0.01"))
	float BasicAttackInterval = 1.0f;

	// ---- Ranged ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged")
	bool bIsRanged = false;

	// 투사체 액터(또는 APotatoProjectile)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	FName MuzzleSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	FVector MuzzleOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Hit")
	TObjectPtr<UAnimMontage> HitReactMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Death")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	//효과음 관련 추가
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Attack")
	FPotatoSFXSlot AttackStartSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Attack")
	FPotatoSFXSlot AttackHitSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Ranged", meta = (EditCondition = "bIsRanged"))
	FPotatoSFXSlot ProjectileFireSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Hit")
	FPotatoSFXSlot GetHitSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Death")
	FPotatoSFXSlot DeathSFX;

	//이펙트 관련 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FPotatoVFXSlot HitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FPotatoVFXSlot DeathVFX;
};
