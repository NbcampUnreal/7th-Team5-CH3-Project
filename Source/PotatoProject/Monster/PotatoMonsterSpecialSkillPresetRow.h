#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterSpecialSkillPresetRow.generated.h"

/**
 * MonsterSpecialSkillPresetTable
 * - RowName = SkillId (예: "Slime_Split", "Skeleton_AoESlash", "Cactus_ContactDot")
 * - “기믹의 상세 파라미터”를 전부 여기서 관리
 */
USTRUCT(BlueprintType)
struct FPotatoMonsterSpecialSkillPresetRow : public FTableRowBase
{
    GENERATED_BODY()

    // 어떤 로직인지 (코드 분기 키)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    EMonsterSpecialLogic Logic = EMonsterSpecialLogic::None;

    // 언제 발동하는지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    EMonsterSpecialTrigger Trigger = EMonsterSpecialTrigger::OnCooldown;

    // 발동 조건(선택): OnNearTarget일 때 거리 등
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    float TriggerRange = 0.f;

    // 타이밍
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
    float Cooldown = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
    float CastTime = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
    float TelegraphTime = 0.f;

    // 형태/범위
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    EMonsterSpecialShape Shape = EMonsterSpecialShape::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0"))
    float Radius = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0"))
    float AngleDeg = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0"))
    float Range = 0.f;

    // 피해/효과 수치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float DamageMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0"))
    float DotDps = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0"))
    float DotDuration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0"))
    float StunDuration = 0.f;

    // 투사체(독침 같은 경우)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
    TSoftClassPtr<AActor> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
    float ProjectileSpeed = 0.f;

    // 연출(선택)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TSoftObjectPtr<UObject> VFX; // Niagara면 UNiagaraSystem으로 바꾸는 걸 추천
};
