#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PotatoWaveMetaRow.generated.h"

/**
 * WaveMetaTable
 * - RowName 추천: WaveId ("1-1", "1-2"...)
 * - 웨이브 1개당 1행 (메타 정보)
 */
USTRUCT(BlueprintType)
struct FPotatoWaveMetaRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    int32 Round = 1;

    // RowName과 동일 추천
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    FName WaveId;

    // 0이면 제한 없음 같은 룰 가능
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    float TimeLimit = 0.f;

    // 웨이브 시작 전 대기
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    float PreDelay = 0.f;

    // 웨이브 내 스폰 템포(0이면 Spawner 기본값 사용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    float SpawnInterval = 0.f;
};
