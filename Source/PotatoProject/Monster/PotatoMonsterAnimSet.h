#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX|Locomotion")
	FPotatoSFXSlot FootstepSFX;
};
