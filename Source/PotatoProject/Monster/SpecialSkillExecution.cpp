// Source/PotatoProject/Monster/SpecialSkillExecution.cpp

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
#include "EngineUtils.h" // TActorIterator

#include "SkillTransformResolver.h"

#include "PotatoDotComponent.h"
#include "PotatoBuffComponent.h"
#include "PotatoMonsterProjectile.h"
#include "PotatoFirePillarActor.h"

#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

// ✅ 공용 유틸
#include "Monster/Utils/PotatoActorSafety.h"
#include "Monster/Utils/PotatoMath2D.h"

// ============================================================
// Target Filter (Skill local)
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

// ============================================================
// Socket / Spawn helpers (Skill local)
// ============================================================

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
		ECC_Visibility,
		Params
	);

	if (bHit && Hit.bBlockingHit)
	{
		return Hit.ImpactPoint;
	}

	return InLoc;
}

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

			if (Row.bFaceTargetOnSpawn && IsActorSafeToTouch(TargetForInit))
			{
				const FVector Dir = (TargetForInit->GetActorLocation() - Owner->GetActorLocation());
				FRotator LookRot = Dir.Rotation();

				if (Row.bYawOnlyRotation)
				{
					LookRot.Pitch = 0.f;
					LookRot.Roll  = 0.f;
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

	if (Row.bSnapToGround)
	{
		const float TraceDist = (Row.GroundTraceDistance > 0.f) ? Row.GroundTraceDistance : 5000.f;
		Loc = SnapToGroundIfNeeded(World, Loc, TraceDist, Owner);
	}

	return FTransform(Rot, Loc);
}

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
// Projectile / Spawn
// ============================================================

static AActor* SpawnActorFromPreset(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* TargetForInit)
{
	if (!Comp) return nullptr;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return nullptr;

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

	// 1) Projectile
	if (APotatoMonsterProjectile* Proj = Cast<APotatoMonsterProjectile>(Spawned))
	{
		const FVector Dir = (TargetForInit && IsActorSafeToTouch(TargetForInit, World))
			? (TargetForInit->GetActorLocation() - Spawned->GetActorLocation()).GetSafeNormal()
			: Owner->GetActorForwardVector();

		const float Speed = (Row.ProjectileSpeed > 0.f) ? Row.ProjectileSpeed : 0.f;
		if (Speed > 0.f) Proj->InitVelocity(Dir, Speed);
		else Proj->InitVelocity(Dir, 0.f);

		const float Dps  = FMath::Max(0.f, Row.DotDps);
		const float Dur  = FMath::Max(0.f, Row.DotDuration);
		const float Tick = FMath::Max(0.05f, Row.DotTickInterval);

		const float ExplodeR = (Row.ExplodeRadius > 0.f) ? Row.ExplodeRadius : 0.f;
		Proj->InitSkillDot(Dps, Dur, Tick, ExplodeR);

		return Spawned;
	}

	// 2) FirePillar
	if (APotatoFirePillarActor* Pillar = Cast<APotatoFirePillarActor>(Spawned))
	{
		AController* InstCon = GetSafeInstigatorController(Owner);

		const float Dps  = FMath::Max(0.f, Row.DotDps);
		const float Dur  = FMath::Max(0.f, Row.DotDuration);
		const float Tick = FMath::Max(0.05f, Row.DotTickInterval);

		const bool  bPlayerOnly = Row.bSpawnedPlayerOnly;
		const bool  bMove       = Row.bSpawnedEnableMove;

		const float Life      = (Row.SpawnedLifeTime > 0.f) ? Row.SpawnedLifeTime : 5.f;
		const float MoveSpeed = (Row.SpawnedMoveSpeed > 0.f) ? Row.SpawnedMoveSpeed : 220.f;
		const float Wander    = (Row.SpawnedWanderRadius > 0.f) ? Row.SpawnedWanderRadius : 500.f;
		const float Repath    = (Row.SpawnedRepathInterval > 0.f) ? Row.SpawnedRepathInterval : 0.8f;

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
// Structure Gather (AABB-based)
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
		if (!IsActorSafeToTouch(S, World)) continue;
		if (S == Owner) continue;

		if (!S->StructureData || !S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		FVector Center3, Extent3;
		{
			FVector C, E;
			S->GetActorBounds(true, C, E);
			Center3 = C; Extent3 = E;
		}

		// Circle
		if (Row.Shape == EMonsterSpecialShape::Circle || Row.Shape == EMonsterSpecialShape::None)
		{
			if (Radius <= 0.f) continue;
			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 <= RadiusSq) InOutVictims.AddUnique(S);
			continue;
		}

		// Line
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
			if (D2 <= WidthSq) InOutVictims.AddUnique(S);
			continue;
		}

		// Cone
		if (Row.Shape == EMonsterSpecialShape::Cone)
		{
			if (Radius <= 0.f) continue;

			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 > RadiusSq) continue;

			const FVector2D Min = AABB_Min2D(Center3, Extent3);
			const FVector2D Max = AABB_Max2D(Center3, Extent3);

			const FVector2D C0(Min.X, Min.Y);
			const FVector2D C1(Max.X, Min.Y);
			const FVector2D C2(Max.X, Max.Y);
			const FVector2D C3(Min.X, Max.Y);

			bool bHit = false;
			if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C0)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C1)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C2)) bHit = true;
			else if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, C3)) bHit = true;

			if (!bHit)
			{
				const FVector Closest3 = ClosestPointOnAABB2D(Origin3D, Center3, Extent3);
				if (IsPointInCone2D(Origin, FwdN, CosMin, RadiusSq, To2D2(Closest3)))
				{
					bHit = true;
				}
			}

			if (bHit) InOutVictims.AddUnique(S);
			continue;
		}

		// 기타: Circle 취급
		if (Radius > 0.f)
		{
			const float D2 = DistPointToAABB2D_Sq(Origin3D, Center3, Extent3);
			if (D2 <= RadiusSq) InOutVictims.AddUnique(S);
		}
	}
}

// ============================================================
// Instant AoE
// ============================================================

static void Exec_InstantAoE(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	if (!Comp) return;

	UWorld* World = GetSafeWorld(Comp);
	AActor* Owner = Comp->GetOwner();
	if (!World || !IsActorSafeToTouch(Owner, World)) return;

	const float Damage = Comp->ComputeFinalDamage(Row);
	if (Damage <= 0.f) return;

	const FVector Origin3D = FSkillTransformResolver::ResolveOrigin(Owner, Target, Row);
	const FVector Fwd3D = Owner->GetActorForwardVector();

	TArray<AActor*> Victims;

	// 1) Pawn overlap
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

	// 2) Structure gather
	GatherStructures_InstantAoE(World, Owner, Row, Origin3D, Fwd3D, Victims);

	// 3) Apply damage
	for (AActor* V : Victims)
	{
		if (!IsActorSafeToTouch(V, World)) continue;

		if (Row.bHitOncePerTarget)
		{
			if (!Comp->ConsumeHitOnce(V)) continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[AoE] Hit Victim=%s Class=%s Damage=%.1f Owner=%s"),
			*GetNameSafe(V),
			*GetNameSafe(V ? V->GetClass() : nullptr),
			Damage,
			*GetNameSafe(Owner));

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
		SpawnActorFromPreset(Comp, RowCopy, Target);
		break;

	default:
		break;
	}
}