#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterAnimSet.h"
#include "PotatoMonsterFinalStats.generated.h"

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gimmick|HardenShell")
	bool bEnableHardenShell = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gimmick|HardenShell", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HardenTriggerHpPercent = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gimmick|HardenShell", meta=(ClampMin="0.0"))
	float HardenDamageMultiplier = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gimmick|HardenShell")
	FLinearColor HardenTint = FLinearColor(0.4f, 0.4f, 0.45f, 1.f);
};