#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterAnimSet.h"
#include "PotatoMonsterFinalStats.generated.h"
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoSplitSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	TArray<float> ThresholdPercents; // 0.6, 0.3 ...

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.0"))
	float MinMaxHpToAllowSplit = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0"))
	int32 MaxDepth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="1"))
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.01"))
	float OwnerScaleMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.01"))
	float ChildMaxHpRatio = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split", meta=(ClampMin="0.0"))
	float SpawnJitterRadius = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	float SpawnZOffset = 10.f;
};

USTRUCT(BlueprintType)
struct FPotatoMonsterFinalStats
{
	GENERATED_BODY()

	// =========================
	// Core Stats
	// =========================
	UPROPERTY(BlueprintReadOnly) float MaxHP = 100.f;
	UPROPERTY(BlueprintReadOnly) float AttackDamage = 10.f;
	UPROPERTY(BlueprintReadOnly) float AttackRange = 150.f;
	UPROPERTY(BlueprintReadOnly) float MoveSpeed = 300.f;

	UPROPERTY(BlueprintReadOnly) float AppliedHpMultiplier = 1.f;
	UPROPERTY(BlueprintReadOnly) float AppliedMoveSpeedRatio = 0.6f;
	UPROPERTY(BlueprintReadOnly) float StructureDamageMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly) bool bIsRanged = false;

	// =========================
	// AI / Anim
	// =========================
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim")
	TObjectPtr<UPotatoMonsterAnimSet> AnimSet = nullptr;

	// =========================
	// Special Skill: Single Binding (Type/Rank에서 주입)
	// - SkillId는 SpecialSkillPresetTable RowName과 동일
	// =========================
	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Bind")
	FName DefaultSpecialSkillId = NAME_None;

	// =========================
	// Special Skill: Rank-wide tuning (Rank에서 주입)
	// - 스킬 Row의 Cooldown / DamageMultiplier에 곱해지는 값
	// =========================
	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Tuning", meta=(ClampMin="0.01"))
	float SpecialCooldownScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Tuning", meta=(ClampMin="0.0"))
	float SpecialDamageScale = 1.0f;

	// =========================
	// Optional: Single Skill Cache
	// - 기존 코드/BTService가 “단일 스킬”을 쉽게 참조하도록 유지
	// - 실제 실행은 SkillComponent가 Row를 직접 읽어도 OK
	// =========================
	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Cache")
	EMonsterSpecialLogic DefaultSpecialLogic = EMonsterSpecialLogic::None;

	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Cache")
	float DefaultSpecialCooldown = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Potato|Stats|Special|Cache")
	float DefaultSpecialDamageMultiplier = 1.f;

	// =========================
	// Special Proc (OnAttack)
	// - “평타는 유지 + 가끔 스페셜 Proc” 정책이면 유지
	// - 단일 DefaultSpecialSkillId를 Proc에도 쓰는 구조로 운용 가능
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Stats|Special Proc")
	bool bEnableOnAttackSpecialProc = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Stats|Special Proc", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OnAttackSpecialChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Stats|Special Proc", meta=(ClampMin="0.0"))
	float OnAttackSpecialProcCooldown = 1.50f;
	
	// =========================
	// HardenShell (Data-driven)
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell")
	bool bEnableHardenShell = false;

	// 데미지 배율(0.5면 50%만 받음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.0"))
	float HardenDamageMultiplier = 0.5f;

	// 발동 스텝(0.10 = 10% 단위)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.01", ClampMax="1.0"))
	float HardenTriggerStepPercent = 0.10f;

	// 유지시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.01"))
	float HardenDurationSeconds = 10.0f;

	// 머티리얼 파라미터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell|Material")
	FName HardenTintStrengthParamName = TEXT("TintStrength");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell|Material")
	float HardenTintStrengthValue = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	bool bEnableSplit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Split")
	FPotatoSplitSpec SplitSpec;
	
	// =========================
	// Gimmick: Aura Damage (접촉 데미지)
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage")
	bool bEnableAuraDamage = false;

	// 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.0"))
	float AuraRadius = 140.f;

	// 초당 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.0"))
	float AuraDps = 15.f;

	// 틱 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.01"))
	float AuraTickInterval = 0.25f;

	// 특정 태그만(비우면 전체)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage")
	TArray<FName> AuraRequiredTargetTags;
};