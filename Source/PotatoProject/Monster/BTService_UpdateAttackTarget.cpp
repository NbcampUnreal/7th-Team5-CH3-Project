// ============================================================================
// BTService_UpdateAttackTarget.cpp (STABILIZED + Player Targeting) - Utils Refactor
// - 기존 로직 유지
// - CPP-local helpers 제거 -> 공용 Utils 사용
// ============================================================================

#include "BTService_UpdateAttackTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Components/CapsuleComponent.h"

#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

#include "PotatoMonster.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

#include "Monster/Utils/PotatoMath2D.h"
#include "Monster/Utils/PotatoTargetGeometry.h"
#include "Monster/Utils/PotatoMonsterRuntimeUtils.h"
#include "Monster/Utils/PotatoPlayerQueryUtils.h"

// =========================================================
// Extra helpers (file-local, minimal)
// =========================================================

// 너 원본이 "거리"를 원해서 sq 버전 대신 거리 버전 하나만 유지
static FORCEINLINE float DistancePointToSegment2D_Dist(const FVector& P3, const FVector& A3, const FVector& B3)
{
	const FVector2D P(P3.X, P3.Y);
	const FVector2D A(A3.X, A3.Y);
	const FVector2D B(B3.X, B3.Y);

	const FVector2D AB = B - A;
	const float AB2 = FVector2D::DotProduct(AB, AB);

	if (AB2 <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::Distance(A, P);
	}

	const float T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / AB2, 0.f, 1.f);
	const FVector2D Closest = A + T * AB;
	return FVector2D::Distance(Closest, P);
}

static bool IsAliveDestructibleStructure(AActor* A)
{
	if (!IsValid(A)) return false;

	if (APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(A))
	{
		if (S->StructureData && S->StructureData->bIsDestructible)
		{
			return (S->CurrentHealth > 0.f);
		}
	}
	return false;
}

// =========================================================
// Service
// =========================================================

UBTService_UpdateAttackTarget::UBTService_UpdateAttackTarget()
{
	NodeName = TEXT("Update AttackTarget (Orthodox + Player)");
	Interval = 0.15f;

	AttackTargetKey.SelectedKeyName = TEXT("CurrentTarget");
	InAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");
	PlayerActorKey.SelectedKeyName = TEXT("PlayerActor");
}

uint16 UBTService_UpdateAttackTarget::GetInstanceMemorySize() const
{
	return sizeof(FUpdateAttackTargetMemory);
}

bool UBTService_UpdateAttackTarget::ComputeInAttackRange(APotatoMonster* M, AActor* Target, float Range) const
{
	if (!M || !IsValid(Target)) return false;
	if (Range <= 0.f) return false;

	//  기존 정책 유지: From은 캡슐 위치 우선
	FVector From = M->GetActorLocation();
	float CapsuleR = 34.f;
	if (UCapsuleComponent* Cap = M->FindComponentByClass<UCapsuleComponent>())
	{
		From = Cap->GetComponentLocation();
		CapsuleR = Cap->GetScaledCapsuleRadius();
	}

	const float EffectiveRange = Range + CapsuleR;
	const float RangeSq = FMath::Square(EffectiveRange);

	//  TargetGeometry 유틸: Prim->ClosestPoint / fallback bounds
	FVector Closest2D;
	if (GetClosestPoint2DOnTarget(Target, From, Closest2D))
	{
		return FVector::DistSquared2D(From, Closest2D) <= RangeSq;
	}

	// fallback (거의 안 탐)
	return FVector::DistSquared2D(From, Target->GetActorLocation()) <= FMath::Square(EffectiveRange + BoundsRangePadding);
}

// ---------------------------------------------------------
//  FINAL: Dynamic corridor blocker picker
// ---------------------------------------------------------
AActor* UBTService_UpdateAttackTarget::FindBestBlockerOnCorridor(
	APotatoMonster* M,
	float SearchRange,
	const FVector& CorridorGoalLoc) const
{
	if (!M || !M->GetWorld()) return nullptr;

	UWorld* World = M->GetWorld();

	const FVector MyLoc = M->GetActorLocation();

	const float MyRadius = GetMonsterCapsuleRadiusSafe(M, 34.f);
	const float CorridorWidth = FMath::Max(80.f, MyRadius * 2.2f + 30.f);

	const FVector Goal2D(CorridorGoalLoc.X, CorridorGoalLoc.Y, MyLoc.Z);
	FVector ToGoal2D = (Goal2D - MyLoc).GetSafeNormal2D();
	if (ToGoal2D.IsNearlyZero())
	{
		return nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRange);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PotatoTargetingCorridor), false);
	Params.AddIgnoredActor(M);

	const FCollisionObjectQueryParams ObjParams = FCollisionObjectQueryParams::AllObjects;

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		MyLoc,
		FQuat::Identity,
		ObjParams,
		Sphere,
		Params
	);

	if (!bHit) return nullptr;

	APotatoPlaceableStructure* Best = nullptr;
	float BestScore = BIG_NUMBER;

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* A = R.GetActor();
		if (!IsValid(A)) continue;

		APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(A);
		if (!S || !S->StructureData) continue;
		if (!S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		FVector SPoint2D;
		if (!GetClosestPoint2DOnTarget(S, MyLoc, SPoint2D))
			continue;

		const FVector SLoc2D(SPoint2D.X, SPoint2D.Y, MyLoc.Z);

		const FVector ToStruct2D = (SLoc2D - MyLoc).GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(ToGoal2D, ToStruct2D);
		if (ForwardDot < 0.0f)
			continue;

		const float SegDist = DistancePointToSegment2D_Dist(SLoc2D, MyLoc, Goal2D);
		if (SegDist > CorridorWidth)
			continue;

		const float Along = FVector::DotProduct((SLoc2D - MyLoc), ToGoal2D);
		if (Along < 0.f)
			continue;

		const float Score = SegDist * 10000.f + Along;

		if (Score < BestScore)
		{
			BestScore = Score;
			Best = S;
		}
	}

	return Best;
}

bool UBTService_UpdateAttackTarget::ShouldKeepCurrentStructureTarget(
	APotatoMonster* M,
	AActor* CurrentTarget,
	const FVector& CorridorGoalLoc) const
{
	if (!M || !IsValid(CurrentTarget)) return false;

	APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(CurrentTarget);
	if (!S || !S->StructureData) return false;
	if (!S->StructureData->bIsDestructible) return false;
	if (S->CurrentHealth <= 0.f) return false;

	const float MyRadius = GetMonsterCapsuleRadiusSafe(M, 34.f);
	const float CorridorWidth = FMath::Max(80.f, MyRadius * 2.2f + 30.f);
	const float KeepWidth = CorridorWidth + FMath::Max(0.f, KeepTargetExtraWidth);

	FVector SPoint2D;
	GetClosestPoint2DOnTarget(S, M->GetActorLocation(), SPoint2D);

	const FVector MyLoc = M->GetActorLocation();
	const FVector Goal2D(CorridorGoalLoc.X, CorridorGoalLoc.Y, MyLoc.Z);
	const FVector SLoc2D(SPoint2D.X, SPoint2D.Y, MyLoc.Z);

	const float SegDist = DistancePointToSegment2D_Dist(SLoc2D, MyLoc, Goal2D);
	return (SegDist <= KeepWidth);
}

void UBTService_UpdateAttackTarget::UpdateBlockedState(
	FUpdateAttackTargetMemory& Mem,
	APotatoMonster* M,
	const FVector& CorridorGoalLoc,
	float DeltaSeconds) const
{
	if (!M) return;

	const float Dist = FVector::Dist2D(M->GetActorLocation(), CorridorGoalLoc);

	if (Mem.LastDistToMoveTarget2D < 0.f)
	{
		Mem.LastDistToMoveTarget2D = Dist;
		Mem.NoProgressTime = 0.f;
		Mem.bBlocked = false;
		return;
	}

	const float Delta = Mem.LastDistToMoveTarget2D - Dist;
	if (Delta < NoProgressMinDelta)
	{
		Mem.NoProgressTime += DeltaSeconds;
	}
	else
	{
		Mem.NoProgressTime = 0.f;
	}

	Mem.bBlocked = (Mem.NoProgressTime >= NoProgressHoldTime);
	Mem.LastDistToMoveTarget2D = Dist;
}

// ---------------------------------------------------------
//  FINAL: TickNode Integrated + Player
// ---------------------------------------------------------
void UBTService_UpdateAttackTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M || M->bIsDead) return;

	FUpdateAttackTargetMemory* Mem = reinterpret_cast<FUpdateAttackTargetMemory*>(NodeMemory);

	// =========================================================
	// 1) Warehouse 확보
	// =========================================================
	AActor* Warehouse = Cast<AActor>(BB->GetValueAsObject(WarehouseKeyName));
	if (!IsValid(Warehouse))
	{
		Warehouse = M->WarehouseActor.Get();
		if (IsValid(Warehouse))
		{
			BB->SetValueAsObject(WarehouseKeyName, Warehouse);
		}
	}

	// =========================================================
	// 2) Corridor Goal 결정
	// =========================================================
	const bool bIsLaneMode = BB->GetValueAsBool(TEXT("bIsLaneMode"));

	AActor* LaneMoveTarget = nullptr;
	if (bIsLaneMode)
	{
		LaneMoveTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("MoveTarget")));
	}

	const FVector MyLoc = M->GetActorLocation();
	const FVector CorridorGoalLoc =
		IsValid(LaneMoveTarget) ? LaneMoveTarget->GetActorLocation() :
		(IsValid(Warehouse) ? Warehouse->GetActorLocation() : MyLoc);

	// =========================================================
	// 3) 막힘 감지(옵션)
	// =========================================================
	if (Mem)
	{
		UpdateBlockedState(*Mem, M, CorridorGoalLoc, DeltaSeconds);
	}

	const float Range = M->FinalStats.AttackRange;

	// =========================================================
	// 3.5) Player 확보 + "Warehouse 근처면 무시"
	// =========================================================
	APawn* PlayerPawn = GetPlayerPawnSafe(M->GetWorld());
	const bool bHasPlayer = IsValidAttackablePlayer(M, PlayerPawn);

	if (PlayerActorKey.SelectedKeyName != NAME_None)
	{
		if (bHasPlayer) BB->SetValueAsObject(PlayerActorKey.SelectedKeyName, PlayerPawn);
		else BB->ClearValue(PlayerActorKey.SelectedKeyName);
	}

	const float DistToWarehouse2D =
		IsValid(Warehouse) ? FVector::Dist2D(MyLoc, Warehouse->GetActorLocation()) : BIG_NUMBER;

	const bool bIgnorePlayerNearWarehouse =
		IsValid(Warehouse) && (DistToWarehouse2D <= IgnorePlayerWhenNearWarehouseDist);

	// =========================================================
	// 4) 공격 중이면 타겟 재선정 잠금
	// =========================================================
	const bool bIsAttacking = BB->GetValueAsBool(TEXT("bIsAttacking"));

	AActor* Current = Cast<AActor>(BB->GetValueAsObject(AttackTargetKey.SelectedKeyName));
	AActor* Target = nullptr;

	if (bIsAttacking && IsValid(Current))
	{
		const bool bCurrentIsPlayer = (bHasPlayer && Current == PlayerPawn);
		if (bCurrentIsPlayer || (IsValid(Warehouse) && Current == Warehouse) || IsAliveDestructibleStructure(Current))
		{
			Target = Current;
		}
	}

	// =========================================================
	// 5) 타겟 선정
	// =========================================================
	if (!IsValid(Target))
	{
		Target = Warehouse;

		if (IsValid(Current) && Current != Warehouse && ShouldKeepCurrentStructureTarget(M, Current, CorridorGoalLoc))
		{
			Target = Current;
		}
		else
		{
			float SearchRange = FMath::Max(Range + CorridorSearchExtraRange, MinCorridorSearchRange);

			if (bIsLaneMode)
			{
				SearchRange = FMath::Max(SearchRange, 800.f);
			}

			if (Mem && Mem->bBlocked)
			{
				SearchRange += BlockedSearchExtra;
			}

			if (AActor* Blocker = FindBestBlockerOnCorridor(M, SearchRange, CorridorGoalLoc))
			{
				Target = Blocker;
			}
		}
	}

	// =========================================================
	// 6) 플레이어 override
	// =========================================================
	const bool bTargetIsWarehouse = (IsValid(Target) && IsValid(Warehouse) && Target == Warehouse);
	const bool bTargetIsBlocker = (IsValid(Target) && Target != Warehouse && IsAliveDestructibleStructure(Target));

	bool bTargetInRange = IsValid(Target) ? ComputeInAttackRange(M, Target, Range) : false;
	const bool bBlockerInRange = (bTargetIsBlocker && bTargetInRange);

	const float PlayerCheckRange = Range + PlayerSenseExtraRange;
	const bool bPlayerInRange =
		(!bIgnorePlayerNearWarehouse && bHasPlayer)
		? ComputeInAttackRange(M, PlayerPawn, PlayerCheckRange)
		: false;

	if (!bBlockerInRange && bPlayerInRange)
	{
		Target = PlayerPawn;
		bTargetInRange = true;
	}

	// =========================================================
	// 7) BB 반영
	// =========================================================
	BB->SetValueAsObject(AttackTargetKey.SelectedKeyName, Target);
	BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bTargetInRange);

	const bool bTargetIsPlayer = (bHasPlayer && IsValid(Target) && Target == PlayerPawn);
	const bool bFinalTargetIsWarehouse = (IsValid(Target) && IsValid(Warehouse) && Target == Warehouse);
	const bool bFinalTargetIsBlocker = (IsValid(Target) && Target != Warehouse && IsAliveDestructibleStructure(Target));

	// (선택) 타겟 변경 순간 StopMovement(블로커 새로 잡을 때만)
	if (Mem)
	{
		if (Mem->LastTarget.Get() != Target)
		{
			if (bFinalTargetIsBlocker)
			{
				AIC->StopMovement();
			}
			Mem->LastTarget = Target;
		}
	}

	// =========================================================
	// 8) MoveGoalLocation 운영 규칙
	// =========================================================
	if (!IsValid(Target))
	{
		BB->ClearValue(MoveGoalLocationKeyName);
	}
	else if (bTargetInRange)
	{
		AIC->StopMovement();
		BB->SetValueAsVector(MoveGoalLocationKeyName, MyLoc);
	}
	else
	{
		const float CapsuleR = GetMonsterCapsuleRadiusSafe(M, 34.f);
		const float Extra = CapsuleR + ApproachExtraOffset;

		// 플레이어 추격용
		if (bTargetIsPlayer)
		{
			FVector Approach;
			if (ComputeApproachPoint2D(M, Target, Extra, Approach))
			{
				BB->SetValueAsVector(MoveGoalLocationKeyName, Approach);
			}
			else
			{
				BB->SetValueAsVector(MoveGoalLocationKeyName, Target->GetActorLocation());
			}
		}
		// 레인 따라가는 중 + Warehouse 기본이면 MoveGoalLocation은 비움
		else if (bIsLaneMode && bFinalTargetIsWarehouse && !bFinalTargetIsBlocker)
		{
			BB->ClearValue(MoveGoalLocationKeyName);
		}
		else
		{
			FVector Approach;
			if (ComputeApproachPoint2D(M, Target, Extra, Approach))
			{
				BB->SetValueAsVector(MoveGoalLocationKeyName, Approach);
			}
			else
			{
				BB->SetValueAsVector(MoveGoalLocationKeyName, Target->GetActorLocation());
			}
		}
	}

	BB->SetValueAsObject(MoveGoalKeyName, Target);
}