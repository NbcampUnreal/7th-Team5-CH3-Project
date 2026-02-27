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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float AngleDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shape", meta=(ClampMin="0"))
	float Range = 0.f;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect|CC", meta=(ClampMin="0"))
	float StunDuration = 0.f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnedActor|Move", meta=(ClampMin="0.01"))
	float SpawnedRepathInterval = 0.8f;

	// Budget / FX Gate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	float MaxFxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 VfxCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Budget", meta=(ClampMin="0"))
	int32 SfxCost = 0;

	// Presentation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FPotatoSkillPresentation Presentation;
};