// Monster/SpecialSkillExecution.cpp

#include "SpecialSkillExecution.h"
#include "SpecialSkillComponent.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/DamageType.h"

#include "PotatoDotComponent.h"
#include "PotatoBuffComponent.h"
#include "PotatoMonsterProjectile.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "PotatoFirePillarActor.h"
#include "EngineUtils.h" // TActorIterator

// ============================================================
// Target Helpers
// ============================================================

static FORCEINLINE bool IsActorSafeToTouch(AActor* A)
{
	return IsValid(A) && !A->IsActorBeingDestroyed();
}

static UWorld* GetSafeWorld(const UObject* Obj)
{
	return Obj ? Obj->GetWorld() : nullptr;
}

static bool TryGetOwnerSocketWorldTransform(AActor* Owner, FName SocketName, FTransform& OutXf)
{
	if (!IsActorSafeToTouch(Owner)) return false;
	if (SocketName.IsNone()) return false;

	const ACharacter* AsChar = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Skel = AsChar ? AsChar->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!Skel) return false;
	if (!Skel->DoesSocketExist(SocketName)) return false;

	OutXf = Skel->GetSocketTransform(SocketName, RTS_World);
	return true;
}

// ------------------------------------------------------------
// Ground snap (line trace down)
// ------------------------------------------------------------

static FVector SnapToGroundIfNeeded(UWorld* World, const FVector& InLoc, float TraceDist, const AActor* IgnoreActor)
{
	if (!World) return InLoc;
	if (TraceDist <= 0.f) return InLoc;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillSnapToGround), false);
	if (IgnoreActor) Params.AddIgnoredActor(IgnoreActor);

	const FVector Start = InLoc + FVector(0, 0, TraceDist * 0.5f);
	const FVector End   = InLoc - FVector(0, 0, TraceDist);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,   //  바닥 스냅은 보통 Visibility
		Params
	);

	if (bHit && Hit.bBlockingHit)
	{
		// 살짝 띄우고 싶으면 +Z 오프셋 추가 가능
		return Hit.ImpactPoint;
	}

	return InLoc;
}

// ------------------------------------------------------------
// Spawn transform resolver (Row.SpawnMode + bSnapToGround)
// ------------------------------------------------------------

static FTransform ResolveSpawnTransform(
	AActor* Owner,
	AActor* TargetForInit,
	const FPotatoMonsterSpecialSkillPresetRow& Row
)
{
	FVector Loc = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	FRotator Rot = Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;

	UWorld* World = Owner ? Owner->GetWorld() : nullptr;

	switch (Row.SpawnMode)
	{
	case EPotatoSkillSpawnMode::Projectile:
		{
			FTransform SocketXf;
			if (TryGetOwnerSocketWorldTransform(Owner, Row.SpawnSocket, SocketXf))
			{
				Loc = SocketXf.GetLocation();
				Rot = SocketXf.GetRotation().Rotator();
			}
			Loc += Rot.RotateVector(Row.SpawnOffset);
		}
		break;

	case EPotatoSkillSpawnMode::AtOwner:
		{
			Loc = Owner->GetActorLocation();
			Rot = Owner->GetActorRotation();
			Loc += Rot.RotateVector(Row.SpawnOffset);
		}
		break;

	case EPotatoSkillSpawnMode::AtTarget:
		{
			if (IsActorSafeToTouch(TargetForInit))
			{
				Loc = TargetForInit->GetActorLocation();
			}
			else
			{
				Loc = Owner->GetActorLocation();
			}

			// -----------------------------
			//  Target 바라보기 옵션
			// -----------------------------
			if (Row.bFaceTargetOnSpawn && IsActorSafeToTouch(TargetForInit))
			{
				const FVector Dir = (TargetForInit->GetActorLocation() - Owner->GetActorLocation());
				FRotator LookRot = Dir.Rotation();

				if (Row.bYawOnlyRotation)
				{
					LookRot.Pitch = 0.f;
					LookRot.Roll = 0.f;
				}

				Rot = LookRot;
			}
			else
			{
				Rot = Owner->GetActorRotation();
			}

			Loc += Row.SpawnOffset;
		}
		break;

	case EPotatoSkillSpawnMode::ForwardOffset:
		{
			Loc = Owner->GetActorLocation() +
				  Owner->GetActorForwardVector() * FMath::Max(0.f, Row.Range);

			Rot = Owner->GetActorRotation();
			Loc += Rot.RotateVector(Row.SpawnOffset);
		}
		break;

	default:
		break;
	}

	// -----------------------------
	// Ground snap
	// -----------------------------
	if (Row.bSnapToGround)
	{
		const float TraceDist = (Row.GroundTraceDistance > 0.f)
			? Row.GroundTraceDistance
			: 5000.f;

		Loc = SnapToGroundIfNeeded(World, Loc, TraceDist, Owner);
	}

	return FTransform(Rot, Loc);
}
static bool IsPlayerLikeActor(AActor* A)
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (A->ActorHasTag(TEXT("Player"))) return true;

	if (APawn* P = Cast<APawn>(A))
	{
		return (Cast<APlayerController>(P->GetController()) != nullptr);
	}
	return false;
}

static bool IsLiveDestructibleStructure(AActor* A)
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;

	if (APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(A))
	{
		if (S->StructureData && S->StructureData->bIsDestructible)
		{
			return (S->CurrentHealth > 0.f);
		}
	}
	return false;
}

static bool IsValidAoETarget(AActor* A)
{
	return IsPlayerLikeActor(A) || IsLiveDestructibleStructure(A);
}

// ============================================================
// Safety Helpers (Crash prevention)
// ============================================================

static UWorld* GetSafeWorld(USpecialSkillComponent* Comp)
{
	if (!Comp) return nullptr;
	UWorld* W = Comp->GetWorld();
	if (!W) return nullptr;
	if (W->bIsTearingDown) return nullptr;
	return W;
}

static bool IsActorSafeToTouch(AActor* A, UWorld* World)
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (A->HasAnyFlags(RF_BeginDestroyed)) return false;
	if (World && A->GetWorld() != World) return false;
	return true;
}

static AController* GetSafeInstigatorController(AActor* Owner)
{
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return nullptr;

	if (APawn* P = Cast<APawn>(Owner))
	{
		if (AController* C = P->GetController())
		{
			return C;
		}
	}
	return Owner->GetInstigatorController();
}

static void ApplyDamage_Safe(AActor* Victim, float Damage, AActor* Owner)
{
	if (!IsValid(Victim) || Victim->IsActorBeingDestroyed()) return;
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;
	if (Damage <= 0.f) return;

	// 구조물/액터에서 데미지 수신 자체가 꺼져 있으면 여기서 컷
	if (!Victim->CanBeDamaged())
	{
		return;
	}

	AController* InstigatorCon = GetSafeInstigatorController(Owner);

	UGameplayStatics::ApplyDamage(
		Victim,
		Damage,
		InstigatorCon,
		Owner,
		UDamageType::StaticClass()
	);
}

template<typename TComp>
static TComp* FindOrAddComponentSafe(AActor* Host, UWorld* World)
{
	if (!IsActorSafeToTouch(Host, World)) return nullptr;

	TComp* Existing = Host->FindComponentByClass<TComp>();
	if (Existing && IsValid(Existing) && Existing->IsRegistered())
	{
		return Existing;
	}

	if (!World || World->bIsTearingDown) return nullptr;

	TComp* NewC = NewObject<TComp>(Host, TComp::StaticClass(), NAME_None, RF_Transient);
	if (!NewC) return nullptr;

	NewC->RegisterComponentWithWorld(World);
	return NewC;
}

// ============================================================
// Math Helpers (2D)
// ============================================================

static FORCEINLINE FVector To2D(const FVector& V)
{
	return FVector(V.X, V.Y, 0.f);
}

static FORCEINLINE FVector2D To2D2(const FVector& V)
{
	return FVector2D(V.X, V.Y);
}

static FORCEINLINE void GetActorBounds3D(AActor* A, FVector& OutCenter, FVector& OutExtent)
{
	FVector C, E;
	A->GetActorBounds(true, C, E);
	OutCenter = C;
	OutExtent = E;
}

static FORCEINLINE FVector2D AABB_Min2D(const FVector& Center3, const FVector& Extent3)
{
	return FVector2D(Center3.X - Extent3.X, Center3.Y - Extent3.Y);
}

static FORCEINLINE FVector2D AABB_Max2D(const FVector& Center3, const FVector& Extent3)
{
	return FVector2D(Center3.X + Extent3.X, Center3.Y + Extent3.Y);
}

static float DistPointToAABB2D_Sq(const FVector& P3, const FVector& BoxCenter3, const FVector& BoxExtent3)
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

static FVector ClosestPointOnAABB2D(const FVector& P3, const FVector& BoxCenter3, const FVector& BoxExtent3)
{
	FVector Out = P3;
	Out.X = FMath::Clamp(P3.X, BoxCenter3.X - BoxExtent3.X, BoxCenter3.X + BoxExtent3.X);
	Out.Y = FMath::Clamp(P3.Y, BoxCenter3.Y - BoxExtent3.Y, BoxCenter3.Y + BoxExtent3.Y);
	Out.Z = 0.f;
	return Out;
}

static bool SegmentIntersectsAABB2D(const FVector2D& A, const FVector2D& B, const FVector2D& Min, const FVector2D& Max)
{
	// Liang–Barsky clipping in 2D
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

	// x
	if (!Clip(-D.X, A.X - Min.X)) return false;
	if (!Clip( D.X, Max.X - A.X)) return false;
	// y
	if (!Clip(-D.Y, A.Y - Min.Y)) return false;
	if (!Clip( D.Y, Max.Y - A.Y)) return false;

	return true;
}

static float DistPointToSegment2D_Sq_V2(const FVector2D& P, const FVector2D& A, const FVector2D& B)
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

static float DistSegmentToAABB2D_Sq(
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

	// 3) AABB 코너 -> 세그먼트 최소거리 (스치기 보강)
	const FVector2D C0(Min.X, Min.Y);
	const FVector2D C1(Max.X, Min.Y);
	const FVector2D C2(Max.X, Max.Y);
	const FVector2D C3(Min.X, Max.Y);

	Best = FMath::Min(Best, DistPointToSegment2D_Sq_V2(C0, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq_V2(C1, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq_V2(C2, SegA, SegB));
	Best = FMath::Min(Best, DistPointToSegment2D_Sq_V2(C3, SegA, SegB));

	return Best;
}

static bool IsPointInCone2D(
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

// ============================================================
// Execution Resolve
// ============================================================

static EMonsterSpecialExecution ResolveExecution(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (Row.Execution != EMonsterSpecialExecution::None)
	{
		return Row.Execution;
	}

	switch (Row.Shape)
	{
	case EMonsterSpecialShape::Projectile: return EMonsterSpecialExecution::Projectile;
	case EMonsterSpecialShape::Aura:       return EMonsterSpecialExecution::ContactDOT;
	case EMonsterSpecialShape::SelfBuff:   return EMonsterSpecialExecution::SelfBuff;
	case EMonsterSpecialShape::Circle:
	case EMonsterSpecialShape::Cone:
	case EMonsterSpecialShape::Line:
		return EMonsterSpecialExecution::InstantAoE;
	default:
		return EMonsterSpecialExecution::None;
	}
}

// ============================================================
// Origin Resolve
// ============================================================

static FVector ResolveSkillOrigin(AActor* Owner, AActor* Target, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Owner) return FVector::ZeroVector;

	if (Row.TargetType == EMonsterSpecialTargetType::Self)
	{
		return Owner->GetActorLocation();
	}

	if (Row.TargetType == EMonsterSpecialTargetType::Location)
	{
		if (IsValid(Target) && !Target->IsActorBeingDestroyed())
		{
			return Target->GetActorLocation();
		}
		return Owner->GetActorLocation() + Owner->GetActorForwardVector() * FMath::Max(0.f, Row.Range);
	}

	return (IsValid(Target) && !Target->IsActorBeingDestroyed())
		? Target->GetActorLocation()
		: Owner->GetActorLocation();
}

// ============================================================
// Projectile / Spawn Helpers
// ============================================================

static bool PassesFxDistanceGate(UWorld* World, const FVector& SpawnLoc, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!World) return true;
	if (Row.MaxFxDistance <= 0.f) return true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return true;

	return FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc) <= Row.MaxFxDistance;
}

static UClass* ResolveClassSync(const TSoftClassPtr<AActor>& SoftClass)
{
	if (SoftClass.IsNull()) return nullptr;
	if (UClass* Loaded = SoftClass.Get()) return Loaded;
	return SoftClass.LoadSynchronous();
}
static AActor* SpawnActorFromPreset(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* TargetForInit)
{
	if (!Comp) return nullptr;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return nullptr;

	// Row.ProjectileClass를 “스폰할 액터 클래스”로 사용 (투사체/불기둥 둘 다 가능)
	UClass* SpawnClass = ResolveClassSync(Row.ProjectileClass);
	if (!SpawnClass) return nullptr;

	const FTransform SpawnXf = ResolveSpawnTransform(Owner, TargetForInit, Row);

	if (!PassesFxDistanceGate(World, SpawnXf.GetLocation(), Row))
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = World->SpawnActor<AActor>(SpawnClass, SpawnXf, Params);
	if (!Spawned) return nullptr;

	// ============================================================
	//  스폰된 액터 타입에 따라 “Row -> Init 주입”
	// - Execution=Projectile은 스폰만 담당하지만,
	//   스폰된 액터가 스스로 스킬 동작을 하려면 파라미터 주입은 필요
	// ============================================================

	// 1) Stingray 독침: Projectile + OnHit DOT (+ 폭발 AoE DOT)
	if (APotatoMonsterProjectile* Proj = Cast<APotatoMonsterProjectile>(Spawned))
	{
		// 방향/속도는 너 기존 정책대로 (Row에 있으면 Row, 없으면 Owner forward)
		const FVector Dir = (TargetForInit && IsActorSafeToTouch(TargetForInit, World))
			? (TargetForInit->GetActorLocation() - Spawned->GetActorLocation()).GetSafeNormal()
			: Owner->GetActorForwardVector();

		// 속도 Row에 있으면 사용, 아니면 기존 기본값 유지
		const float Speed = (Row.ProjectileSpeed > 0.f) ? Row.ProjectileSpeed : 0.f;
		if (Speed > 0.f) Proj->InitVelocity(Dir, Speed);
		else Proj->InitVelocity(Dir, 0.f);

		// 스킬 모드 주입
		const float Dps  = FMath::Max(0.f, Row.DotDps);
		const float Dur  = FMath::Max(0.f, Row.DotDuration);
		const float Tick = FMath::Max(0.05f, Row.DotTickInterval);

		// 폭발 반경: Row에 별도 필드 없으면 Radius 재사용 가능
		const float ExplodeR = (Row.ExplodeRadius > 0.f) ? Row.ExplodeRadius : 0.f;

		Proj->InitSkillDot(Dps, Dur, Tick, ExplodeR);

		return Spawned;
	}

	// 2) Dragon 불기둥: SpawnActor + FirePillarActor가 DOT
	if (APotatoFirePillarActor* Pillar = Cast<APotatoFirePillarActor>(Spawned))
	{
		AController* InstCon = nullptr;
		if (APawn* P = Cast<APawn>(Owner)) InstCon = P->GetController();
		if (!InstCon) InstCon = Owner->GetInstigatorController();

		const float Dps  = FMath::Max(0.f, Row.DotDps);
		const float Dur  = FMath::Max(0.f, Row.DotDuration);
		const float Tick = FMath::Max(0.05f, Row.DotTickInterval);

		//  Row 변경 반영(Spawned* 사용)
		const bool bPlayerOnly = Row.bSpawnedPlayerOnly;
		const bool bMove       = Row.bSpawnedEnableMove;

		const float Life     = (Row.SpawnedLifeTime > 0.f) ? Row.SpawnedLifeTime : 5.f;
		const float MoveSpeed= (Row.SpawnedMoveSpeed > 0.f) ? Row.SpawnedMoveSpeed : 220.f;
		const float Wander   = (Row.SpawnedWanderRadius > 0.f) ? Row.SpawnedWanderRadius : 500.f;
		const float Repath   = (Row.SpawnedRepathInterval > 0.f) ? Row.SpawnedRepathInterval : 0.8f;

		Pillar->InitPillar(
			Owner,
			InstCon,
			Dps,
			Dur,
			Tick,
			Row.DotStackPolicy,
			FMath::Max(0.f, Row.Radius),
			bPlayerOnly,
			bMove,
			Life,
			MoveSpeed,
			Wander,
			Repath,
			&Row.Presentation.ExecuteVFX
		);

		return Spawned;
	}

	return Spawned;
}

// ============================================================
// Structure Gather (AABB-based, reliable)
// ============================================================

static void GatherStructures_InstantAoE(
	UWorld* World,
	AActor* Owner,
	const FPotatoMonsterSpecialSkillPresetRow& Row,
	const FVector& Origin3D,
	const FVector& Fwd3D,
	TArray<AActor*>& InOutVictims
)
{
	if (!World || !Owner) return;

	const FVector2D Origin = To2D2(Origin3D);
	const FVector2D FwdN = To2D2(Fwd3D).GetSafeNormal();

	const float Radius = FMath::Max(0.f, Row.Radius);
	const float Range  = FMath::Max(0.f, Row.Range);
	const float Width  = FMath::Max(30.f, Row.Radius);

	const float RadiusSq = Radius * Radius;
	const float WidthSq  = Width * Width;

	const float HalfRad = FMath::DegreesToRadians(FMath::Max(0.f, Row.AngleDeg) * 0.5f);
	const float CosMin  = FMath::Cos(HalfRad);

	const FVector2D LineA = Origin;
	const FVector2D LineB = Origin + FwdN * Range;

	for (TActorIterator<APotatoPlaceableStructure> It(World); It; ++It)
	{
		APotatoPlaceableStructure* S = *It;
		if (!IsValid(S) || S->IsActorBeingDestroyed()) continue;
		if (S == Owner) continue;

		if (!S->StructureData || !S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		FVector Center3, Extent3;
		GetActorBounds3D(S, Center3, Extent3);

		const FVector2D Min = AABB_Min2D(Center3, Extent3);
		const FVector2D Max = AABB_Max2D(Center3, Extent3);

		const FVector2D C0(Min.X, Min.Y);
		const FVector2D C1(Max.X, Min.Y);
		const FVector2D C2(Max.X, Max.Y);
		const FVector2D C3(Min.X, Max.Y);

		// Circle / fallback : origin -> AABB 최소거리
		if (Row.Shape == EMonsterSpecialShape::Circle || Row.Shape == EMonsterSpecialShape::None)
		{
			if (Radius <= 0.f) continue;

			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 <= RadiusSq)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// Line : segment -> AABB 최소거리
		if (Row.Shape == EMonsterSpecialShape::Line)
		{
			if (Range <= 0.f) continue;

			// 완만한 거리 컷(성능)
			{
				const float ApproxR = Range + Width + FMath::Max(Extent3.X, Extent3.Y);
				if (FVector2D::DistSquared(Origin, To2D2(Center3)) > (ApproxR * ApproxR))
				{
					continue;
				}
			}

			const float D2 = DistSegmentToAABB2D_Sq(LineA, LineB, Center3, Extent3);
			if (D2 <= WidthSq)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// Cone : (AABB가 반경에 걸림) AND (각도에 걸림)
		if (Row.Shape == EMonsterSpecialShape::Cone)
		{
			if (Radius <= 0.f) continue;

			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 > RadiusSq) continue;

			bool bHit = false;

			if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C0)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C1)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C2)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C3)) bHit = true;

			if (!bHit)
			{
				const FVector Closest3 = ClosestPointOnAABB2D(Origin3D, Center3, Extent3);
				const FVector2D Closest2 = To2D2(Closest3);
				if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, Closest2))
				{
					bHit = true;
				}
			}

			if (bHit)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// 기타: Circle 취급
		if (Radius > 0.f)
		{
			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 <= RadiusSq)
			{
				InOutVictims.AddUnique(S);
			}
		}
	}
}

// ============================================================
// Instant AoE (Pawn overlap + Structure geometry)
// ============================================================

static void Exec_InstantAoE(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	if (!Comp) return;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return;

	const float Damage = Comp->ComputeFinalDamage(Row);
	if (Damage <= 0.f) return;

	const FVector Origin3D = ResolveSkillOrigin(Owner, Target, Row);
	const FVector Fwd3D = Owner->GetActorForwardVector();

	TArray<AActor*> Victims;

	// 1) Pawn overlap (빠른 수집)
	{
		const float R = (Row.Shape == EMonsterSpecialShape::Line)
			? FMath::Max(0.f, Row.Range)
			: FMath::Max(0.f, Row.Radius);

		if (R > 0.f)
		{
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_Pawn);

			FCollisionQueryParams QParams(SCENE_QUERY_STAT(SkillAoEOverlapPawn), false, Owner);

			TArray<FOverlapResult> Overlaps;
			const bool bAny = World->OverlapMultiByObjectType(
				Overlaps,
				Origin3D,
				FQuat::Identity,
				ObjParams,
				FCollisionShape::MakeSphere(R),
				QParams
			);

			if (bAny)
			{
				for (const FOverlapResult& O : Overlaps)
				{
					AActor* A = O.GetActor();
					if (!IsActorSafeToTouch(A, World) || A == Owner) continue;
					if (!IsPlayerLikeActor(A)) continue;

					// Cone/Line 추가 필터(플레이어)
					if (Row.Shape == EMonsterSpecialShape::Cone)
					{
						const FVector To = To2D(A->GetActorLocation() - Origin3D);
						const float DistSq = FVector::DotProduct(To, To);
						if (DistSq <= KINDA_SMALL_NUMBER) continue;

						const FVector Fwd2D = To2D(Fwd3D).GetSafeNormal();
						const float Dot = FVector::DotProduct(Fwd2D, To.GetSafeNormal());

						const float HalfRad = FMath::DegreesToRadians(FMath::Max(0.f, Row.AngleDeg) * 0.5f);
						const float CosMin = FMath::Cos(HalfRad);
						if (Dot < CosMin) continue;
					}
					else if (Row.Shape == EMonsterSpecialShape::Line)
					{
						const float Range = FMath::Max(0.f, Row.Range);
						const float Width = FMath::Max(30.f, Row.Radius);
						if (Range <= 0.f) continue;

						const FVector O2 = To2D(Origin3D);
						const FVector F2 = To2D(Fwd3D).GetSafeNormal();
						const FVector A2 = O2;
						const FVector B2 = O2 + F2 * Range;

						const FVector P2 = To2D(A->GetActorLocation());
						// 기존 함수 재사용(단순)
						const FVector AB = B2 - A2;
						const float AB2 = FVector::DotProduct(AB, AB);
						if (AB2 <= KINDA_SMALL_NUMBER) continue;

						const float T = FMath::Clamp(FVector::DotProduct(P2 - A2, AB) / AB2, 0.f, 1.f);
						const FVector C = A2 + T * AB;
						const float D2 = FVector::DistSquared(C, P2);

						if (D2 > (Width * Width)) continue;
					}

					Victims.AddUnique(A);
				}
			}
		}
	}

	// 2) Structure gather (AABB 기반)
	GatherStructures_InstantAoE(World, Owner, Row, Origin3D, Fwd3D, Victims);

	// 3) Apply Damage (안전)
	for (AActor* V : Victims)
	{
		if (!IsActorSafeToTouch(V, World)) continue;

		if (Row.bHitOncePerTarget)
		{
			if (!Comp->ConsumeHitOnce(V)) continue;
		}

		ApplyDamage_Safe(V, Damage, Owner);
	}
}

// ============================================================
// Contact DOT (Aura)
// ============================================================

static void Exec_ContactDOT(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	if (!Comp) return;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return;
	if (!IsActorSafeToTouch(Target, World)) return;

	if (Row.TargetType == EMonsterSpecialTargetType::PlayerOnly && !IsPlayerLikeActor(Target))
	{
		return;
	}

	const float Dps  = FMath::Max(0.f, Row.DotDps * Comp->SpecialDamageScale);
	const float Dur  = FMath::Max(0.f, Row.DotDuration);
	const float Tick = FMath::Max(0.f, Row.DotTickInterval);
	if (Dps <= 0.f || Dur <= 0.f) return;

	UPotatoDotComponent* Dot = FindOrAddComponentSafe<UPotatoDotComponent>(Target, World);
	if (!Dot) return;

	Dot->ApplyDot(Owner, Dps, Dur, Tick, Row.DotStackPolicy);
}

// ============================================================
// Self Buff
// ============================================================

static void Exec_SelfBuff(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Comp) return;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return;

	UPotatoBuffComponent* Buff = FindOrAddComponentSafe<UPotatoBuffComponent>(Owner, World);
	if (!Buff) return;

	// Row에 BuffDuration이 없으면 DotDuration 재사용(프로젝트 정책대로 바꿔도 됨)
	const float Dur = FMath::Max(0.f, Row.DotDuration);
	if (Dur <= 0.f) return;

	const float Reduction = FMath::Clamp(Row.DamageMultiplier, 0.f, 0.99f);
	const FName BuffId = Comp->GetActiveSkillId();

	Buff->ApplyBuff(BuffId, EPotatoBuffType::DamageReduction, Reduction, Dur, Owner);
}

// ============================================================
// Entry
// ============================================================

void FSpecialSkillExecution::Execute(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& RowCopy, AActor* Target)
{
	if (!Comp) return;

	const EMonsterSpecialExecution Exec = ResolveExecution(RowCopy);

	switch (Exec)
	{
	case EMonsterSpecialExecution::InstantAoE:
		Exec_InstantAoE(Comp, RowCopy, Target);
		break;

	case EMonsterSpecialExecution::ContactDOT:
		Exec_ContactDOT(Comp, RowCopy, Target);
		break;

	case EMonsterSpecialExecution::SelfBuff:
		Exec_SelfBuff(Comp, RowCopy);
		break;

	case EMonsterSpecialExecution::Projectile:
		//  정석: 스폰만. DOT/폭발/AoE 등은 “스폰된 액터”가 처리
		SpawnActorFromPreset(Comp, RowCopy, Target);
		break;

	default:
		break;
	}
}