#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "../Core/PotatoEnums.h"

#include "PotatoMonsterFinalStats.h"
#include "PotatoPresetApplier.generated.h"

UCLASS()
class UPotatoPresetApplier : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static FName GetRankRowName(EMonsterRank InRank);
    static FName GetTypeRowName(EMonsterType InType);

    UFUNCTION(BlueprintCallable, Category = "Potato|Presets")
    static FPotatoMonsterFinalStats BuildFinalStats(
        UObject* WorldContextObject,
        EMonsterType MonsterType,
        EMonsterRank Rank,
        float WaveBaseHP,
        float PlayerReferenceSpeed,
        UDataTable* TypePresetTable,
        UDataTable* RankPresetTable,
        UDataTable* SpecialSkillPresetTable,
        UBehaviorTree* DefaultBehaviorTree
    );
};
