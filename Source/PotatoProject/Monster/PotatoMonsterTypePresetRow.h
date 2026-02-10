#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterTypePresetRow.generated.h"

USTRUCT(BlueprintType)
struct FPotatoMonsterTypePresetRow : public FTableRowBase
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float BaseHP = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float BaseAttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float BaseAttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MoveSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    TSoftObjectPtr<class UBehaviorTree> OverrideBehaviorTree;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bIsRanged = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    TSoftClassPtr<AActor> ProjectileClass;

    // (선택) 타입 기본 특수 스킬 (예: Ranged 타입이 기본적으로 “독침”을 쓰는 경우)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    FName DefaultSpecialSkillId;
};
