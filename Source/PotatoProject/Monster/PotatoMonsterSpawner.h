#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"

#include "PotatoWaveMetaRow.h"
#include "PotatoWaveSpawnRow.h"
#include "../Core/PotatoEnums.h"

#include "PotatoMonsterSpawner.generated.h"

class APotatoMonster;

USTRUCT()
struct FPendingSpawn
{
    GENERATED_BODY()

    FName WaveId;
    EMonsterRank Rank = EMonsterRank::Normal;
    EMonsterType Type = EMonsterType::None;
    int32 Remaining = 0;

    float EntryDelay = 0.f;
    FName SpawnGroup;

    // 내부용
    bool bEntryDelayConsumed = false;
};

UCLASS()
class POTATOPROJECT_API APotatoMonsterSpawner : public AActor
{
    GENERATED_BODY()

public:
    APotatoMonsterSpawner();

    UFUNCTION(BlueprintCallable, Category = "Wave")
    void StartWave(FName WaveId);

    UFUNCTION(BlueprintCallable, Category = "Wave")
    void StopWave();

protected:
    // ==== Wave Tables ====
    UPROPERTY(EditAnywhere, Category = "Wave|Data")
    TObjectPtr<UDataTable> WaveMetaTable = nullptr;

    UPROPERTY(EditAnywhere, Category = "Wave|Data")
    TObjectPtr<UDataTable> WaveSpawnTable = nullptr;

    // ==== Monster preset tables (주입용) ====
    UPROPERTY(EditAnywhere, Category = "Monster|Data")
    TObjectPtr<UDataTable> TypePresetTable = nullptr;

    UPROPERTY(EditAnywhere, Category = "Monster|Data")
    TObjectPtr<UDataTable> RankPresetTable = nullptr;

    UPROPERTY(EditAnywhere, Category = "Monster|Data")
    TObjectPtr<UDataTable> SpecialSkillPresetTable = nullptr;

    // ==== Warehouse (직접 지정) ====
    UPROPERTY(EditAnywhere, Category = "Warehouse")
    TObjectPtr<AActor> DefaultWarehouseActor = nullptr;

    // ==== Spawn class mapping ====
    UPROPERTY(EditAnywhere, Category = "Monster|Classes")
    TMap<EMonsterType, TSubclassOf<APotatoMonster>> MonsterClassByType;

    // ==== Spawn settings ====
    UPROPERTY(EditAnywhere, Category = "Wave|Settings")
    float DefaultSpawnInterval = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Wave|Settings")
    float DefaultWaveBaseHP = 0.f;

    // ==== Lane (BP LanePathManager) ====
    // BP_LanePathManager 인스턴스를 여기 넣고, BP Interface(GetLanePoints)로 경유지 배열을 받아온다
    UPROPERTY(EditInstanceOnly, Category = "Lane")
    TObjectPtr<AActor> LanePathManager = nullptr;

    // Blackboard Key Name (MoveTo가 바라볼 키)
    UPROPERTY(EditDefaultsOnly, Category = "Lane")
    FName MoveTargetKeyName = TEXT("MoveTarget");

    // ==== Runtime ====
    FName CurrentWaveId = NAME_None;
    float CurrentSpawnInterval = 0.5f;

    TArray<FPendingSpawn> PendingQueue;

    FTimerHandle SpawnTickHandle;

    void BuildQueueForWave(FName WaveId, const FPotatoWaveMetaRow& Meta);
    void TickSpawn();

    APotatoMonster* SpawnOne(EMonsterType Type, EMonsterRank Rank, FName SpawnGroup);
    FVector GetSpawnLocationByGroup(FName SpawnGroup) const;

};
