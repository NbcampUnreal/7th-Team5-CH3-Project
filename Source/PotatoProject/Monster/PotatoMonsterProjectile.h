// PotatoMonsterProjectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectileDamageable.h"
#include "PotatoMonsterProjectile.generated.h"

class UNiagaraComponent;

/**
 * APotatoMonsterProjectile
 * - 기본: 몬스터 원거리 평타(단일 ApplyDamage)
 * - 확장: 스킬 모드(독침) = OnHit DOT, (옵션) 폭발 AoE DOT
 *
 * ✅ 안정화 포인트
 * - World teardown / Destroyed 가드
 * - Authority에서만 DOT/데미지 적용(구조물 TakeDamage Authority 요구 대응)
 * - DotComponent 생성 시 RegisterComponentWithWorld 사용
 * - 폭발 AoE는 Pawn + WorldDynamic Overlap + (구조물은 TActorIterator + AABB 거리 체크로 보강)
 */
UCLASS()
class POTATOPROJECT_API APotatoMonsterProjectile
	: public AActor
	, public IPotatoProjectileDamageable
{
	GENERATED_BODY()

public:
	APotatoMonsterProjectile();

	// 데미지 주입 (CombatComponent에서 호출)
	virtual void SetProjectileDamage_Implementation(float InDamage) override;

	// 스폰 직후 방향/속도 세팅
	void InitVelocity(const FVector& InDirection, float InSpeed);

	// ============================================================
	// Skill Mode (독침)
	// - InitSkillDot를 호출하면 "스킬 투사체"로 동작
	// - ExplodeRadius > 0 이면 폭발 AoE DOT
	// ============================================================
	UFUNCTION(BlueprintCallable, Category="Potato|Projectile|Skill")
	void InitSkillDot(float InDotDps, float InDotDuration, float InDotTickInterval, float InExplodeRadius);

	UFUNCTION(BlueprintCallable, Category="Potato|Projectile|Skill")
	bool IsSkillDotMode() const { return bSkillDotMode; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool ShouldIgnore(AActor* Other) const;
	void DoSweep(const FVector& From, const FVector& To);

	// ---- Skill helpers ----
	bool IsWorldSafe() const;
	bool IsActorSafe(AActor* A) const;

	void ApplyDirectDamage(AActor* Victim);
	void ApplyDotToActor(AActor* Victim);
	void ExplodeApplyDot(const FVector& Origin);

	// 구조물 AoE 보강: Overlap로 안 잡히는 구조물도 거리 체크로 포함
	void GatherStructuresInRadius_AABB(const FVector& Origin, float Radius, TArray<AActor*>& OutVictims) const;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> VFX;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float Speed = 1800.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float TraceRadius = 12.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float LifeSeconds = 5.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldDynamic;

	UPROPERTY(EditAnywhere, Category="Projectile")
	bool bIgnoreAllMonsters = true;

	// 기본(평타) 데미지
	float Damage = 0.f;

	FVector MoveDir = FVector::ForwardVector;
	bool bHitOnce = false;

	// -------------------------
	// Skill DOT Mode runtime
	// -------------------------
	UPROPERTY(VisibleAnywhere, Category="Skill")
	bool bSkillDotMode = false;

	UPROPERTY(VisibleAnywhere, Category="Skill")
	float DotDps = 0.f;

	UPROPERTY(VisibleAnywhere, Category="Skill")
	float DotDuration = 0.f;

	UPROPERTY(VisibleAnywhere, Category="Skill")
	float DotTickInterval = 0.2f;

	// 폭발형이면 > 0
	UPROPERTY(VisibleAnywhere, Category="Skill")
	float ExplodeRadius = 0.f;

	// 구조물 AoE 보강 ON/OFF
	UPROPERTY(EditAnywhere, Category="Skill")
	bool bIncludeStructuresInExplode = true;
};