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

    UPROPERTY(BlueprintReadOnly) float MaxHP = 100.f;
    UPROPERTY(BlueprintReadOnly) float AttackDamage = 10.f;
    UPROPERTY(BlueprintReadOnly) float AttackRange = 150.f;
    UPROPERTY(BlueprintReadOnly) float MoveSpeed = 300.f;

    UPROPERTY(BlueprintReadOnly) float AppliedHpMultiplier = 1.f;
    UPROPERTY(BlueprintReadOnly) float AppliedMoveSpeedRatio = 0.6f;
    UPROPERTY(BlueprintReadOnly) float StructureDamageMultiplier = 1.f;

    UPROPERTY(BlueprintReadOnly) bool bIsRanged = false;

    UPROPERTY(BlueprintReadOnly) FName SpecialSkillId = NAME_None;
    UPROPERTY(BlueprintReadOnly) EMonsterSpecialLogic SpecialLogic = EMonsterSpecialLogic::None;
    UPROPERTY(BlueprintReadOnly) float SpecialCooldown = 0.f;
    UPROPERTY(BlueprintReadOnly) float SpecialDamageMultiplier = 1.f;

    UPROPERTY(BlueprintReadOnly) TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
    TObjectPtr<UPotatoMonsterAnimSet> AnimSet = nullptr;

};
