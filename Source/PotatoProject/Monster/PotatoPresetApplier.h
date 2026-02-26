#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterFinalStats.h"
#include "PotatoPresetApplier.generated.h"

class UDataTable;
class UBehaviorTree;

UCLASS()
class POTATOPROJECT_API UPotatoPresetApplier : public UObject
{
	GENERATED_BODY()

public:
	static FName GetRankRowName(EMonsterRank InRank);
	static FName GetTypeRowName(EMonsterType InType);

	static FPotatoMonsterFinalStats BuildFinalStats(
		UObject* WorldContextObject,
		EMonsterType MonsterType,
		EMonsterRank Rank,
		float WaveBaseHP,
		float PlayerReferenceSpeed,
		UDataTable* TypePresetTable,
		UDataTable* RankPresetTable,
		UDataTable* SpecialSkillPresetTable, // (여기선 "검증" 용도로만 써도 됨)
		UBehaviorTree* DefaultBehaviorTree
	);
};