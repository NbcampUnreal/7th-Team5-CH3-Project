// Source/PotatoProject/Monster/Utils/PotatoMath2D.h
#pragma once

#include "CoreMinimal.h"

// ------------------------------
// 2D conversions
// ------------------------------
static FORCEINLINE FVector To2D(const FVector& V)  { return FVector(V.X, V.Y, 0.f); }
static FORCEINLINE FVector2D To2D2(const FVector& V) { return FVector2D(V.X, V.Y); }

// ------------------------------
// AABB in 2D
// ------------------------------
static FORCEINLINE FVector2D AABB_Min2D(const FVector& Center3, const FVector& Extent3)
{
	return FVector2D(Center3.X - Extent3.X, Center3.Y - Extent3.Y);
}

static FORCEINLINE FVector2D AABB_Max2D(const FVector& Center3, const FVector& Extent3)
{
	return FVector2D(Center3.X + Extent3.X, Center3.Y + Extent3.Y);
}

static FORCEINLINE float DistPointToAABB2D_Sq(const FVector& P3, const FVector& BoxCenter3, const FVector& BoxExtent3)
{
	const float Px = P3.X;
	const float Py = P3.Y;

	const float MinX = BoxCenter3.X - BoxExtent3.X;
	const float MaxX = BoxCenter3.X + BoxExtent3.X;
	const float MinY = BoxCenter3.Y - BoxExtent3.Y;
	const float MaxY = BoxCenter3.Y + BoxExtent3.Y;

	const float Cx = FMath::Clamp(Px, MinX, MaxX);
	const float Cy = FMath::Clamp(Py, MinY, MaxY);

	const float Dx = Px - Cx;
	const float Dy = Py - Cy;
	return Dx * Dx + Dy * Dy;
}

static FORCEINLINE FVector ClosestPointOnAABB2D(const FVector& P3, const FVector& BoxCenter3, const FVector& BoxExtent3)
{
	FVector Out = P3;
	Out.X = FMath::Clamp(P3.X, BoxCenter3.X - BoxExtent3.X, BoxCenter3.X + BoxExtent3.X);
	Out.Y = FMath::Clamp(P3.Y, BoxCenter3.Y - BoxExtent3.Y, BoxCenter3.Y + BoxExtent3.Y);
	Out.Z = 0.f;
	return Out;
}

// ------------------------------
// Segment vs AABB (2D) : Liang–Barsky
// ------------------------------
static FORCEINLINE bool SegmentIntersectsAABB2D(const FVector2D& A, const FVector2D& B, const FVector2D& Min, const FVector2D& Max)
{
	const FVector2D D = B - A;

	float t0 = 0.f;
	float t1 = 1.f;

	auto Clip = [&](float p, float q) -> bool
	{
		if (FMath::IsNearlyZero(p))
		{
			return q >= 0.f;
		}

		const float r = q / p;
		if (p < 0.f)
		{
			if (r > t1) return false;
			if (r > t0) t0 = r;
		}
		else
		{
			if (r < t0) return false;
			if (r < t1) t1 = r;
		}
		return true;
	};

	if (!Clip(-D.X, A.X - Min.X)) return false;
	if (!Clip( D.X, Max.X - A.X)) return false;
	if (!Clip(-D.Y, A.Y - Min.Y)) return false;
	if (!Clip( D.Y, Max.Y - A.Y)) return false;

	return true;
}

static FORCEINLINE float DistPointToSegment2D_Sq(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	const FVector2D AB = B - A;
	const float AB2 = FVector2D::DotProduct(AB, AB);

	if (AB2 <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::DistSquared(P, A);
	}

	const float t = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / AB2, 0.f, 1.f);
	const FVector2D C = A + t * AB;
	return FVector2D::DistSquared(P, C);
}

static FORCEINLINE float DistSegmentToAABB2D_Sq(
	const FVector2D& SegA,
	const FVector2D& SegB,
	const FVector& BoxCenter3,
	const FVector& BoxExtent3
)
{
	const FVector2D Min = AABB_Min2D(BoxCenter3, BoxExtent3);
	const FVector2D Max = AABB_Max2D(BoxCenter3, BoxExtent3);

	// 1) 교차하면 거리 0
	if (SegmentIntersectsAABB2D(SegA, SegB, Min, Max))
	{
		return 0.f;
	}

	// 2) 세그먼트 끝점 -> AABB 최소거리
	const FVector PA3(SegA.X, SegA.Y, 0.f);
	const FVector PB3(SegB.X, SegB.Y, 0.f);

	const float D1 = DistPointToAABB2D_Sq(PA3, BoxCenter3, BoxExtent3);
	const float D2 = DistPointToAABB2D_Sq(PB3, BoxCenter3, BoxExtent3);

	float Best = FMath::Min(D1, D2);

	// 3) AABB 코너 -> 세그먼트 최소거리(스치기 보강)
	const FVector2D C0(Min.X, Min.Y);
	const FVector2D C1(Max.X, Min.Y);
	const FVector2D C2(Max.X, Max.Y);
	const FVector2D C3(Min.X, Max.Y);

	Best = FMath::Min(Best, DistPointToSegment2D_Sq(C0, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq(C1, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq(C2, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq(C3, SegA, SegB));

	return Best;
}

// ------------------------------
// Cone test (2D)
// ------------------------------
static FORCEINLINE bool IsPointInCone2D(
	const FVector2D& Origin,
	const FVector2D& FwdN,
	float CosMin,
	float RadiusSq,
	const FVector2D& P
)
{
	const FVector2D To = P - Origin;
	const float DistSq = FVector2D::DotProduct(To, To);
	if (DistSq > RadiusSq || DistSq <= KINDA_SMALL_NUMBER) return false;

	const FVector2D ToN = To.GetSafeNormal();
	const float Dot = FVector2D::DotProduct(FwdN, ToN);
	return Dot >= CosMin;
}

static FORCEINLINE float DistancePointToSegment2D(const FVector& P3, const FVector& A3, const FVector& B3)
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