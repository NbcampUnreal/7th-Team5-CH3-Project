#pragma once

#include "CoreMinimal.h"
#include "PotatoEnums.h"
#include "PotatoTypes.generated.h" // 파일명과 일치 필수

USTRUCT(BlueprintType)
struct FWaveData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    int WaveNumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    float SpawnTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    int MonsterCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    EMonsterType MonsterType;

    /*FWaveData()
        : WaveNumber(100.f), SpawnTime(10.f), MonsterCount(0), MonsterType();
    {}*/
};