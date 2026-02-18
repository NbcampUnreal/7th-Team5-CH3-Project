#include "BTService_UpdateAttackTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"

#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"

#include "PotatoMonster.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

// =========================================================
// Helpers (CPP-local)
// =========================================================

static float DistancePointToSegment2D(const FVector& P3, const FVector& A3, const FVector& B3)
{
	const FVector P(P3.X, P3.Y, 0.f);
	const FVector A(A3.X, A3.Y, 0.f);
	const FVector B(B3.X, B3.Y, 0.f);

	const FVector AB = B - A;
	const float AB2 = FVector::DotProduct(AB, AB);

	if (AB2 <= KINDA_SMALL_NUMBER)
	{
		return FVector::Dist(A, P);
	}

	const float T = FMath::Clamp(FVector::DotProduct(P - A, AB) / AB2, 0.f, 1.f);
	const FVector Closest = A + T * AB;
	return FVector::Dist(Closest, P);
}

static UPrimitiveComponent* FindFirstCollisionPrimitive(AActor* Target)
{
	if (!Target) return nullptr;

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		if (RootPrim->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			return RootPrim;
	}

	TArray<UPrimitiveComponent*> Prims;
	Target->GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* C : Prims)
	{
		if (C && C->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			return C;
	}
	return nullptr;
}

static FVector ClosestPointOnAABB2D(const FVector& Point, const FVector& Origin, const FVector& Extent)
{
	FVector Closest;
	Closest.X = FMath::Clamp(Point.X, Origin.X - Extent.X, Origin.X + Extent.X);
	Closest.Y = FMath::Clamp(Point.Y, Origin.Y - Extent.Y, Origin.Y + Extent.Y);
	Closest.Z = Point.Z;
	return Closest;
}

static bool GetClosestPoint2DOnTarget(AActor* Target, const FVector& From, FVector& OutClosest2D)
{
	if (!Target) return false;

	if (UPrimitiveComponent* Prim = FindFirstCollisionPrimitive(Target))
	{
		FVector Closest3D = FVector::ZeroVector;
		const float Dist = Prim->GetClosestPointOnCollision(From, Closest3D);
		if (Dist >= 0.f)
		{
			OutClosest2D = Closest3D;
			OutClosest2D.Z = 0.f;
			return true;
		}
	}

	FVector Origin, Extent;
	Target->GetActorBounds(true, Origin, Extent);

	OutClosest2D = ClosestPointOnAABB2D(From, Origin, Extent);
	OutClosest2D.Z = 0.f;
	return true;
}

static bool ComputeApproachPoint2D(APotatoMonster* M, AActor* Target, float ExtraOffset, FVector& OutPoint)
{
	if (!M || !Target) return false;

	const FVector From = M->GetActorLocation();

	if (UPrimitiveComponent* Prim = FindFirstCollisionPrimitive(Target))
	{
		FVector Closest = FVector::ZeroVector;
		const float Dist = Prim->GetClosestPointOnCollision(From, Closest);
		if (Dist >= 0.f)
		{
			const FVector Dir = (From - Closest).GetSafeNormal2D();
			OutPoint = Closest + Dir * ExtraOffset;
			OutPoint.Z = From.Z;
			return true;
		}
	}

	FVector Origin, Extent;
	Target->GetActorBounds(true, Origin, Extent);

	const FVector Closest = ClosestPointOnAABB2D(From, Origin, Extent);
	const FVector Dir = (From - Closest).GetSafeNormal2D();

	OutPoint = Closest + Dir * ExtraOffset;
	OutPoint.Z = From.Z;
	return true;
}

static bool IsAliveDestructibleStructure(AActor* A)
{
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
	NodeName = TEXT("Update AttackTarget (Orthodox)");
	Interval = 0.15f;

	AttackTargetKey.SelectedKeyName = TEXT("CurrentTarget");
	InAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");

	// 기본 튜닝은 헤더 기본값 그대로 써도 됨
}

uint16 UBTService_UpdateAttackTarget::GetInstanceMemorySize() const
{
	return sizeof(FUpdateAttackTargetMemory);
}

bool UBTService_UpdateAttackTarget::ComputeInAttackRange(APotatoMonster* M, AActor* Target, float Range) const
{
	if (!M || !Target) return false;

	FVector From = M->GetActorLocation();
	float CapsuleR = 34.f;
	if (UCapsuleComponent* Cap = M->FindComponentByClass<UCapsuleComponent>())
	{
		From = Cap->GetComponentLocation();
		CapsuleR = Cap->GetScaledCapsuleRadius();
	}

	const float EffectiveRange = Range + CapsuleR;
	const float RangeSq = FMath::Square(EffectiveRange);

	if (UPrimitiveComponent* Prim = FindFirstCollisionPrimitive(Target))
	{
		FVector ClosestPoint = FVector::ZeroVector;
		const float Dist3D = Prim->GetClosestPointOnCollision(From, ClosestPoint);
		if (Dist3D >= 0.f)
		{
			return FVector::DistSquared2D(From, ClosestPoint) <= RangeSq;
		}
	}

	FVector Origin, Extent;
	Target->GetActorBounds(true, Origin, Extent);

	const FVector Closest = ClosestPointOnAABB2D(From, Origin, Extent);
	return FVector::DistSquared2D(From, Closest) <= FMath::Square(EffectiveRange + BoundsRangePadding);
}

AActor* UBTService_UpdateAttackTarget::FindBestBlockerOnCorridor(APotatoMonster* M, float SearchRange, const FVector& MoveTargetLoc) const
{
	if (!M || !M->GetWorld()) return nullptr;

	UWorld* World = M->GetWorld();

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRange);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PotatoTargetingCorridor), false);
	Params.AddIgnoredActor(M);

	FCollisionObjectQueryParams ObjParams = FCollisionObjectQueryParams::AllObjects;

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		M->GetActorLocation(),
		FQuat::Identity,
		ObjParams,
		Sphere,
		Params
	);

	if (!bHit) return nullptr;

	APotatoPlaceableStructure* Best = nullptr;
	float BestScore = BIG_NUMBER;

	const FVector MyLoc = M->GetActorLocation();
	const float UseWidth = BlockerWidth;

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* A = R.GetActor();
		if (!IsValid(A)) continue;

		APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(A);
		if (!S || !S->StructureData) continue;
		if (!S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		FVector SPoint2D;
		GetClosestPoint2DOnTarget(S, MyLoc, SPoint2D);

		const float SegDist = DistancePointToSegment2D(SPoint2D, MyLoc, MoveTargetLoc);
		if (SegDist > UseWidth) continue;

		const float ToDist = FVector::Dist2D(MyLoc, FVector(SPoint2D.X, SPoint2D.Y, MyLoc.Z));
		const float Score = SegDist * 10000.f + ToDist;

		if (Score < BestScore)
		{
			BestScore = Score;
			Best = S;
		}
	}

	return Best;
}

bool UBTService_UpdateAttackTarget::ShouldKeepCurrentStructureTarget(APotatoMonster* M, AActor* CurrentTarget, const FVector& MoveTargetLoc) const
{
	if (!M || !CurrentTarget) return false;

	APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(CurrentTarget);
	if (!S || !S->StructureData) return false;
	if (!S->StructureData->bIsDestructible) return false;
	if (S->CurrentHealth <= 0.f) return false;

	const float KeepWidth = BlockerWidth + FMath::Max(0.f, KeepTargetExtraWidth);

	FVector SPoint2D;
	GetClosestPoint2DOnTarget(S, M->GetActorLocation(), SPoint2D);

	const float SegDist = DistancePointToSegment2D(SPoint2D, M->GetActorLocation(), MoveTargetLoc);
	return (SegDist <= KeepWidth);
}

void UBTService_UpdateAttackTarget::UpdateBlockedState(
	FUpdateAttackTargetMemory& Mem,
	APotatoMonster* M,
	const FVector& MoveTargetLoc,
	float DeltaSeconds) const
{
	if (!M) return;

	const float Dist = FVector::Dist2D(M->GetActorLocation(), MoveTargetLoc);

	if (Mem.LastDistToMoveTarget2D < 0.f)
	{
		Mem.LastDistToMoveTarget2D = Dist;
		Mem.NoProgressTime = 0.f;
		Mem.bBlocked = false;
		return;
	}

	const float Delta = Mem.LastDistToMoveTarget2D - Dist; // 전진하면 +
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

void UBTService_UpdateAttackTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M || M->bIsDead) return;

	FUpdateAttackTargetMemory* Mem = reinterpret_cast<FUpdateAttackTargetMemory*>(NodeMemory);

	// ---- Warehouse 기준 잡기 ----
	AActor* Warehouse = Cast<AActor>(BB->GetValueAsObject(WarehouseKeyName));
	if (!IsValid(Warehouse))
	{
		Warehouse = M->WarehouseActor.Get();
		if (IsValid(Warehouse))
		{
			BB->SetValueAsObject(WarehouseKeyName, Warehouse);
		}
	}

	const FVector MoveTargetLoc = IsValid(Warehouse) ? Warehouse->GetActorLocation() : M->GetActorLocation();

	// 막힘 감지(옵션)
	if (Mem)
	{
		UpdateBlockedState(*Mem, M, MoveTargetLoc, DeltaSeconds);
	}

	const float Range = M->FinalStats.AttackRange;

	// ---- 공격 중이면 “타겟 재선정”만 잠금 (이동 목표 계산은 계속해야 멈춤 방지) ----
	const bool bIsAttacking = BB->GetValueAsBool(TEXT("bIsAttacking"));

	AActor* Current = Cast<AActor>(BB->GetValueAsObject(AttackTargetKey.SelectedKeyName));
	AActor* Target = nullptr;

	if (bIsAttacking && IsValid(Current))
	{
		if (Current == Warehouse || IsAliveDestructibleStructure(Current))
		{
			Target = Current;
		}
	}

	// ---- 타겟 선정 ----
	if (!IsValid(Target))
	{
		Target = Warehouse;

		if (IsValid(Current) && Current != Warehouse && ShouldKeepCurrentStructureTarget(M, Current, MoveTargetLoc))
		{
			Target = Current;
		}
		else
		{
			float SearchRange = FMath::Max(Range + CorridorSearchExtraRange, MinCorridorSearchRange);
			if (Mem && Mem->bBlocked)
			{
				SearchRange += BlockedSearchExtra;
			}

			if (AActor* Blocker = FindBestBlockerOnCorridor(M, SearchRange, MoveTargetLoc))
			{
				Target = Blocker;
			}
		}
	}

	// ---- 사거리 ----
	const bool bInRange = IsValid(Target) ? ComputeInAttackRange(M, Target, Range) : false;

	BB->SetValueAsObject(AttackTargetKey.SelectedKeyName, Target);
	BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bInRange);

	// ✅ 정석: Vector 목표 단일화
	if (IsValid(Target) && !bInRange)
	{
		float CapsuleR = 34.f;
		if (UCapsuleComponent* Cap = M->FindComponentByClass<UCapsuleComponent>())
		{
			CapsuleR = Cap->GetScaledCapsuleRadius();
		}

		const float Extra = CapsuleR + ApproachExtraOffset;

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
	else
	{
		BB->ClearValue(MoveGoalLocationKeyName);
	}

	// (선택) Object Goal은 디버그/표시용으로만 세팅
	BB->SetValueAsObject(MoveGoalKeyName, Target);
}
