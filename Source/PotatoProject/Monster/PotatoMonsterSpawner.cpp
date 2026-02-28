#include "PotatoMonsterSpawner.h"

#include "PotatoMonster.h"
#include "BPI_LanePathProvider.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

//  Utils (CPP-local helper 치환)
#include "Utils/PotatoWaveIdUtils.h" // IsStageOnlyName, MakeStageWaveId

APotatoMonsterSpawner::APotatoMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool APotatoMonsterSpawner::HasMetaRow(FName WaveId) const
{
	return WaveMetaTable && WaveMetaTable->FindRow<FPotatoWaveMetaRow>(WaveId, TEXT("HasMetaRow")) != nullptr;
}

bool APotatoMonsterSpawner::ResolveFirstWaveForStage(int32 Stage, FName& OutWaveId, int32& OutSub)
{
	int32& NextIdx = NextSubWaveIndexByStage.FindOrAdd(Stage);
	if (NextIdx <= 0) NextIdx = 1;

	const int32 ScanMax = NextIdx + 50;
	for (int32 Sub = NextIdx; Sub <= ScanMax; ++Sub)
	{
		const FName Candidate = MakeStageWaveId(Stage, Sub);
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

// ---------------------------------------------------------
//  등록 루트 (AliveCount/OnDestroyed/SpawnedMonsters)
// ---------------------------------------------------------

void APotatoMonsterSpawner::RegisterSpawnedMonster(APotatoMonster* Monster)
{
	if (!IsValid(Monster)) return;

	AliveCount++;
	SpawnedMonsters.Add(Monster);

	//  반드시 UFUNCTION + (AActor*) 시그니처
	Monster->OnDestroyed.AddDynamic(this, &APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed);

	UE_LOG(LogTemp, Warning, TEXT("[Spawner] Register OK | Alive=%d | Monster=%s"),
		AliveCount, *GetNameSafe(Monster));
}

void APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed(AActor* DestroyedActor)
{
	if (!bWaveActive) return;

	AliveCount = FMath::Max(0, AliveCount - 1);

	if (bSpawnFinished && AliveCount <= 0)
	{
		EndWave(EPotatoWaveEndReason::Cleared, /*bClearMonsters=*/false);
	}
}

// ---------------------------------------------------------
// Public API
// ---------------------------------------------------------

void APotatoMonsterSpawner::StartWave(FName WaveId)
{
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
			UE_LOG(LogTemp, Warning, TEXT("[Spawner] No wave found for Stage=%d"), Stage);
			return;
		}

		ActiveStage = Stage;
		ActiveSubWave = ResolvedSub;
		bStageAutoProgress = true;

		WaveId = ResolvedWaveId;

		UE_LOG(LogTemp, Log, TEXT("[Spawner] Stage input resolved | Stage=%d -> WaveId=%s"),
			Stage, *WaveId.ToString());
	}
	else
	{
		bStageAutoProgress = false;
		ActiveStage = INDEX_NONE;
		ActiveSubWave = INDEX_NONE;
	}

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

	bWaveActive = true;
	bSpawnFinished = false;
	AliveCount = 0;
	SpawnedMonsters.Reset();

	const float PreDelay = FMath::Max(0.f, Meta->PreDelay);

	GetWorldTimerManager().SetTimer(
		SpawnTickHandle,
		this,
		&APotatoMonsterSpawner::TickSpawn,
		CurrentSpawnInterval,
		true,
		PreDelay
	);

	UE_LOG(LogTemp, Warning, TEXT("[Spawner] Wave %s started | Interval=%.2f | PreDelay=%.2f | QueueItems=%d"),
		*WaveId.ToString(), CurrentSpawnInterval, PreDelay, PendingQueue.Num());
}

void APotatoMonsterSpawner::StopWave()
{
	GetWorldTimerManager().ClearTimer(SpawnTickHandle);
	PendingQueue.Reset();

	bWaveActive = false;
	bSpawnFinished = false;

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
	EndWave(EPotatoWaveEndReason::GameOver, /*bClearMonsters=*/true);
}

void APotatoMonsterSpawner::NotifyTimeExpired()
{
	EndWave(EPotatoWaveEndReason::TimeExpired, /*bClearMonsters=*/true);
}

void APotatoMonsterSpawner::EndWave(EPotatoWaveEndReason Reason, bool bClearMonsters)
{
	if (!bWaveActive && CurrentWaveId.IsNone())
	{
		return;
	}

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

	if (bStageAutoProgress && Reason == EPotatoWaveEndReason::Cleared && ActiveStage > 0)
	{
		int32& NextIdx = NextSubWaveIndexByStage.FindOrAdd(ActiveStage);
		NextIdx = FMath::Max(NextIdx, ActiveSubWave + 1);

		FName NextWaveId = NAME_None;
		int32 NextSub = INDEX_NONE;

		if (ResolveNextWaveForActiveStage(NextWaveId, NextSub))
		{
			ActiveSubWave = NextSub;
			UE_LOG(LogTemp, Warning, TEXT("[Spawner] Auto progress -> NextWave=%s"), *NextWaveId.ToString());
			StartWave(NextWaveId);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Spawner] Stage %d finished (no more waves)"), ActiveStage);
		bStageAutoProgress = false;
		ActiveStage = INDEX_NONE;
		ActiveSubWave = INDEX_NONE;
	}
}

// ---------------------------------------------------------
// Queue Build / Tick
// ---------------------------------------------------------

void APotatoMonsterSpawner::BuildQueueForWave(FName WaveId, const FPotatoWaveMetaRow& /*Meta*/)
{
	PendingQueue.Reset();

	if (!WaveSpawnTable) return;

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
	if (!bWaveActive) return;

	// 더 이상 스폰할 게 없다
	if (PendingQueue.Num() == 0)
	{
		bSpawnFinished = true;
		GetWorldTimerManager().ClearTimer(SpawnTickHandle);

		UE_LOG(LogTemp, Warning, TEXT("[Spawner] Spawn finished | Wave=%s | Alive=%d"),
			*CurrentWaveId.ToString(), AliveCount);

		if (AliveCount <= 0)
		{
			EndWave(EPotatoWaveEndReason::Cleared, /*bClearMonsters=*/false);
		}
		return;
	}

	FPendingSpawn& Cur = PendingQueue[0];

	// 엔트리 지연 1회 소모
	if (!Cur.bEntryDelayConsumed && Cur.EntryDelay > 0.f)
	{
		Cur.bEntryDelayConsumed = true;

		GetWorldTimerManager().ClearTimer(SpawnTickHandle);
		GetWorldTimerManager().SetTimer(
			SpawnTickHandle,
			this,
			&APotatoMonsterSpawner::TickSpawn,
			CurrentSpawnInterval,
			true,
			Cur.EntryDelay
		);
		return;
	}

	APotatoMonster* Spawned = SpawnOne(Cur.Type, Cur.Rank, Cur.SpawnGroup);
	if (Spawned)
	{
		Cur.Remaining--;
		if (Cur.Remaining <= 0)
		{
			PendingQueue.RemoveAt(0);

			// 다음 아이템 스폰 간격 복구
			GetWorldTimerManager().ClearTimer(SpawnTickHandle);
			GetWorldTimerManager().SetTimer(
				SpawnTickHandle,
				this,
				&APotatoMonsterSpawner::TickSpawn,
				CurrentSpawnInterval,
				true,
				CurrentSpawnInterval
			);
		}
	}
}

// ---------------------------------------------------------
// SpawnOne
// ---------------------------------------------------------

APotatoMonster* APotatoMonsterSpawner::SpawnOne(EMonsterType Type, EMonsterRank Rank, FName SpawnGroup)
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TSubclassOf<APotatoMonster>* ClassPtr = MonsterClassByType.Find(Type);
	if (!ClassPtr || !(*ClassPtr))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Spawner] No monster class for Type=%s (%d)"),
			*UEnum::GetValueAsString(Type), (int32)Type);
		return nullptr;
	}

	// Lane points (raw)
	TArray<AActor*> LanePointsRaw;
	bool bHasLane = false;

	if (LanePathManager && LanePathManager->GetClass()->ImplementsInterface(UBPI_LanePathProvider::StaticClass()))
	{
		IBPI_LanePathProvider::Execute_GetLanePoints(LanePathManager, SpawnGroup, LanePointsRaw);
		bHasLane = (LanePointsRaw.Num() > 0);
	}

	FVector SpawnLoc = GetActorLocation();
	if (bHasLane && LanePointsRaw.Num() > 0 && IsValid(LanePointsRaw[0]))
	{
		SpawnLoc = LanePointsRaw[0]->GetActorLocation();
	}

	const FTransform Xform(FRotator::ZeroRotator, SpawnLoc);

	APotatoMonster* Monster = GetWorld()->SpawnActorDeferred<APotatoMonster>(
		*ClassPtr,
		Xform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (!Monster) return nullptr;

	// =========================
	// Spawner -> Monster 주입 (네 Monster.h/.cpp 기준)
	// =========================
	Monster->WarehouseActor = DefaultWarehouseActor;
	Monster->Rank = Rank;
	Monster->MonsterType = Type;
	Monster->WaveBaseHP = DefaultWaveBaseHP;

	Monster->TypePresetTable = TypePresetTable;
	Monster->RankPresetTable = RankPresetTable;
	Monster->SpecialSkillPresetTable = SpecialSkillPresetTable;

	//  Split에서 반드시 필요
	Monster->SpawnerRef = this;

	// Lane 주입 (Monster는 TArray<TObjectPtr<AActor>>)
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
	}

	Monster->FinishSpawning(Xform);

	//  네 Monster.cpp는 여기서 Stats/AI/BT까지 보장
	Monster->ApplyPresetsOnce();

	//  등록 (AliveCount + OnDestroyed)
	RegisterSpawnedMonster(Monster);

	return Monster;
}

// ---------------------------------------------------------
//  Split child spawn (Spawner 파이프라인)
// ---------------------------------------------------------

APotatoMonster* APotatoMonsterSpawner::SpawnSplitChildFromParent(
	APotatoMonster* Parent,
	int32 ChildDepth,
	float ChildMaxHpRatio,
	float SpawnJitterRadius,
	float SpawnZOffset)
{
	if (!bWaveActive) return nullptr;
	if (!IsValid(Parent) || !GetWorld()) return nullptr;

	// Spawn 위치: Parent 주변 지터
	const float R = FMath::Max(0.f, SpawnJitterRadius);

	FVector SpawnLoc = Parent->GetActorLocation();
	if (R > 0.f)
	{
		const FVector2D Rand2D = FVector2D(
			FMath::FRandRange(-1.f, 1.f),
			FMath::FRandRange(-1.f, 1.f)
		).GetSafeNormal();

		const float Dist = FMath::FRandRange(0.f, R);
		SpawnLoc += FVector(Rand2D.X, Rand2D.Y, 0.f) * Dist;
	}
	SpawnLoc.Z += SpawnZOffset;

	const FTransform Xform(Parent->GetActorRotation(), SpawnLoc);

	TSubclassOf<APotatoMonster> ChildClass = Parent->GetClass();
	if (!ChildClass) return nullptr;

	APotatoMonster* Child = GetWorld()->SpawnActorDeferred<APotatoMonster>(
		ChildClass,
		Xform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (!Child) return nullptr;

	// =========================
	// 부모 컨텍스트 복사 (네 Monster.cpp 설계에 맞게)
	// =========================
	Child->CopyPresetContextFrom(Parent);

	// Target/Lane는 실제 플레이에 바로 필요하니 같이 복사
	Child->WarehouseActor = Parent->WarehouseActor;
	Child->LanePoints = Parent->LanePoints;
	Child->LaneIndex = Parent->LaneIndex;

	//  Split에서 스포너 접근
	Child->SpawnerRef = this;

	Child->FinishSpawning(Xform);

	//  프리셋 적용 + AI 보장(네 Monster.cpp가 내부에서 함)
	Child->ApplyPresetsOnce();

	// =========================
	// HP ratio 적용 (ApplyPresetsOnce 이후 덮어쓰기)
	// =========================
	const float ParentMax = FMath::Max(1.f, Parent->MaxHealth);
	const float Ratio = FMath::Max(0.01f, ChildMaxHpRatio);
	const float ChildMax = FMath::Max(1.f, ParentMax * Ratio);

	Child->MaxHealth = ChildMax;
	Child->Health = ChildMax;
	Child->RefreshHPBar();

	// Split Depth 세팅
	if (Child->SplitComp)
	{
		Child->SplitComp->SetSplitDepth(ChildDepth);
	}

	//  웨이브 카운트에 포함
	RegisterSpawnedMonster(Child);

	return Child;
}

// ---------------------------------------------------------

FVector APotatoMonsterSpawner::GetSpawnLocationByGroup(FName /*SpawnGroup*/) const
{
	const FVector Origin = GetActorLocation();
	const float Radius = 300.f;

	const FVector2D Rand2D = FVector2D(
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f)
	).GetSafeNormal();

	const float Dist = FMath::FRandRange(0.f, Radius);

	FVector Loc = Origin + FVector(Rand2D.X, Rand2D.Y, 0.f) * Dist;
	Loc.Z += 20.f;
	return Loc;
}