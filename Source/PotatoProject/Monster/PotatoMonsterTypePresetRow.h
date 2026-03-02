#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterTypePresetRow.generated.h"

class UPotatoMonsterAnimSet;
class UBehaviorTree;

/**
 * TypePreset: “몬스터 종족/타입 고유값”
 * - 스킬의 ‘내용/연출/쿨다운/판정’은 SpecialSkillPresetRow에 있음
 * - Type은 “어떤 스킬을 어떤 트리거로 쓸지”만 지정
 */
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoMonsterTypePresetRow : public FTableRowBase
{
	GENERATED_BODY()

	// -----------------------------
	// Base Stats
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float BaseHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float BaseAttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float BaseAttackRange = 150.0f;

	// -----------------------------
	// Movement
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	float MoveSpeedMultiplier = 1.0f;

	// -----------------------------
	// AI
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	TSoftObjectPtr<UBehaviorTree> OverrideBehaviorTree;

	// -----------------------------
	// Combat (기본 원거리 여부)
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	bool bIsRanged = false;

	// -----------------------------
	// Anim
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Anim")
	TSoftObjectPtr<UPotatoMonsterAnimSet> AnimSet;

	// -----------------------------
	// Special Skills: Trigger binding 
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special")
	FName DefaultSpecialSkillId = NAME_None;

	// -----------------------------
	// OnAttack Proc override (Type)
	// -----------------------------
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc")
	bool bOverrideOnAttackSpecialProc = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc")
	bool bEnableOnAttackSpecialProc = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OnAttackSpecialChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0"))
	float OnAttackSpecialProcCooldown = 1.50f;*/
	
	// =========================
	// Gimmick: Harden Shell
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell")
	bool bEnableHardenShell = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.0"))
	float HardenDamageMultiplier = 0.5f;

	// 0.10 = 10% 단위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.01", ClampMax="1.0"))
	float HardenTriggerStepPercent = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell", meta=(ClampMin="0.01"))
	float HardenDurationSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell|Material")
	FName HardenTintStrengthParamName = TEXT("TintStrength");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HardenShell|Material")
	float HardenTintStrengthValue = 1.0f;
	
	// =========================
	// Gimmick: Aura Damage (선인장 접촉 데미지)
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage")
	bool bEnableAuraDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.0"))
	float AuraRadius = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.0"))
	float AuraDps = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage", meta=(ClampMin="0.01"))
	float AuraTickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraDamage")
	TArray<FName> AuraRequiredTargetTags;
};