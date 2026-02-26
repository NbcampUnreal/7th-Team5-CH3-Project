#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterSpecialSkillPresetRow.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class USoundBase;
class USoundAttenuation;
class USoundConcurrency;

/**
 * Skill SFX Slot (AnimSet의 FPotatoSFXSlot과 동일 개념 - 스킬 전용)
 */
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSkillSFXSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TSoftObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX", meta=(ClampMin="0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX", meta=(ClampMin="0.01"))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TSoftObjectPtr<USoundAttenuation> Attenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TSoftObjectPtr<USoundConcurrency> Concurrency = nullptr;

	FORCEINLINE bool HasAny() const { return Sound != nullptr; }
};

/**
 * Skill VFX Slot (AnimSet의 FPotatoVFXSlot과 동일 개념 - 스킬 전용)
 * - Niagara/Cascade 둘 중 하나만 써도 됨
 * - bAutoScale 등은 “스킬 연출에도” 동일하게 쓸 수 있게 유지
 */
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSkillVFXSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TSoftObjectPtr<UNiagaraSystem> Niagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TSoftObjectPtr<UParticleSystem> Cascade = nullptr;

	// 기본 스케일 (에셋에서 조절)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector Scale = FVector(1.f, 1.f, 1.f);

	// 어디에 스폰할지 보정
	// - 보통: 스킬 Origin(Owner 위치 또는 Target 위치) 기준
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 소켓에 붙이는 연출이 필요하면 사용 (Owner의 Mesh 소켓)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName AttachSocket = NAME_None;

	// ------------------------------------------------------------
	// AutoScale (per-slot)
	// ------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|AutoScale")
	bool bAutoScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|AutoScale",
		meta=(EditCondition="bAutoScale", ClampMin="1.0"))
	float RefExtent = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|AutoScale",
		meta=(EditCondition="bAutoScale", ClampMin="0.01"))
	float AutoScaleMin = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|AutoScale",
		meta=(EditCondition="bAutoScale", ClampMin="0.01"))
	float AutoScaleMax = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|AutoScale",
		meta=(EditCondition="bAutoScale"))
	bool bUseMaxExtentXYZ = false;

	FORCEINLINE bool HasAny() const { return (Niagara != nullptr) || (Cascade != nullptr); }
};

/**
 * 스킬 파이프라인 단계별 Presentation 묶음
 * - Telegraph / Cast / Execute / End 를 분리
 */
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSkillPresentation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Telegraph")
	FPotatoSkillVFXSlot TelegraphVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Telegraph")
	FPotatoSkillSFXSlot TelegraphSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Cast")
	FPotatoSkillVFXSlot CastVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Cast")
	FPotatoSkillSFXSlot CastSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Execute")
	FPotatoSkillVFXSlot ExecuteVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Execute")
	FPotatoSkillSFXSlot ExecuteSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|End")
	FPotatoSkillVFXSlot EndVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|End")
	FPotatoSkillSFXSlot EndSFX;
};

/**
 * MonsterSpecialSkillPresetTable
 * - RowName = SkillId
 * - Logic/Trigger/Shape 유지
 * - Execution(실행 방식) 추가
 * - Presentation(단계별 VFX/SFX) 추가
 */
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoMonsterSpecialSkillPresetRow : public FTableRowBase
{
	GENERATED_BODY()

	// =========================
	// 기존: 무엇/언제/모양
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialLogic Logic = EMonsterSpecialLogic::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialTrigger Trigger = EMonsterSpecialTrigger::OnCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape")
	EMonsterSpecialShape Shape = EMonsterSpecialShape::None;

	// =========================
	// 추가: 어떻게 실행할지(Execution)
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialExecution Execution = EMonsterSpecialExecution::None;

	// =========================
	// Target / Gating
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	EMonsterSpecialTargetType TargetType = EMonsterSpecialTargetType::CurrentTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	bool bRequireLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger", meta=(ClampMin="0"))
	float TriggerRange = 0.f;

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
	float DotTickInterval = 0.f;

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

	// 0이면 제한 없음(프로젝트 정책에 따라 해석)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	float MaxFxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 VfxCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 SfxCost = 0;

	// =========================
	// Presentation (NEW)
	// =========================

	/**
	 * 스킬 파이프라인 단계별 연출
	 * - BeginTelegraph / BeginCast / Execute / End
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FPotatoSkillPresentation Presentation;


};