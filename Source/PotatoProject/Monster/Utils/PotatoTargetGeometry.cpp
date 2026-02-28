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

bool GetClosestPoint2DOnTarget(AActor* Target, const FVector& From, FVector& OutClosest2D)
{
	if (!IsValid(Target)) return false;

	// Prim 기반 (가능하면)
	if (UPrimitiveComponent* Prim = FindFirstCollisionPrimitive(Target))
	{
		if (IsValid(Prim) && Prim->IsRegistered())
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
	}

	// fallback: Bounds
	FVector Origin(0), Extent(0);
	GetTargetBoundsSafe(Target, Origin, Extent);

	OutClosest2D = ClosestPointOnAABB2D_Local(From, Origin, Extent);
	OutClosest2D.Z = 0.f;
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