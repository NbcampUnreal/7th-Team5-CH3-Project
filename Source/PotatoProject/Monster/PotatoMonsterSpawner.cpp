// ===============================
// PotatoMonsterSpawner.cpp (SHIPPING CLEAN + TIMER SAFE)
// ===============================
#include "PotatoMonsterSpawner.h"

#include "PotatoMonster.h"
#include "BPI_LanePathProvider.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

#include "TimerManager.h"

// Utils
#include "Utils/PotatoWaveIdUtils.h" // IsRoundOnlyName, MakeRoundWaveId, ParseRoundWaveId

APotatoMonsterSpawner::APotatoMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool APotatoMonsterSpawner::HasMetaRow(FName WaveId) const
{
	return WaveMetaTable && WaveMetaTable->FindRow<FPotatoWaveMetaRow>(WaveId, TEXT("HasMetaRow")) != nullptr;
}

bool APotatoMonsterSpawner::ResolveFirstWaveForRound(int32 Round, FName& OutWaveId, int32& OutSub)
{
	int32& NextIdx = NextSubWaveIndexByRound.FindOrAdd(Round);
	if (NextIdx <= 0) NextIdx = 1;

	const int32 ScanMax = NextIdx + 50;
	for (int32 Sub = NextIdx; Sub <= ScanMax; ++Sub)
	{
		const FName Candidate = MakeRoundWaveId(Round, Sub);
		if (HasMetaRow(Candidate))
		{
			OutWaveId = Candidate;
			OutSub = Sub;
			return true;
		}
	}
	return false;
}

bool APotatoMonsterSpawner::ResolveNextWaveForActiveRound(FName& OutWaveId, int32& OutSub)
{
	if (ActiveRound <= 0) return false;
	return ResolveFirstWaveForRound(ActiveRound, OutWaveId, OutSub);
}

// ---------------------------------------------------------
// 등록 루트 (AliveCount/OnDestroyed/SpawnedMonsters)
// ---------------------------------------------------------

void APotatoMonsterSpawner::RegisterSpawnedMonster(APotatoMonster* Monster)
{
	if (!IsValid(Monster)) return;

	AliveCount++;
	SpawnedMonsters.Add(Monster);

	// ✅ Shipping 크래시 예방 포인트:
	// - 이 AddDynamic 대상 함수는 반드시 헤더에서 UFUNCTION() 이어야 함.
	// - (아래 "검토 포인트" 참고)
	Monster->OnDestroyed.AddUniqueDynamic(this, &APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed);
}

void APotatoMonsterSpawner::HandleSpawnedMonsterDestroyed(AActor* DestroyedActor)
{
	// StopWave/EndWave(clear)에서 Destroy() 호출 시 중복 감소 방지
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
	// ===== Round input ("1") 처리 =====
	int32 Round = INDEX_NONE;
	if (IsRoundOnlyName(WaveId, Round))
	{
		if (!WaveMetaTable || !WaveSpawnTable)
		{
			return;
		}

		FName ResolvedWaveId = NAME_None;
		int32 ResolvedSub = INDEX_NONE;

		if (!ResolveFirstWaveForRound(Round, ResolvedWaveId, ResolvedSub))
		{
			return;
		}

		ActiveRound = Round;
		ActiveSubWave = ResolvedSub;
		bRoundAutoProgress = true;

		WaveId = ResolvedWaveId;
	}
	else
	{
		// ===== 핵심 수정 유지 =====
		int32 ParsedRound = INDEX_NONE;
		int32 ParsedSub = INDEX_NONE;

		if (ParseRoundWaveId(WaveId, ParsedRound, ParsedSub))
		{
			ActiveRound = ParsedRound;
			ActiveSubWave = ParsedSub;
			bRoundAutoProgress = true;
		}
		else
		{
			bRoundAutoProgress = false;
			ActiveRound = INDEX_NONE;
			ActiveSubWave = INDEX_NONE;
		}
	}

	StopWave();

	if (!WaveMetaTable || !WaveSpawnTable)
	{
		return;
	}

	const FPotatoWaveMetaRow* Meta = WaveMetaTable->FindRow<FPotatoWaveMetaRow>(
		WaveId, TEXT("StartWave:Meta")
	);
	if (!Meta)
	{
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

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			SpawnTickHandle,
			this,
			&APotatoMonsterSpawner::TickSpawn,
			CurrentSpawnInterval,
			true,
			PreDelay
		);
	}
}

void APotatoMonsterSpawner::StopWave()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(SpawnTickHandle);
	}

	PendingQueue.Reset();

	// Destroy() 중 OnDestroyed 콜백에서 AliveCount 감소되지 않게 먼저 false 처리
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
	// 둘 다 비활성이면 종료
	if (!bWaveActive && CurrentWaveId.IsNone())
	{
		return;
	}

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(SpawnTickHandle);
	}

	PendingQueue.Reset();

	const FName EndedWave = CurrentWaveId;

	// 먼저 inactive 처리 (Destroy 중 콜백 방지)
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

	// ---- 자동 진행 ----
	if (bRoundAutoProgress && Reason == EPotatoWaveEndReason::Cleared && ActiveRound > 0)
	{
		int32& NextIdx = NextSubWaveIndexByRound.FindOrAdd(ActiveRound);
		NextIdx = FMath::Max(NextIdx, ActiveSubWave + 1);

		FName NextWaveId = NAME_None;
		int32 NextSub = INDEX_NONE;

		if (ResolveNextWaveForActiveRound(NextWaveId, NextSub))
		{
			ActiveSubWave = NextSub;
			StartWave(NextWaveId);
			return;
		}

		// Round 종료: 5초 후 Broadcast
		if (UWorld* W = GetWorld())
		{
			const int32 RoundToSend = ActiveRound;

			FTimerHandle DelayHandle;
			W->GetTimerManager().SetTimer(
				DelayHandle,
				FTimerDelegate::CreateWeakLambda(this, [this, RoundToSend]()
				{
					// ✅ Spawner가 이미 파괴되었으면 WeakLambda가 실행 자체를 막아줌
					OnRoundFinished.Broadcast(RoundToSend);
				}),
				5.0f,
				false
			);
		}

		bRoundAutoProgress = false;
		ActiveRound = INDEX_NONE;
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

	if (PendingQueue.Num() == 0)
	{
		bSpawnFinished = true;

		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(SpawnTickHandle);
		}

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

		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(SpawnTickHandle);
			W->GetTimerManager().SetTimer(
				SpawnTickHandle,
				this,
				&APotatoMonsterSpawner::TickSpawn,
				CurrentSpawnInterval,
				true,
				Cur.EntryDelay
			);
		}
		return;
	}

	APotatoMonster* Spawned = SpawnOne(Cur.Type, Cur.Rank, Cur.SpawnGroup);
	if (Spawned)
	{
		Cur.Remaining--;
		if (Cur.Remaining <= 0)
		{
			PendingQueue.RemoveAt(0);

			if (UWorld* W = GetWorld())
			{
				W->GetTimerManager().ClearTimer(SpawnTickHandle);
				W->GetTimerManager().SetTimer(
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
}

// ---------------------------------------------------------
// SpawnOne
// ---------------------------------------------------------

APotatoMonster* APotatoMonsterSpawner::SpawnOne(EMonsterType Type, EMonsterRank Rank, FName SpawnGroup)
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	TSubclassOf<APotatoMonster>* ClassPtr = MonsterClassByType.Find(Type);
	if (!ClassPtr || !(*ClassPtr))
	{
		return nullptr;
	}

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

	APotatoMonster* Monster = W->SpawnActorDeferred<APotatoMonster>(
		*ClassPtr,
		Xform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (!Monster) return nullptr;

	Monster->WarehouseActor = DefaultWarehouseActor;
	Monster->Rank = Rank;
	Monster->MonsterType = Type;
	Monster->WaveBaseHP = DefaultWaveBaseHP;

	Monster->TypePresetTable = TypePresetTable;
	Monster->RankPresetTable = RankPresetTable;
	Monster->SpecialSkillPresetTable = SpecialSkillPresetTable;

	Monster->SpawnerRef = this;

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
	Monster->ApplyPresetsOnce();

	RegisterSpawnedMonster(Monster);
	return Monster;
}

// ---------------------------------------------------------
// Split child spawn (Spawner 파이프라인)
// ---------------------------------------------------------

APotatoMonster* APotatoMonsterSpawner::SpawnSplitChildFromParent(
	APotatoMonster* Parent,
	int32 ChildDepth,
	float ChildMaxHpRatio,
	float SpawnJitterRadius,
	float SpawnZOffset)
{
	if (!bWaveActive) return nullptr;

	UWorld* W = GetWorld();
	if (!W) return nullptr;

	if (!IsValid(Parent)) return nullptr;

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

	APotatoMonster* Child = W->SpawnActorDeferred<APotatoMonster>(
		ChildClass,
		Xform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (!Child) return nullptr;

	Child->CopyPresetContextFrom(Parent);

	Child->WarehouseActor = Parent->WarehouseActor;
	Child->LanePoints = Parent->LanePoints;
	Child->LaneIndex = Parent->LaneIndex;

	Child->SpawnerRef = this;

	Child->FinishSpawning(Xform);
	Child->ApplyPresetsOnce();

	const float ParentMax = FMath::Max(1.f, Parent->MaxHealth);
	const float Ratio = FMath::Max(0.01f, ChildMaxHpRatio);
	const float ChildMax = FMath::Max(1.f, ParentMax * Ratio);

	Child->MaxHealth = ChildMax;
	Child->Health = ChildMax;
	Child->RefreshHPBar();

	if (Child->SplitComp)
	{
		Child->SplitComp->SetSplitDepth(ChildDepth);
	}

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