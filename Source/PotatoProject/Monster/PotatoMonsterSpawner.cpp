#include "PotatoMonsterSpawner.h"
#include "PotatoMonster.h"
#include "BPI_LanePathProvider.h"

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

void APotatoMonsterSpawner::BuildQueueForWave(FName WaveId, const FPotatoWaveMetaRow& /*Meta*/)
{
    PendingQueue.Reset();

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
}

void APotatoMonsterSpawner::TickSpawn()
{
    if (PendingQueue.Num() == 0)
    {
        const FName FinishedWave = CurrentWaveId;
        StopWave();
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %s finished"), *FinishedWave.ToString());
        return;
    }

    FPendingSpawn& Cur = PendingQueue[0];

    if (!Cur.bEntryDelayConsumed && Cur.EntryDelay > 0.f)
    {
        Cur.bEntryDelayConsumed = true;

        GetWorldTimerManager().ClearTimer(SpawnTickHandle);
        GetWorldTimerManager().SetTimer(
            SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
            CurrentSpawnInterval, true, Cur.EntryDelay
        );
        return;
    }

    SpawnOne(Cur.Type, Cur.Rank, Cur.SpawnGroup);

    Cur.Remaining--;
    if (Cur.Remaining <= 0)
    {
        PendingQueue.RemoveAt(0);

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
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] No monster class for Type=%s (%d)"),
            *UEnum::GetValueAsString(Type), (int32)Type);
        return nullptr;
    }

    if (!DefaultWarehouseActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] DefaultWarehouseActor is NULL (Wave=%s)"), *CurrentWaveId.ToString());
    }

    TArray<AActor*> LanePointsRaw;
    bool bHasLane = false;

    if (!LanePathManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] LanePathManager is NULL"));
    }
    else if (!LanePathManager->GetClass()->ImplementsInterface(UBPI_LanePathProvider::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] LanePathManager '%s' does NOT implement BPI_LanePathProvider"),
            *LanePathManager->GetName());
    }
    else
    {
        // BP에서 구현한 인터페이스 함수를 안전하게 호출
        IBPI_LanePathProvider::Execute_GetLanePoints(LanePathManager, SpawnGroup, LanePointsRaw);
        bHasLane = (LanePointsRaw.Num() > 0);

        UE_LOG(LogTemp, Log, TEXT("[Spawner] GetLanePoints OK | Group=%s | RawPoints=%d"),
            *SpawnGroup.ToString(), LanePointsRaw.Num());
    }

    FVector SpawnLoc = GetActorLocation(); // fallback
    if (bHasLane && IsValid(LanePointsRaw[0]))
    {
        SpawnLoc = LanePointsRaw[0]->GetActorLocation();
    }
    else
    {
        // fallback이 계속 뜬다면 LanePointsRaw[0]이 NULL/Invalid인 상태
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] SpawnLoc fallback -> Spawner location | Group=%s | bHasLane=%d | RawNum=%d | FirstValid=%d"),
            *SpawnGroup.ToString(),
            bHasLane ? 1 : 0,
            LanePointsRaw.Num(),
            (LanePointsRaw.Num() > 0 && IsValid(LanePointsRaw[0])) ? 1 : 0);
        SpawnLoc = GetActorLocation();
    }

    const FTransform Xform(FRotator::ZeroRotator, SpawnLoc);

    APotatoMonster* Monster = GetWorld()->SpawnActorDeferred<APotatoMonster>(
        *ClassPtr, Xform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (!Monster) return nullptr;

    // =========================================================
    // Spawner 책임: "데이터 주입"만 (AI/BB/MoveTarget 금지)
    // =========================================================
    Monster->WarehouseActor = DefaultWarehouseActor;
    Monster->Rank = Rank;
    Monster->MonsterType = Type;
    Monster->WaveBaseHP = DefaultWaveBaseHP;

    Monster->TypePresetTable = TypePresetTable;
    Monster->RankPresetTable = RankPresetTable;
    Monster->SpecialSkillPresetTable = SpecialSkillPresetTable;

    // =========================================================
    // 2) LanePoints 주입 + LaneIndex 시작값
    //    - Entry(0)에서 스폰했으면, 첫 이동 목표는 1번부터
    // =========================================================
    Monster->LanePoints.Reset();
    Monster->LaneIndex = 0;

    if (bHasLane)
    {
        for (AActor* P : LanePointsRaw)
        {
            if (IsValid(P))
            {
                Monster->LanePoints.Add(P);
            }
        }

        if (Monster->LanePoints.Num() >= 2)
        {
            Monster->LaneIndex = 1; // 0번은 스폰 위치, 1번부터 이동
        }

        UE_LOG(LogTemp, Log, TEXT("[Spawner] Lane injected | Group=%s | Points=%d | StartIndex=%d"),
            *SpawnGroup.ToString(), Monster->LanePoints.Num(), Monster->LaneIndex);
    }

    // FinishSpawning (AutoPossessAI 설정이면 여기서 컨트롤러가 붙고 OnPossess에서 BB 세팅)
    Monster->FinishSpawning(Xform);

    return Monster;
}

FVector APotatoMonsterSpawner::GetSpawnLocationByGroup(FName /*SpawnGroup*/) const
{
    const FVector Origin = GetActorLocation();
    const float Radius = 300.f;

    const FVector2D Rand2D = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
    const float Dist = FMath::FRandRange(0.f, Radius);

    FVector Loc = Origin + FVector(Rand2D.X, Rand2D.Y, 0.f) * Dist;
    Loc.Z += 20.f;
    return Loc;
}
