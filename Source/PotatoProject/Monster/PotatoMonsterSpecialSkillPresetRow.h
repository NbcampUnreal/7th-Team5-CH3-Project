#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterSpecialSkillPresetRow.generated.h"

/**
 * MonsterSpecialSkillPresetTable
 * - RowName = SkillId
 * - Logic/Trigger/Shape는 유지
 * - Execution(실행 방식)만 추가해서 자동 실행을 안정화
 */
USTRUCT(BlueprintType)
struct FPotatoMonsterSpecialSkillPresetRow : public FTableRowBase
{
	GENERATED_BODY()

	// =========================
	// 기존: 무엇/언제/모양
	// =========================

	/** 어떤 로직인지 (콘텐츠 키) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialLogic Logic = EMonsterSpecialLogic::None;

	/** 언제 발동하는지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialTrigger Trigger = EMonsterSpecialTrigger::OnCooldown;

	/** 판정 모양(원/부채꼴/라인 등). 기존 유지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape")
	EMonsterSpecialShape Shape = EMonsterSpecialShape::None;

	// =========================
	// 추가: 어떻게 실행할지(Execution)
	// =========================

	/** 실행 방식(권장: 반드시 지정). None이면 Shape에서 역추론(하위호환) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialExecution Execution = EMonsterSpecialExecution::None;

	// =========================
	// Target / Gating
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	EMonsterSpecialTargetType TargetType = EMonsterSpecialTargetType::CurrentTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	bool bRequireLineOfSight = false;

	/** 기존 TriggerRange 유지: OnNearTarget 등에서 쓰는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger", meta=(ClampMin="0"))
	float TriggerRange = 0.f;

	/** 실행 가능 거리 하한/상한(0이면 무시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger", meta=(ClampMin="0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger", meta=(ClampMin="0"))
	float MaxRange = 0.f;

	// =========================
	// Timing
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float Cooldown = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float CastTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float TelegraphTime = 0.f;

	/** 안정화: 실제 판정/스폰은 기본 NextTick에서 실행 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing")
	bool bExecuteOnNextTick = true;

	// =========================
	// Shape params
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float AngleDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape")
	bool bHitOncePerTarget = true;

	// =========================
	// Effect
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotDps = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotTickInterval = 0.f; // 0이면 기본값 사용

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT")
	EMonsterDotStackPolicy DotStackPolicy = EMonsterDotStackPolicy::RefreshDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|CC", meta=(ClampMin="0"))
	float StunDuration = 0.f;

	// =========================
	// Projectile
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	TSoftClassPtr<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	FName SpawnSocket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	FVector SpawnOffset = FVector::ZeroVector;

	// =========================
	// Budget / FX Gate
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	float MaxFxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 VfxCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 SfxCost = 0;

	// =========================
	// Presentation
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TSoftObjectPtr<UObject> VFX;
};