#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoMonsterFinalStats.h"
#include "PotatoCombatComponent.generated.h"

class AActor;
class UAnimMontage;
class UPotatoMonsterAnimSet;
class APotatoMonster;

UCLASS(ClassGroup = (Potato), meta = (BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoCombatComponent();

	// Stats(=FinalStats) 주입
	void InitFromStats(const FPotatoMonsterFinalStats& InStats);

	// 공격 요청 (BT/AI에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	bool RequestBasicAttack(AActor* Target);

	// (근접) AnimNotify에서 호출: PendingTarget에 데미지 적용
	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	void ApplyPendingBasicDamage();

	// (원거리) AnimNotify에서 호출: PendingTarget으로 투사체 발사
	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	void FirePendingRangedProjectile();

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	void EndBasicAttack();

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	bool IsAttacking() const { return bIsAttacking; }

	// 현재 정책 유지: 공격 간격에 스케일/여유를 더함
	UPROPERTY(EditAnywhere, Category = "Potato|Combat")
	float AttackIntervalScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat")
	float AttackIntervalExtra = 0.05f;

	// 몽타주 종료 콜백 (안전하게 공격상태 해제)
	UFUNCTION()
	void OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 사거리 여유 (Bounds 표면 거리 근사에 더함)
	UPROPERTY(EditAnywhere, Category = "Potato|Combat|Range")
	float AttackRangePadding = 60.f;

	UPROPERTY(Transient)
	bool bQueuedApplyDamageNextTick = false;

	UPROPERTY(Transient)
	bool bQueuedFireProjectileNextTick = false;

	const UPotatoMonsterAnimSet* GetAnimSet() const;
private:
	// =====================
	// State
	// =====================
	// FinalStats 캐시 (PresetApplier에서 주입)
	UPROPERTY(Transient)
	FPotatoMonsterFinalStats Stats;

	bool bIsAttacking = false;
	double NextAttackTime = 0.0;

	TWeakObjectPtr<AActor> PendingBasicTarget;

	// =====================
	// Helpers
	// =====================


	bool IsTargetInRange(AActor* Target) const;
	bool IsStructureTarget(AActor* Target) const;

	// (근접) 실제 데미지 적용
	void ApplyBasicDamage(AActor* Target) const;

	// (원거리) 투사체 스폰
	void SpawnProjectileToTarget(AActor* Target) const;

	// muzzle 위치 계산
	bool GetMuzzleTransform(FVector& OutLoc, FRotator& OutRot) const;

	// =====================
	// OnAttack Special Proc (정석)
	// - Proc gate/Chance/Cooldown/SkillId/Ready 체크는 SpecialSkillComponent 내부가 담당
	// - Combat은 "공격 시작 직전" 한 번 호출만 한다.
	// =====================
	void TryProcOnAttackSpecial(APotatoMonster* Monster, AActor* Target, double Now);

	// =====================
	// SFX control (per-owner)
	// =====================
	double LastAttackStartSFXTime = -1.0;
	double LastAttackHitSFXTime = -1.0;
	double LastProjectileFireSFXTime = -1.0;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "0.0"))
	float AttackStartSFXCooldown = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "0.0"))
	float AttackHitSFXCooldown = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "0.0"))
	float ProjectileFireSFXCooldown = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX")
	float AttackSFXNearDistance = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX")
	float AttackSFXFarDistance = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackSFXFarChance = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "0.0"))
	float CombatSFXGlobalWindowSec = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Potato|Combat|SFX", meta = (ClampMin = "1"))
	int32 CombatSFXGlobalMaxCount = 4;
};