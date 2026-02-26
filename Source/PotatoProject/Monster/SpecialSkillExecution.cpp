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

#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

#include "EngineUtils.h" // TActorIterator

// ============================================================
// Target Helpers
// ============================================================

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

static bool IsAliveDestructibleStructure(AActor* A)
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
	return IsPlayerLikeActor(A) || IsAliveDestructibleStructure(A);
}

// ============================================================
// Math Helpers (2D, Squared Distance)
// ============================================================

static FORCEINLINE FVector To2D(const FVector& V)
{
	return FVector(V.X, V.Y, 0.f);
}

static float DistPointToSegment2D_Sq(const FVector& P3, const FVector& A3, const FVector& B3)
{
	const FVector P = To2D(P3);
	const FVector A = To2D(A3);
	const FVector B = To2D(B3);

	const FVector AB = B - A;
	const float AB2 = FVector::DotProduct(AB, AB);

	if (AB2 <= KINDA_SMALL_NUMBER)
	{
		return FVector::DistSquared(A, P);
	}

	const float T = FMath::Clamp(FVector::DotProduct(P - A, AB) / AB2, 0.f, 1.f);
	const FVector C = A + T * AB;
	return FVector::DistSquared(C, P);
}

// 구조물 Bounds 기반 2D 포인트(Origin) + “반경 여유(Extent 최대)”를 제공
static void GetActorBounds2D(AActor* A, FVector& OutOrigin2D, float& OutInflateRadius)
{
	FVector Origin, Extent;
	A->GetActorBounds(true, Origin, Extent);
	OutOrigin2D = To2D(Origin);

	// Extent가 큰 구조물도 “가운데만 재면” 빗나가서,
	// 2D 반경 여유(최대 extent)를 더해준다.
	OutInflateRadius = FMath::Max(Extent.X, Extent.Y);
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
// Projectile Helpers
// ============================================================

static bool PassesFxDistanceGate(USpecialSkillComponent* Comp, const FVector& SpawnLoc, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Comp) return true;
	if (Row.MaxFxDistance <= 0.f) return true;

	UWorld* World = Comp->GetWorld();
	if (!World) return true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return true;

	return FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc) <= Row.MaxFxDistance;
}

static UClass* ResolveProjectileClass(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (Row.ProjectileClass.IsNull()) return nullptr;
	if (UClass* Loaded = Row.ProjectileClass.Get()) return Loaded;
	return Row.ProjectileClass.LoadSynchronous();
}

static FTransform ResolveProjectileSpawnTransform(AActor* Owner, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Owner) return FTransform::Identity;

	FVector Loc = Owner->GetActorLocation();
	FRotator Rot = Owner->GetActorRotation();

	const ACharacter* AsChar = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Skel = AsChar ? AsChar->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();

	if (Skel && !Row.SpawnSocket.IsNone() && Skel->DoesSocketExist(Row.SpawnSocket))
	{
		const FTransform SocketXf = Skel->GetSocketTransform(Row.SpawnSocket, RTS_World);
		Loc = SocketXf.GetLocation();
		Rot = SocketXf.GetRotation().Rotator();
	}

	Loc += Rot.RotateVector(Row.SpawnOffset);
	return FTransform(Rot, Loc);
}

static AActor* SpawnProjectileFromPreset(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Comp) return nullptr;

	UClass* ProjClass = ResolveProjectileClass(Row);
	if (!ProjClass) return nullptr;

	UWorld* World = Comp->GetWorld();
	AActor* Owner = Comp->GetOwner();
	if (!World || !Owner || Owner->IsActorBeingDestroyed()) return nullptr;

	const FTransform SpawnXf = ResolveProjectileSpawnTransform(Owner, Row);

	if (!PassesFxDistanceGate(Comp, SpawnXf.GetLocation(), Row))
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<AActor>(ProjClass, SpawnXf, Params);
}

// ============================================================
// Structure Gather (Collision-free, Optimized)
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

	const FVector Origin = To2D(Origin3D);
	const FVector Fwd = To2D(Fwd3D).GetSafeNormal();

	// shape params
	const float Radius = FMath::Max(0.f, Row.Radius);
	const float Range = FMath::Max(0.f, Row.Range);
	const float Width = FMath::Max(30.f, Row.Radius);

	// cone
	const float HalfRad = FMath::DegreesToRadians(FMath::Max(0.f, Row.AngleDeg) * 0.5f);
	const float CosMin = FMath::Cos(HalfRad);

	// precomputed squares
	const float RadiusSq = Radius * Radius;
	const float WidthSq = Width * Width;
	const float RangeSq = Range * Range;

	// line endpoints (2D)
	const FVector LineA = Origin;
	const FVector LineB = Origin + Fwd * Range;

	for (TActorIterator<APotatoPlaceableStructure> It(World); It; ++It)
	{
		APotatoPlaceableStructure* S = *It;
		if (!IsValid(S) || S->IsActorBeingDestroyed()) continue;
		if (S == Owner) continue;

		// destructible + alive only
		if (!S->StructureData || !S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		// bounds-based 2D point + inflate radius
		FVector P;
		float InflateR = 0.f;
		GetActorBounds2D(S, P, InflateR);

		// ---------------- Circle / fallback ----------------
		if (Row.Shape == EMonsterSpecialShape::Circle || Row.Shape == EMonsterSpecialShape::None)
		{
			if (Radius <= 0.f) continue;

			// (Radius + InflateR)^2
			const float R2 = FMath::Square(Radius + InflateR);
			if (FVector::DistSquared(Origin, P) <= R2)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// ---------------- Cone ----------------
		if (Row.Shape == EMonsterSpecialShape::Cone)
		{
			if (Radius <= 0.f) continue;

			const FVector To = (P - Origin);
			const float DistSq = FVector::DotProduct(To, To);

			// 원뿔은 반경 밖이면 컷 (inflate 반영)
			const float R2 = FMath::Square(Radius + InflateR);
			if (DistSq > R2 || DistSq <= KINDA_SMALL_NUMBER) continue;

			const float Dot = FVector::DotProduct(Fwd, To.GetSafeNormal());
			if (Dot >= CosMin)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// ---------------- Line ----------------
		if (Row.Shape == EMonsterSpecialShape::Line)
		{
			if (Range <= 0.f) continue;

			// 라인 전방 범위 밖이면 먼저 컷 (대략적인 원)
			if (FVector::DistSquared(Origin, P) > (RangeSq + FMath::Square(Width + InflateR)))
				continue;

			const float D2 = DistPointToSegment2D_Sq(P, LineA, LineB);
			const float W2 = FMath::Square(Width + InflateR);

			if (D2 <= W2)
			{
				InOutVictims.AddUnique(S);
			}
			continue;
		}

		// 기타 shape는 Circle 취급
		if (Radius > 0.f)
		{
			const float R2 = FMath::Square(Radius + InflateR);
			if (FVector::DistSquared(Origin, P) <= R2)
			{
				InOutVictims.AddUnique(S);
			}
		}
	}
}

// ============================================================
// Instant AoE (Pawn fast overlap + Structure geometry)
// ============================================================

static void Exec_InstantAoE(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	UWorld* World = Comp->GetWorld();
	if (!Owner || Owner->IsActorBeingDestroyed() || !World) return;

	const float Damage = Comp->ComputeFinalDamage(Row);
	if (Damage <= 0.f) return;

	const FVector Origin3D = ResolveSkillOrigin(Owner, Target, Row);
	const FVector Fwd3D = Owner->GetActorForwardVector();

	TArray<AActor*> Victims; // ✅ 안전 (기본 allocator)

	// -------------------------
	// 1) Pawn(플레이어) 빠른 Overlap
	// -------------------------
	{
		const float R = (Row.Shape == EMonsterSpecialShape::Line)
			? FMath::Max(0.f, Row.Range)
			: FMath::Max(0.f, Row.Radius);

		if (R > 0.f)
		{
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_Pawn);

			FCollisionQueryParams QParams(SCENE_QUERY_STAT(SkillAoEOverlapPawn), false, Owner);

			TArray<FOverlapResult> Overlaps; // ✅ 안전 (기본 allocator)
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
					if (!IsValid(A) || A->IsActorBeingDestroyed() || A == Owner) continue;
					if (!IsPlayerLikeActor(A)) continue;

					// Cone/Line 추가 필터
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
						const float D2 = DistPointToSegment2D_Sq(P2, A2, B2);
						if (D2 > (Width * Width)) continue;
					}

					Victims.AddUnique(A);
				}
			}
		}
	}

	// -------------------------
	// 2) Structure (collision-free geometry)
	// -------------------------
	GatherStructures_InstantAoE(World, Owner, Row, Origin3D, Fwd3D, Victims);

	// -------------------------
	// 3) Apply Damage
	// -------------------------
	for (AActor* V : Victims)
	{
		if (!IsValid(V) || V->IsActorBeingDestroyed()) continue;

		if (Row.bHitOncePerTarget)
		{
			if (!Comp->ConsumeHitOnce(V)) continue;
		}

		UGameplayStatics::ApplyDamage(
			V,
			Damage,
			Owner->GetInstigatorController(),
			Owner,
			UDamageType::StaticClass()
		);
	}
}

// ============================================================
// Contact DOT
// ============================================================

static void Exec_ContactDOT(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return;
	if (!IsValid(Target) || Target->IsActorBeingDestroyed()) return;

	// TargetType 필터가 CheckTrigger에서 이미 걸러지지만, 안전하게 한 번 더
	if (Row.TargetType == EMonsterSpecialTargetType::PlayerOnly && !IsPlayerLikeActor(Target))
	{
		return;
	}

	const float Dps = FMath::Max(0.f, Row.DotDps * Comp->SpecialDamageScale);
	const float Dur = FMath::Max(0.f, Row.DotDuration);
	const float Tick = FMath::Max(0.f, Row.DotTickInterval);

	if (Dps <= 0.f || Dur <= 0.f) return;

	UPotatoDotComponent* Dot = Target->FindComponentByClass<UPotatoDotComponent>();
	if (!Dot)
	{
		Dot = NewObject<UPotatoDotComponent>(Target, UPotatoDotComponent::StaticClass(), NAME_None, RF_Transient);
		if (Dot) Dot->RegisterComponent();
	}
	if (!Dot) return;

	Dot->ApplyDot(Owner, Dps, Dur, Tick, Row.DotStackPolicy);
}

// ============================================================
// Self Buff
// ============================================================

static void Exec_SelfBuff(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return;

	UPotatoBuffComponent* Buff = Owner->FindComponentByClass<UPotatoBuffComponent>();
	if (!Buff)
	{
		Buff = NewObject<UPotatoBuffComponent>(Owner, UPotatoBuffComponent::StaticClass(), NAME_None, RF_Transient);
		if (Buff) Buff->RegisterComponent();
	}
	if (!Buff) return;

	// ⚠️ Row에 BuffDuration이 따로 없으면 임시로 DotDuration을 재사용
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
		SpawnProjectileFromPreset(Comp, RowCopy);
		break;

	default:
		break;
	}
}