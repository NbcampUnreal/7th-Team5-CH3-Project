#include "PotatoMonsterSpawner.h"
#include "PotatoMonster.h"
#include "BPI_LanePathProvider.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

APotatoMonsterSpawner::APotatoMonsterSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

// =========================================================
// Stage / WaveId Helpers
// - GameMode에서 StartWave("1")만 호출하면
//   내부적으로 1-1, 1-2, 1-3 ... 순서로 자동 진행
// =========================================================

static bool IsStageOnlyName(const FName& InName, int32& OutStage)
{
    const FString S = InName.ToString().TrimStartAndEnd();
    if (S.Contains(TEXT("-"))) return false;
    if (!S.IsNumeric()) return false;

    OutStage = FCString::Atoi(*S);
    return OutStage > 0;
}

static FName MakeStageWaveId(int32 Stage, int32 Sub)
{
    return FName(*FString::Printf(TEXT("%d-%d"), Stage, Sub));
}

bool APotatoMonsterSpawner::HasMetaRow(FName WaveId) const
{
    return WaveMetaTable && WaveMetaTable->FindRow<FPotatoWaveMetaRow>(WaveId, TEXT("HasMetaRow")) != nullptr;
}

bool APotatoMonsterSpawner::ResolveFirstWaveForStage(int32 Stage, FName& OutWaveId, int32& OutSub)
{
    // stage의 첫 웨이브는 기본적으로 1부터 스캔
    int32& NextIdx = NextSubWaveIndexByStage.FindOrAdd(Stage);
    if (NextIdx <= 0) NextIdx = 1;

    const int32 ScanMax = NextIdx + 50; // 안전장치
    for (int32 Sub = NextIdx; Sub <= ScanMax; ++Sub)
    {
        FName Candidate = MakeStageWaveId(Stage, Sub);
        if (HasMetaRow(Candidate))
        {
            OutWaveId = Candidate;
            OutSub = Sub;
            return true;
        }
    }
    return false;
}

bool APotatoMonsterSpawner::ResolveNextWaveForActiveStage(FName& OutWaveId, int32& OutSub)
{
    if (ActiveStage <= 0) return false;
    return ResolveFirstWaveForStage(ActiveStage, OutWaveId, OutSub);
}

// =========================================================
// Public API
// =========================================================

void APotatoMonsterSpawner::StartWave(FName WaveId)
{
    // ---------------------------------------------------------
    // 입력이 "1" 같은 stage-only면 -> 1-1/1-2/... 자동 진행 모드
    // ---------------------------------------------------------
    int32 Stage = INDEX_NONE;
    if (IsStageOnlyName(WaveId, Stage))
    {
        if (!WaveMetaTable || !WaveSpawnTable)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave tables missing"));
            return;
        }

        FName ResolvedWaveId = NAME_None;
        int32 ResolvedSub = INDEX_NONE;

        if (!ResolveFirstWaveForStage(Stage, ResolvedWaveId, ResolvedSub))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Spawner] No wave found for Stage=%d (expected %d-1, %d-2, ...)"),
                Stage, Stage, Stage);
            return;
        }

        // stage 상태 기록 (자동 진행 ON)
        ActiveStage = Stage;
        ActiveSubWave = ResolvedSub;
        bStageAutoProgress = true;

        WaveId = ResolvedWaveId;

        UE_LOG(LogTemp, Log, TEXT("[Spawner] Stage input resolved | Stage=%d -> WaveId=%s"),
            Stage, *WaveId.ToString());
    }
    else
    {
        // 명시 WaveId(예: 1-2)로 시작하면 자동 진행 OFF (원하면 ON으로 바꿔도 됨)
        bStageAutoProgress = false;
        ActiveStage = INDEX_NONE;
        ActiveSubWave = INDEX_NONE;
    }

    // 이전 웨이브 강제 중단(스폰 중지 + 남은 몬스터 제거)
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

    //  웨이브 상태 초기화
    bWaveActive = true;
    bSpawnFinished = false;
    AliveCount = 0;

    const float PreDelay = FMath::Max(0.f, Meta->PreDelay);
    GetWorldTimerManager().SetTimer(
        SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
        CurrentSpawnInterval, true, PreDelay
    );

    UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %s started | Interval=%.2f | PreDelay=%.2f | QueueItems=%d"),
        *WaveId.ToString(), CurrentSpawnInterval, PreDelay, PendingQueue.Num());
}

void APotatoMonsterSpawner::StopWave()
{
    //  StopWave = "강제 중단" (스폰 중지 + 남아있는 몬스터 즉시 제거)
    GetWorldTimerManager().ClearTimer(SpawnTickHandle);
    PendingQueue.Reset();

    bWaveActive = false;
    bSpawnFinished = false;

    // 남은 몬스터 즉시 제거 (Force Reset 용도)
    for (TWeakObjectPtr<APotatoMonster>& M : SpawnedMonsters)
    {
        if (M.IsValid())
        {
            M->Destroy();
        }
    }
    SpawnedMonsters.Reset();

    AliveCount = 0;
    CurrentWaveId = NAME_None;
}

void APotatoMonsterSpawner::NotifyGameOver()
{
    //  게임 오버면 즉시 종료 + 몬스터 즉시 제거
    EndWave(EPotatoWaveEndReason::GameOver, /*bClearMonsters=*/true);
}

void APotatoMonsterSpawner::NotifyTimeExpired()
{
    //  시간 종료면 즉시 종료 + 몬스터 즉시 제거
    EndWave(EPotatoWaveEndReason::TimeExpired, /*bClearMonsters=*/true);
}

// =========================================================
// Wave End / Clear
// =========================================================

void APotatoMonsterSpawner::EndWave(EPotatoWaveEndReason Reason, bool bClearMonsters)
{
    if (!bWaveActive && CurrentWaveId.IsNone())
    {
        // 이미 끝난 상태
        return;
    }

    // 스폰 중지 / 큐 정리
    GetWorldTimerManager().ClearTimer(SpawnTickHandle);
    PendingQueue.Reset();

    const FName EndedWave = CurrentWaveId;

    bWaveActive = false;
    bSpawnFinished = false;
    CurrentWaveId = NAME_None;

    if (bClearMonsters)
    {
        for (TWeakObjectPtr<APotatoMonster>& M : SpawnedMonsters)
        {
            if (M.IsValid())
            {
                M->Destroy();
            }
        }
        SpawnedMonsters.Reset();
        AliveCount = 0;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %s ended | Reason=%d | ClearMonsters=%d"),
        *EndedWave.ToString(), (int32)Reason, bClearMonsters ? 1 : 0);

    // =========================================================
    // 자동 진행: Cleared면 다음 1-x를 찾아서 자동 StartWave
    // (GameMode는 StartWave("1") 한 번만 호출하면 됨)
    // =========================================================
    if (bStageAutoProgress && Reason == EPotatoWaveEndReason::Cleared && ActiveStage > 0)
    {
        // "클리어"일 때만 next index 갱신
        int32& NextIdx = NextSubWaveIndexByStage.FindOrAdd(ActiveStage);
        NextIdx = FMath::Max(NextIdx, ActiveSubWave + 1);

        FName NextWaveId = NAME_None;
        int32 NextSub = INDEX_NONE;

        if (ResolveNextWaveForActiveStage(NextWaveId, NextSub))
        {
            ActiveSubWave = NextSub;

            UE_LOG(LogTemp, Warning, TEXT("[Spawner] Auto progress -> NextWave=%s"), *NextWaveId.ToString());

            // 바로 다음 웨이브 시작
            StartWave(NextWaveId);
            return;
        }

        // 더 이상 1-x가 없으면 stage 종료
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Stage %d finished (no more waves)"), ActiveStage);

        // 필요하면 델리게이트/이벤트로 GameMode에 알림:
        // OnStageEnded.Broadcast(ActiveStage);

        bStageAutoProgress = false;
        ActiveStage = INDEX_NONE;
        ActiveSubWave = INDEX_NONE;
    }

    // 여기서 필요하면 델리게이트/이벤트로 GameMode에 알림:
    // OnWaveEnded.Broadcast(EndedWave, Reason);
}

// =========================================================
// Queue Build
// =========================================================

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

// =========================================================
// Spawn Tick
// =========================================================

void APotatoMonsterSpawner::TickSpawn()
{
    if (!bWaveActive)
    {
        return;
    }

    //  스폰 데이터 소진 = "스폰 종료" (완전 종료 아님)
    if (PendingQueue.Num() == 0)
    {
        bSpawnFinished = true;
        GetWorldTimerManager().ClearTimer(SpawnTickHandle);

        UE_LOG(LogTemp, Warning, TEXT("[Spawner] Spawn finished | Wave=%s | Alive=%d"),
            *CurrentWaveId.ToString(), AliveCount);

        // 스폰도 끝났고, 몬스터도 없다면 여기서 웨이브 클리어
        if (AliveCount <= 0)
        {
            EndWave(EPotatoWaveEndReason::Cleared, /*bClearMonsters=*/false);
        }
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

    //  스폰 성공 시에만 Remaining 감소 (중요)
    APotatoMonster* Spawned = SpawnOne(Cur.Type, Cur.Rank, Cur.SpawnGroup);
    if (Spawned)
    {
        Cur.Remaining--;
        if (Cur.Remaining <= 0)
        {
            PendingQueue.RemoveAt(0);

            // 다음 tick은 원래 interval로
            GetWorldTimerManager().ClearTimer(SpawnTickHandle);
            GetWorldTimerManager().SetTimer(
                SpawnTickHandle, this, &APotatoMonsterSpawner::TickSpawn,
                CurrentSpawnInterval, true, CurrentSpawnInterval
            );
        }
    }
}

// =========================================================
// Spawn One
// =========================================================

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
        IBPI_LanePathProvider::Execute_GetLanePoints(LanePathManager, SpawnGroup, LanePointsRaw);
        bHasLane = (LanePointsRaw.Num() > 0);

        UE_LOG(LogTemp, Log, TEXT("[Spawner] GetLanePoints OK | Group=%s | RawPoints=%d"),
            *SpawnGroup.ToString(), LanePointsRaw.Num());
    }

    FVector SpawnLoc = GetActorLocation(); // fallback
    if (bHasLane && LanePointsRaw.Num() > 0 && IsValid(LanePointsRaw[0]))
    {
        SpawnLoc = LanePointsRaw[0]->GetActorLocation();
    }
    else
    {
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
    // LanePoints 주입 + LaneIndex 시작값
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
            Monster->LaneIndex = 1;
        }

        UE_LOG(LogTemp, Log, TEXT("[Spawner] Lane injected | Group=%s | Points=%d | StartIndex=%d"),
            *SpawnGroup.ToString(), Monster->LanePoints.Num(), Monster->LaneIndex);
    }

    Monster->FinishSpawning(Xform);

    //  살아있는 몬스터 카운트/추적
    AliveCount++;
    SpawnedMonsters.Add(Monster);

    //  몬스터가 Destroy 되면 AliveCount 감소 -> (스폰 종료 후) 0이면 웨이브 클리어
    Monster->OnDestroyed.AddDynamic(this, &APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed);

    return Monster;
}

// =========================================================
// Monster Destroy Handler
// =========================================================

void APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed(AActor* DestroyedActor)
{
    if (!bWaveActive)
    {
        return;
    }

    AliveCount = FMath::Max(0, AliveCount - 1);

    //  스폰이 끝났고, 살아있는 몬스터가 0이면 웨이브 클리어
    if (bSpawnFinished && AliveCount <= 0)
    {
        EndWave(EPotatoWaveEndReason::Cleared, /*bClearMonsters=*/false);
    }
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