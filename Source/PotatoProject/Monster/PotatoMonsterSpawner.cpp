#include "PotatoMonsterSpawner.h"
#include "PotatoMonster.h"
#include "Kismet/GameplayStatics.h"

APotatoMonsterSpawner::APotatoMonsterSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APotatoMonsterSpawner::StartWave(FName WaveId)
{
    StopWave();

    if (!WaveMetaTable || !WaveSpawnTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave tables missing"));
        return;
    }

    const FPotatoWaveMetaRow* Meta = WaveMetaTable->FindRow<FPotatoWaveMetaRow>(
        WaveId, TEXT("StartWave:Meta")
    );

    if (!Meta)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Meta not found for WaveId=%s"), *WaveId.ToString());
        return;
    }

    CurrentWaveId = WaveId;
    CurrentSpawnInterval = (Meta->SpawnInterval > 0.f) ? Meta->SpawnInterval : DefaultSpawnInterval;

    BuildQueueForWave(WaveId, *Meta);

    // PreDelay 처리
    const float PreDelay = FMath::Max(0.f, Meta->PreDelay);
    GetWorldTimerManager().SetTimer(
        SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
        CurrentSpawnInterval, true, PreDelay
    );
}

void APotatoMonsterSpawner::StopWave()
{
    GetWorldTimerManager().ClearTimer(SpawnTickHandle);
    PendingQueue.Reset();
    CurrentWaveId = NAME_None;
}

void APotatoMonsterSpawner::BuildQueueForWave(FName WaveId, const FPotatoWaveMetaRow& Meta)
{
    PendingQueue.Reset();

    // WaveSpawnTable 전체를 훑고 WaveId 매칭되는 행만 큐로
    const TArray<FName> RowNames = WaveSpawnTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        const FPotatoWaveSpawnRow* Row = WaveSpawnTable->FindRow<FPotatoWaveSpawnRow>(RowName, TEXT("BuildQueue"));
        if (!Row) continue;
        if (Row->WaveId != WaveId) continue;
        if (Row->Count <= 0) continue;

        FPendingSpawn Item;
        Item.WaveId = Row->WaveId;
        Item.Rank = Row->Rank;
        Item.Type = Row->Type;
        Item.Remaining = Row->Count;
        Item.EntryDelay = Row->Delay;
        Item.SpawnGroup = Row->SpawnGroup;
        PendingQueue.Add(Item);
    }

    // (선택) 특정 규칙으로 정렬하고 싶으면 여기서 sort
    // 예) Elite 먼저/나중, Type별 섞기 등
}

void APotatoMonsterSpawner::TickSpawn()
{
    if (PendingQueue.Num() == 0)
    {
        StopWave();
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %s finished"), *CurrentWaveId.ToString());
        return;
    }

    FPendingSpawn& Cur = PendingQueue[0];

    // 엔트리 시작 지연(1회)
    if (!Cur.bEntryDelayConsumed && Cur.EntryDelay > 0.f)
    {
        Cur.bEntryDelayConsumed = true;

        // 이번 Tick은 소비하고 다음 Tick부터 스폰하도록 “한 번만 지연” 처리
        // 더 정확히 하려면 별도의 타이머로 Cur.EntryDelay를 반영해도 됨.
        GetWorldTimerManager().ClearTimer(SpawnTickHandle);
        GetWorldTimerManager().SetTimer(
            SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
            CurrentSpawnInterval, true, Cur.EntryDelay
        );
        return;
    }

    // 하나 스폰
    SpawnOne(Cur.Type, Cur.Rank, Cur.SpawnGroup);

    Cur.Remaining--;
    if (Cur.Remaining <= 0)
    {
        PendingQueue.RemoveAt(0);

        // 지연 타이머를 다시 기본 tick으로 복구
        GetWorldTimerManager().ClearTimer(SpawnTickHandle);
        GetWorldTimerManager().SetTimer(
            SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
            CurrentSpawnInterval, true, CurrentSpawnInterval
        );
    }
}

APotatoMonster* APotatoMonsterSpawner::SpawnOne(EMonsterType Type, EMonsterRank Rank, FName SpawnGroup)
{
    TSubclassOf<APotatoMonster>* ClassPtr = MonsterClassByType.Find(Type);
    if (!ClassPtr || !(*ClassPtr))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] No monster class for Type"));
        return nullptr;
    }

    const FVector Loc = GetSpawnLocationByGroup(SpawnGroup);
    const FRotator Rot = FRotator::ZeroRotator;
    const FTransform Xform(Rot, Loc);

    // ✅ 주입-타이밍 안정화를 위해 Deferred Spawn 추천
    APotatoMonster* Monster = GetWorld()->SpawnActorDeferred<APotatoMonster>(
        *ClassPtr, Xform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (!Monster) return nullptr;

    // ---- 필수 주입: WarehouseActor (월드 검색 X, BP 지정 액터) ----
    Monster->WarehouseActor = DefaultWarehouseActor;

    // ---- 웨이브/프리셋 테이블 주입 ----
    Monster->Rank = Rank;                 // BP 자식 고정이라면 생략 가능하지만, 웨이브가 바꾸는 구조면 유지
    Monster->MonsterType = Type;          // 웨이브 타입에 따라 바뀌는 구조면 필요

    Monster->WaveBaseHP = DefaultWaveBaseHP; // 또는 메타 기반/난이도 기반 계산 가능

    Monster->TypePresetTable = TypePresetTable;
    Monster->RankPresetTable = RankPresetTable;
    Monster->SpecialSkillPresetTable = SpecialSkillPresetTable;

    // ApplyPresets는 FinishSpawning 이후 BeginPlay/컴포넌트 준비가 끝난 뒤 호출해도 되고,
    // 지금처럼 테이블만 쓰는 순수 로직이면 여기서 호출해도 됨.
    // 안정적으로는 FinishSpawning 직후 호출 추천.

    UGameplayStatics::FinishSpawningActor(Monster, Xform);

    Monster->ApplyPresets();

    return Monster;
}

FVector APotatoMonsterSpawner::GetSpawnLocationByGroup(FName SpawnGroup) const
{
    // TODO: SpawnGroup별 SpawnPoint 배열/볼륨/소켓 등을 붙일 수 있음.
    // 테스트 단계: 스포너 위치 기반으로 단순 스폰
    return GetActorLocation();
}
