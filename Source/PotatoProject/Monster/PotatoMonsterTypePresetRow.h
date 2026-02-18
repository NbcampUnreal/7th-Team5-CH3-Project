#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterTypePresetRow.generated.h"

class UPotatoMonsterAnimSet;
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special")
    FName DefaultSpecialSkillId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim")
    TSoftObjectPtr<UPotatoMonsterAnimSet> AnimSet;
};
