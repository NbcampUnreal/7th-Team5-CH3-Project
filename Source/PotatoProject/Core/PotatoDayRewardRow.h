// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PotatoDayRewardRow.generated.h"

/**
 * DayRewardTable
 * - DayPhase로 같은 데이 리워드 엔트리들을 모아서 사용
 */
USTRUCT(BlueprintType)
struct FPotatoDayRewardRow : public FTableRowBase
{
    GENERATED_BODY()

    // 나무 보상
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
    int32 WoodReward = 0;

    // 돌 보상
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
    int32 StoneReward = 0;

    // 농작물 보상
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
    int32 CropReward = 0;

    // 축산물 보상
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
    int32 LivestockReward = 0;
};