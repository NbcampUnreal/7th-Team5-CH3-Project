#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoMonsterFinalStats.h"
#include "PotatoCombatComponent.generated.h"

class UAnimMontage;
class UPotatoMonsterAnimSet;

UCLASS(ClassGroup = (Potato), meta = (BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoCombatComponent();

	// Stats(=FinalStats) 주입
	void InitFromStats(const FPotatoMonsterFinalStats& InStats);

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

	UFUNCTION()
	void OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditAnywhere, Category = "Combat|Range")
	float AttackRangePadding = 60.f;   // ★ 30~80 추천

private:
	FPotatoMonsterFinalStats Stats;

	bool bIsAttacking = false;
	double NextAttackTime = 0.0;
	TWeakObjectPtr<AActor> PendingBasicTarget;

	// ===== Helpers =====
	const UPotatoMonsterAnimSet* GetAnimSet() const;

	bool IsTargetInRange(AActor* Target) const;
	bool IsStructureTarget(AActor* Target) const;

	// (근접) 실제 데미지 적용
	void ApplyBasicDamage(AActor* Target) const;

	// (원거리) 투사체 스폰
	void SpawnProjectileToTarget(AActor* Target) const;

	// muzzle 위치 계산
	bool GetMuzzleTransform(FVector& OutLoc, FRotator& OutRot) const;
};
