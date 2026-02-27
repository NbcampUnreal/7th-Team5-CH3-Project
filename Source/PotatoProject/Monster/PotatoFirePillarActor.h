#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// ✅ enum 정의 들어있는 헤더를 반드시 include (forward declare 금지 권장)
#include "../Core/PotatoEnums.h"

#include "PotatoFirePillarActor.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystemComponent;
class UParticleSystem;

struct FPotatoSkillVFXSlot; // ✅ Row의 VFX 슬롯(헤더 include는 cpp에서)

UCLASS()
class POTATOPROJECT_API APotatoFirePillarActor : public AActor
{
	GENERATED_BODY()

public:
	APotatoFirePillarActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void EndLife();
	void TickDamage();
	void PickNewMoveTarget();

	bool ShouldAffectActor(AActor* Other) const;

	// 구조물은 collision overlap이 안 잡힐 수 있어서 AABB iterator로 보강(옵션)
	void GatherStructuresInRadius_AABB(const FVector& Origin, float Radius, TArray<AActor*>& OutVictims) const;

	// VFX 슬롯을 FirePillar 내부 컴포넌트(Niagara/Cascade)에 적용
	void ApplyVfxFromSlot(const FPotatoSkillVFXSlot& Slot);

public:
	/**
	 * ✅ 스폰 직후 한 번에 주입 (Row 기반)
	 * - DOT: 대상에 UPotatoDotComponent 찾아 ApplyDot
	 * - VFX: Niagara/Cascade 둘 다 지원 (Row 슬롯 우선, 없으면 BP 디폴트 템플릿 사용)
	 */
	void InitPillar(
		AActor* InDamageCauser,
		AController* InInstigator,

		// DOT
		float InDps,
		float InDuration,
		float InTickInterval,
		EMonsterDotStackPolicy InPolicy,
		float InRadius,

		// Options
		bool bInPlayerOnly,
		bool bInEnableMove,
		float InLifeTime,
		float InMoveSpeed,
		float InWanderRadius,
		float InRepathInterval,

		// VFX override slot (optional)
		const FPotatoSkillVFXSlot* InVfxSlot = nullptr
	);

protected:
	// -------------------------
	// Components
	// -------------------------
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> DamageSphere;

	// ✅ Niagara 기본
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|VFX")
	TObjectPtr<UNiagaraSystem> NiagaraTemplate = nullptr;

	// ✅ Cascade 추가 지원
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent> CascadeComp;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|VFX")
	TObjectPtr<UParticleSystem> CascadeTemplate = nullptr;

	// -------------------------
	// Settings (Defaults)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category="Pillar|Life", meta=(ClampMin="0.1"))
	float LifeTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|DOT", meta=(ClampMin="0.05"))
	float TickInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|DOT")
	bool bPlayerOnly = true;

	// 구조물도 태울지 (Row에서 bPlayerOnly=false로 주면 사실상 구조물 포함인데,
	// 구조물 overlap이 안 잡히는 케이스 보강용)
	UPROPERTY(EditDefaultsOnly, Category="Pillar|DOT")
	bool bIncludeStructuresAABB = true;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|DOT", meta=(ClampMin="0"))
	float DamageRadius = 220.f;

	// Move
	UPROPERTY(EditDefaultsOnly, Category="Pillar|Move")
	bool bEnableMove = true;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|Move", meta=(EditCondition="bEnableMove", ClampMin="0"))
	float MoveSpeed = 220.f;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|Move", meta=(EditCondition="bEnableMove", ClampMin="0"))
	float WanderRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Pillar|Move", meta=(EditCondition="bEnableMove", ClampMin="0.05"))
	float RepathInterval = 0.8f;

private:
	// -------------------------
	// Runtime injected
	// -------------------------
	UPROPERTY()
	TObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY()
	TObjectPtr<AController> InstigatorController = nullptr;

	float DotDps = 0.f;
	float DotDuration = 0.f;
	EMonsterDotStackPolicy DotPolicy = EMonsterDotStackPolicy::RefreshDuration;

	FTimerHandle TimerLife;
	FTimerHandle TimerTick;

	FVector SpawnCenter = FVector::ZeroVector;
	FVector MoveTarget = FVector::ZeroVector;
	float RepathAcc = 0.f;
};