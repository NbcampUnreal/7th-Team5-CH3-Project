// PotatoMonsterSpecialSkillPresetRow.h (추가 필드 포함 버전)
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

USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSkillVFXSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TSoftObjectPtr<UNiagaraSystem> Niagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TSoftObjectPtr<UParticleSystem> Cascade = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector Scale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName AttachSocket = NAME_None;

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

USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSkillPresentation
{
	GENERATED_BODY()

public:
	// ============================================================
	// Execute-only mode (default)
	// - bEnablePhases = false (default) 이면
	//   Telegraph/Cast/End 슬롯은 에디터에서 숨김(= 사실상 막힘)
	// - 필요해지면 true로 켜서 단계별 프레젠테이션을 다시 사용 가능
	// ============================================================

	// (에디터용) 단계별 슬롯 잠금 해제 토글
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Advanced", meta=(InlineEditConditionToggle))
	bool bEnablePhases = false;

	// -------------------------
	// Execute (항상 사용)
	// -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Execute")
	FPotatoSkillVFXSlot ExecuteVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Execute")
	FPotatoSkillSFXSlot ExecuteSFX;

	// -------------------------
	// Telegraph (잠금)
	// -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Telegraph",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillVFXSlot TelegraphVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Telegraph",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillSFXSlot TelegraphSFX;

	// -------------------------
	// Cast (잠금)
	// -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Cast",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillVFXSlot CastVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Cast",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillSFXSlot CastSFX;

	// -------------------------
	// End (잠금)
	// -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|End",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillVFXSlot EndVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|End",
		meta=(EditCondition="bEnablePhases", EditConditionHides))
	FPotatoSkillSFXSlot EndSFX;
};

// ============================================================
// ✅ NEW: Spawn mode for Execution=Projectile(SpawnActor) unification
// ============================================================

UENUM(BlueprintType)
enum class EPotatoSkillSpawnMode : uint8
{
	// 발사체(방향/속도/스윕/OnHit 기반)
	Projectile,

	// 설치형: Owner 위치에 스폰 (불기둥 같은)
	AtOwner,

	// 설치형: Target 위치에 스폰 (장판/불기둥)
	AtTarget,

	// 설치형: Target 앞 Range 지점에 스폰
	ForwardOffset
};

// ============================================================

USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoMonsterSpecialSkillPresetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialLogic Logic = EMonsterSpecialLogic::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialTrigger Trigger = EMonsterSpecialTrigger::OnCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape")
	EMonsterSpecialShape Shape = EMonsterSpecialShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	EMonsterSpecialExecution Execution = EMonsterSpecialExecution::None;

	// Target / Gating
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

	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float Cooldown = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float CastTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0"))
	float TelegraphTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing")
	bool bExecuteOnNextTick = true;

	// Shape params
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float Radius = 0.f;

	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float AngleDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float Range = 0.f;*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape")
	bool bHitOncePerTarget = true;

	// Effect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	float DamageMultiplier = 1.f;

	// DOT
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotDps = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT", meta=(ClampMin="0"))
	float DotTickInterval = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|DOT")
	EMonsterDotStackPolicy DotStackPolicy = EMonsterDotStackPolicy::RefreshDuration;

	// CC
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|CC", meta=(ClampMin="0"))
	float StunDuration = 0.f;*/

	// ============================================================
	// ✅ NEW: Projectile(독침) / SpawnActor(불기둥) 공용 스폰 세팅
	// ============================================================

	/**
	 * Execution=Projectile일 때 스폰되는 클래스
	 * - 진짜 투사체(APotatoMonsterProjectile)도 가능
	 * - 설치형(APotatoFirePillarActor)도 가능
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn")
	TSoftClassPtr<AActor> ProjectileClass; // 이름 유지(호환)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn")
	EPotatoSkillSpawnMode SpawnMode = EPotatoSkillSpawnMode::Projectile;

	// 소켓/오프셋(Owner 기준)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn")
	FName SpawnSocket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn")
	FVector SpawnOffset = FVector::ZeroVector;

	// 설치형을 바닥에 붙이기(불기둥 추천)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn|Ground")
	bool bSnapToGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn|Ground", meta=(EditCondition="bSnapToGround", ClampMin="0"))
	float GroundTraceDistance = 5000.f;

	/**
	 * SpawnMode=AtTarget일 때
	 * - true  : Owner → Target 방향을 바라보게(Yaw only)
	 * - false : Owner 회전 유지
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn|Rotation")
	bool bFaceTargetOnSpawn = false;

	/**
	 * Yaw만 적용할지 (Pitch/Roll 제거)
	 * - 일반적으로 true 권장 (장판/불기둥/투사체 모두 안전)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn|Rotation",
		meta=(EditCondition="bFaceTargetOnSpawn"))
	bool bYawOnlyRotation = true;
	
	// ============================================================
	// ✅ NEW: Split (HP threshold based)
	// - Logic == Split 일 때 사용
	// - Execution == SummonSplit 로 같이 걸어도 됨
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	bool bEnableSplit = false;

	// ex) 0.6, 0.3  (내림차순 권장)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	TArray<float> SplitThresholdPercents;

	// MaxHP가 이 값 미만이면 Split 금지(자연스럽게 멈춤)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.0"))
	float SplitMinMaxHpToAllow = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0"))
	int32 SplitMaxDepth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="1"))
	int32 SplitSpawnCount = 1;

	// Split 시 본체 스케일 곱
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.01"))
	float SplitOwnerScaleMultiplier = 0.85f;

	// 자식 MaxHP = 부모 MaxHP * 비율 (자식은 풀피로 시작)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.01"))
	float SplitChildMaxHpRatio = 0.65f;

	// 스폰 분산(겹침 방지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.0"))
	float SplitSpawnJitterRadius = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	float SplitSpawnZOffset = 10.f;
	
	// ============================================================
	// ✅ NEW: Projectile 이동/충돌(독침)
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0"))
	float ProjectileSpeed = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0"))
	float ProjectileLifeSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0"))
	float ProjectileTraceRadius = 12.f;

	// ============================================================
	// ✅ NEW: Projectile 폭발 AoE DOT(독침 광역)
	// ============================================================

	// 0이면 단일 OnHit DOT
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Explode", meta=(ClampMin="0"))
	float ExplodeRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Explode")
	bool bExplodeAffectsStructures = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Explode", meta=(ClampMin="0"))
	int32 ExplodeMaxTargets = 0; // 0이면 제한 없음

	// ============================================================
	// ✅ NEW: Spawned Actor 옵션(불기둥)
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Life", meta=(ClampMin="0"))
	float SpawnedLifeTime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|DOT")
	bool bSpawnedPlayerOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Move")
	bool bSpawnedEnableMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Move", meta=(ClampMin="0"))
	float SpawnedMoveSpeed = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Move", meta=(ClampMin="0"))
	float SpawnedWanderRadius = 500.f;

	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Move", meta=(ClampMin="0.01"))
	float SpawnedRepathInterval = 0.8f;*/


	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 VfxCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 SfxCost = 0;*/
		
		// Budget / FX Gate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	float MaxFxDistance = 0.f;

	// Presentation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FPotatoSkillPresentation Presentation;
	
	// Skill Anim (optional override)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Anim")
	TSoftObjectPtr<UAnimMontage> AttackMontageOverride;

	/*// 기본 공격 몽타주로 폴백할지(공격형 스킬만 true 추천)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Anim")
	bool bFallbackToBasicAttackMontage = true;

	// 텔레/캐스트/실행 중 언제 몽타주를 틀지 (기본: 캐스트 시작)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Anim")
	bool bPlayMontageAtCastBegin = true;*/
};