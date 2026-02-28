#include "PotatoDamageLibrary.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"

// Utils (CPP-local helper 치환)
#include "Monster/Utils/PotatoHitOnceUtils.h"

bool UPotatoDamageLibrary::TryApplyDamageOnce(
	AActor* Victim,
	AActor* DamageCauser,
	AController* InstigatorController,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass,
	bool bHitOncePerTarget,
	TArray<AActor*>& InOutHitOnceList
)
{
	if (!IsValidVictim(Victim)) return false;
	if (!IsValid(DamageCauser)) return false;
	if (Damage <= 0.f) return false;

	if (bHitOncePerTarget)
	{
		if (HasHitAlready(InOutHitOnceList, Victim)) return false;
		MarkHit(InOutHitOnceList, Victim);
	}

	UGameplayStatics::ApplyDamage(
		Victim,
		Damage,
		InstigatorController,
		DamageCauser,
		DamageTypeClass ? *DamageTypeClass : UDamageType::StaticClass()
	);

	return true;
}

// ------------------------------------------------------------
// Overlap gather (여기는 HitOnce 유틸 대상 아님: 쿼리 정책/성능 로직)
// ------------------------------------------------------------
static void GatherOverlapPawnsBySphere(
	UWorld* World,
	const FVector& Center,
	float Radius,
	const TArray<AActor*>& IgnoreActors,
	TArray<AActor*>& OutActors,
	AActor* DamageCauserForQuery
)
{
	OutActors.Reset();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PotatoSphereOverlap), false, DamageCauserForQuery);
	for (AActor* Ig : IgnoreActors)
	{
		if (IsValid(Ig)) Params.AddIgnoredActor(Ig);
	}

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	const float R = FMath::Max(0.f, Radius);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(R);

	TArray<FOverlapResult> Overlaps;
	const bool bAny = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		ObjParams,
		Shape,
		Params
	);

	if (!bAny) return;

	TSet<AActor*> Unique;
	for (const FOverlapResult& OR : Overlaps)
	{
		AActor* A = OR.GetActor();
		if (!IsValidVictim(A)) continue;
		if (Unique.Contains(A)) continue;
		Unique.Add(A);
		OutActors.Add(A);
	}
}

int32 UPotatoDamageLibrary::ApplyAoE_Circle(
	UObject* WorldContextObject,
	AActor* DamageCauser,
	AController* InstigatorController,
	const FVector& Center,
	float Radius,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass,
	const TArray<AActor*>& IgnoreActors,
	bool bHitOncePerTarget,
	TArray<AActor*>& InOutHitOnceList
)
{
	if (!WorldContextObject) return 0;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return 0;

	CompactHitOnceList(InOutHitOnceList);

	TArray<AActor*> Overlaps;
	GatherOverlapPawnsBySphere(World, Center, Radius, IgnoreActors, Overlaps, DamageCauser);

	int32 Applied = 0;
	for (AActor* V : Overlaps)
	{
		if (UPotatoDamageLibrary::TryApplyDamageOnce(
			V, DamageCauser, InstigatorController, Damage, DamageTypeClass, bHitOncePerTarget, InOutHitOnceList))
		{
			Applied++;
		}
	}
	return Applied;
}

int32 UPotatoDamageLibrary::ApplyAoE_Cone(
	UObject* WorldContextObject,
	AActor* DamageCauser,
	AController* InstigatorController,
	const FVector& Origin,
	const FVector& Forward,
	float Radius,
	float HalfAngleDeg,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass,
	const TArray<AActor*>& IgnoreActors,
	bool bHitOncePerTarget,
	TArray<AActor*>& InOutHitOnceList
)
{
	if (!WorldContextObject) return 0;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return 0;

	CompactHitOnceList(InOutHitOnceList);

	const FVector Fwd = Forward.GetSafeNormal();
	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDeg, 0.f, 179.f)));

	TArray<AActor*> Overlaps;
	GatherOverlapPawnsBySphere(World, Origin, Radius, IgnoreActors, Overlaps, DamageCauser);

	int32 Applied = 0;
	for (AActor* V : Overlaps)
	{
		if (!IsValidVictim(V)) continue;

		const FVector To = (V->GetActorLocation() - Origin);
		const float Dist2D = FVector(To.X, To.Y, 0.f).Size();
		if (Dist2D > Radius) continue;

		const FVector Dir = To.GetSafeNormal();
		const float Dot = FVector::DotProduct(Fwd, Dir);
		if (Dot < CosThreshold) continue;

		if (UPotatoDamageLibrary::TryApplyDamageOnce(
			V, DamageCauser, InstigatorController, Damage, DamageTypeClass, bHitOncePerTarget, InOutHitOnceList))
		{
			Applied++;
		}
	}
	return Applied;
}

int32 UPotatoDamageLibrary::ApplyAoE_Line(
	UObject* WorldContextObject,
	AActor* DamageCauser,
	AController* InstigatorController,
	const FVector& Start,
	const FVector& End,
	float ThicknessRadius,
	float Damage,
	TSubclassOf<UDamageType> DamageTypeClass,
	const TArray<AActor*>& IgnoreActors,
	bool bHitOncePerTarget,
	TArray<AActor*>& InOutHitOnceList
)
{
	if (!WorldContextObject) return 0;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return 0;

	CompactHitOnceList(InOutHitOnceList);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PotatoLineAoE), false, DamageCauser);
	for (AActor* Ig : IgnoreActors)
	{
		if (IsValid(Ig)) Params.AddIgnoredActor(Ig);
	}

	TArray<FHitResult> Hits;
	const float R = FMath::Max(0.f, ThicknessRadius);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(R);

	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Shape,
		Params
	);

	if (!bHit) return 0;

	int32 Applied = 0;
	for (const FHitResult& HR : Hits)
	{
		AActor* V = HR.GetActor();
		if (!IsValidVictim(V)) continue;

		if (UPotatoDamageLibrary::TryApplyDamageOnce(
			V, DamageCauser, InstigatorController, Damage, DamageTypeClass, bHitOncePerTarget, InOutHitOnceList))
		{
			Applied++;
		}
	}
	return Applied;
}