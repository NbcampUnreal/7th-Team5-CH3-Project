#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterRankPresetRow.generated.h"

USTRUCT(BlueprintType)
struct FPotatoMonsterRankPresetRow : public FTableRowBase
{
	GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float HpMultiplierMin = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float HpMultiplierMax = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MoveSpeedRatioToPlayer = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float StructureDamageMultiplier = 1.0f;

    // (선택) 랭크 기본 특수 스킬 (예: Elite는 기본 AOE)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    FName DefaultSpecialSkillId;

    // (선택) 스페셜이 더 자주/강하게 나가게 하고 싶으면 공통 배수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    float SpecialCooldownMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    float SpecialDamageMultiplier = 1.0f;
};
