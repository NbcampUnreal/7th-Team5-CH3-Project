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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    FName DefaultSpecialSkillId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    float SpecialCooldownMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    float SpecialDamageMultiplier = 1.0f;
	
	// OnAttack Proc (Rank default)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc")
	bool bEnableOnAttackSpecialProc = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OnAttackSpecialChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0"))
	float OnAttackSpecialProcCooldown = 1.50f;
};
