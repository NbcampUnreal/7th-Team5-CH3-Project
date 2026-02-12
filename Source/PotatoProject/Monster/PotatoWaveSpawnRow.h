#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoWaveSpawnRow.generated.h"

/**
 * WaveSpawnTable
 * - WaveId로 같은 웨이브 엔트리들을 모아서 사용
 */
USTRUCT(BlueprintType)
struct FPotatoWaveSpawnRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    FName WaveId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    EMonsterRank Rank = EMonsterRank::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    EMonsterType Type = EMonsterType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
    int32 Count = 1;

    // (선택) 이 엔트리만 따로 지연/연출 주고 싶을 때
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
    float Delay = 0.f;

    // (선택) 스폰 포인트 그룹 분기(예: "Left", "Right", "NearNexus")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    FName SpawnGroup;
};
