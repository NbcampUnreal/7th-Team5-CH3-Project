// Source/PotatoProject/Monster/Utils/PotatoTargetGeometry.cpp
#include "Monster/Utils/PotatoTargetGeometry.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"

#include "../PotatoMonster.h" 

static FVector ClosestPointOnAABB2D_Local(const FVector& Point, const FVector& Origin, const FVector& Extent)
{
	FVector Closest;
	Closest.X = FMath::Clamp(Point.X, Origin.X - Extent.X, Origin.X + Extent.X);
	Closest.Y = FMath::Clamp(Point.Y, Origin.Y - Extent.Y, Origin.Y + Extent.Y);
	Closest.Z = Point.Z;
	return Closest;
}

UPrimitiveComponent* FindFirstCollisionPrimitive(AActor* Target)
{
	if (!IsValid(Target)) return nullptr;

	USceneComponent* Root = Target->GetRootComponent();
	if (!IsValid(Root)) return nullptr;

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Root))
	{
		if (IsValid(RootPrim) &&
			RootPrim->IsRegistered() &&
			RootPrim->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			return RootPrim;
		}
	}

	TInlineComponentArray<UPrimitiveComponent*> Prims;
	Target->GetComponents(Prims);

	for (UPrimitiveComponent* C : Prims)
	{
		if (!IsValid(C)) continue;
		if (!C->IsRegistered()) continue;
		if (C->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;
		return C;
	}

	return nullptr;
}

void GetTargetBoundsSafe(AActor* Target, FVector& OutOrigin, FVector& OutExtent)
{
	if (!Target)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		return;
	}

	Target->GetActorBounds(true, OutOrigin, OutExtent);

	if (OutExtent.IsNearlyZero())
	{
		Target->GetActorBounds(false, OutOrigin, OutExtent);
	}
}

static const FName TAG_DamageArea(TEXT("DamageArea"));

bool GetClosestPoint2DOnTarget(AActor* Target, const FVector& From, FVector& OutClosest2D)
{
	if (!IsValid(Target)) return false;

	UPrimitiveComponent* BestPrim = nullptr;
	FVector BestPoint = FVector::ZeroVector;
	float BestDistSq = BIG_NUMBER;

	TArray<UPrimitiveComponent*> Prims;
	Target->GetComponents<UPrimitiveComponent>(Prims);

	for (UPrimitiveComponent* Prim : Prims)
	{
		if (!IsValid(Prim)) continue;

		// ✅ 1) DamageArea 컴포넌트는 타겟팅/사거리 계산에서 제외
		if (Prim->ComponentHasTag(TAG_DamageArea))
		{
			continue;
		}

		// ✅ 2) 콜리전이 꺼진 컴포넌트는 제외
		if (Prim->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}

		// (선택) 시각효과/트리거류 제외 강화하고 싶으면 추가 가능
		// if (!Prim->IsQueryCollisionEnabled()) continue;  // 버전에 따라 함수명 다를 수 있음

		FVector Closest;
		float Dist = Prim->GetClosestPointOnCollision(From, Closest);

		if (Dist <= 0.f)
		{
			continue;
		}

		const float D2 = FVector::DistSquared2D(From, Closest);
		if (D2 < BestDistSq)
		{
			BestDistSq = D2;
			BestPrim = Prim;
			BestPoint = Closest;
		}
	}

	if (BestPrim)
	{
		OutClosest2D = FVector(BestPoint.X, BestPoint.Y, From.Z);
		return true;
	}

	// fallback: bounds 중심
	OutClosest2D = FVector(Target->GetActorLocation().X, Target->GetActorLocation().Y, From.Z);
	return true;
}

bool ComputeApproachPoint2D(APotatoMonster* M, AActor* Target, float ExtraOffset, FVector& OutPoint)
{
	if (!M || !IsValid(Target)) return false;

	const FVector From = M->GetActorLocation();

	if (UPrimitiveComponent* Prim = FindFirstCollisionPrimitive(Target))
	{
		if (IsValid(Prim) && Prim->IsRegistered())
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
	}

	FVector Origin(0), Extent(0);
	GetTargetBoundsSafe(Target, Origin, Extent);

	const FVector Closest = ClosestPointOnAABB2D_Local(From, Origin, Extent);
	const FVector Dir = (From - Closest).GetSafeNormal2D();

	OutPoint = Closest + Dir * ExtraOffset;
	OutPoint.Z = From.Z;
	return true;
}