// PotatoMonsterProjectile.cpp

#include "PotatoMonsterProjectile.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "PotatoMonster.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h" // TActorIterator
#include "Components/PrimitiveComponent.h"

// DOT
#include "PotatoDotComponent.h"

// Structure
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

APotatoMonsterProjectile::APotatoMonsterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 시각용 (선택사항)
	VFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX"));
	VFX->SetupAttachment(Root);

	Speed = 1800.f;
	TraceRadius = 12.f;
	LifeSeconds = 5.f;
	TraceChannel = ECC_WorldDynamic;

	Damage = 0.f;
	bHitOnce = false;

	// 스킬 기본값
	bSkillDotMode = false;
	DotDps = 0.f;
	DotDuration = 0.f;
	DotTickInterval = 0.2f;
	ExplodeRadius = 0.f;
	bIncludeStructuresInExplode = true;

	// 네트워크 환경에서 구조물 데미지/닷은 서버에서만 의미 있음
	SetReplicates(true);
}

void APotatoMonsterProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSeconds);
}

void APotatoMonsterProjectile::SetProjectileDamage_Implementation(float InDamage)
{
	Damage = InDamage;
}

void APotatoMonsterProjectile::InitVelocity(const FVector& InDirection, float InSpeed)
{
	MoveDir = InDirection.GetSafeNormal();
	if (!MoveDir.IsNearlyZero())
	{
		SetActorRotation(MoveDir.Rotation());
	}
	Speed = InSpeed > 0.f ? InSpeed : Speed;
}

void APotatoMonsterProjectile::InitSkillDot(float InDotDps, float InDotDuration, float InDotTickInterval, float InExplodeRadius)
{
	bSkillDotMode = true;

	DotDps = FMath::Max(0.f, InDotDps);
	DotDuration = FMath::Max(0.f, InDotDuration);
	DotTickInterval = FMath::Max(0.01f, InDotTickInterval); // 0 방지
	ExplodeRadius = FMath::Max(0.f, InExplodeRadius);

	// 스킬 투사체는 기본 Damage를 안 써도 되지만, 혼용 가능하도록 그대로 둠
}

bool APotatoMonsterProjectile::IsWorldSafe() const
{
	UWorld* World = GetWorld();
	if (!World) return false;
	if (World->bIsTearingDown) return false;
	return true;
}

bool APotatoMonsterProjectile::IsActorSafe(AActor* A) const
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (!IsWorldSafe()) return false;
	if (A->GetWorld() != GetWorld()) return false;
	return true;
}

bool APotatoMonsterProjectile::ShouldIgnore(AActor* Other) const
{
	if (!Other) return true;
	if (Other == this) return true;

	if (Other == GetOwner()) return true;
	if (Other == GetInstigator()) return true;

	if (bIgnoreAllMonsters && Other->IsA(APotatoMonster::StaticClass()))
		return true;

	return false;
}

void APotatoMonsterProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHitOnce) return;
	if (!IsWorldSafe()) { Destroy(); return; }

	const FVector Start = GetActorLocation();
	const FVector End = Start + MoveDir * Speed * DeltaSeconds;

	SetActorLocation(End, false);
	DoSweep(Start, End);
}

void APotatoMonsterProjectile::ApplyDirectDamage(AActor* Victim)
{
	if (!IsActorSafe(Victim)) return;
	if (!Victim->CanBeDamaged()) return;

	AController* InstigatorController = nullptr;
	if (APawn* InstPawn = GetInstigator())
	{
		InstigatorController = InstPawn->GetController();
	}
	if (!InstigatorController)
	{
		InstigatorController = GetInstigatorController();
	}

	UGameplayStatics::ApplyDamage(
		Victim,
		Damage,
		InstigatorController,
		this,
		UDamageType::StaticClass()
	);
}

static bool IsPlayerLikeActor_Proj(AActor* A)
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (A->ActorHasTag(TEXT("Player"))) return true;

	if (APawn* P = Cast<APawn>(A))
	{
		return (Cast<APlayerController>(P->GetController()) != nullptr);
	}
	return false;
}

static bool IsLiveDestructibleStructure_Proj(AActor* A)
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

void APotatoMonsterProjectile::ApplyDotToActor(AActor* Victim)
{
	if (!IsActorSafe(Victim)) return;
	if (!Victim->CanBeDamaged()) return;

	// 타겟 필터: 플레이어 or 파괴 가능 구조물
	if (!IsPlayerLikeActor_Proj(Victim) && !IsLiveDestructibleStructure_Proj(Victim))
	{
		return;
	}

	if (DotDps <= 0.f || DotDuration <= 0.f) return;

	// 서버에서만 적용 (구조물 TakeDamage도 Authority 요구)
	if (!HasAuthority()) return;

	UPotatoDotComponent* Dot = Victim->FindComponentByClass<UPotatoDotComponent>();
	if (!Dot)
	{
		// 안전 생성 + 월드 등록
		UWorld* World = GetWorld();
		if (!World || World->bIsTearingDown) return;

		Dot = NewObject<UPotatoDotComponent>(Victim, UPotatoDotComponent::StaticClass(), NAME_None, RF_Transient);
		if (Dot)
		{
			Dot->RegisterComponentWithWorld(World);
		}
	}
	if (!Dot) return;

	// DamageCauser는 “투사체”보다 “몬스터(Owner)”가 정책적으로 더 자연스러움
	AActor* Causer = GetOwner() ? GetOwner() : this;

	// ✅ 정석: 투사체 OnHit에서 DOT 부여
	Dot->ApplyDot(Causer, DotDps, DotDuration, DotTickInterval, /*Row 정책*/ EMonsterDotStackPolicy::RefreshDuration);
}

// ------------------------------------------------------------
// 구조물 AoE 보강 (Overlap 미검출 대비)
//  - Origin에서 Radius 안에 “AABB가 걸치면” 포함
// ------------------------------------------------------------

static float DistPointToAABB2D_Sq_Proj(const FVector& P3, const FVector& BoxCenter3, const FVector& BoxExtent3)
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

void APotatoMonsterProjectile::GatherStructuresInRadius_AABB(const FVector& Origin, float Radius, TArray<AActor*>& OutVictims) const
{
	if (!IsWorldSafe()) return;
	if (Radius <= 0.f) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const float R2 = Radius * Radius;

	for (TActorIterator<APotatoPlaceableStructure> It(World); It; ++It)
	{
		APotatoPlaceableStructure* S = *It;
		if (!IsValid(S) || S->IsActorBeingDestroyed()) continue;
		if (S == GetOwner()) continue;

		if (!S->StructureData || !S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		FVector C, E;
		S->GetActorBounds(true, C, E);

		const float D2 = DistPointToAABB2D_Sq_Proj(Origin, C, E);
		if (D2 <= R2)
		{
			OutVictims.AddUnique(S);
		}
	}
}

void APotatoMonsterProjectile::ExplodeApplyDot(const FVector& Origin)
{
	if (!IsWorldSafe()) return;
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const float R = FMath::Max(0.f, ExplodeRadius);
	if (R <= 0.f) return;

	TArray<AActor*> Victims;

	// 1) Pawn + WorldDynamic overlap (빠른 수집)
	{
		FCollisionObjectQueryParams Obj;
		Obj.AddObjectTypesToQuery(ECC_Pawn);
		Obj.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams Q(SCENE_QUERY_STAT(ProjExplodeOverlap), false);
		Q.AddIgnoredActor(this);
		if (AActor* O = GetOwner()) Q.AddIgnoredActor(O);
		if (APawn* I = GetInstigator()) Q.AddIgnoredActor(I);

		TArray<FOverlapResult> Overlaps;
		const bool bAny = World->OverlapMultiByObjectType(
			Overlaps,
			Origin,
			FQuat::Identity,
			Obj,
			FCollisionShape::MakeSphere(R),
			Q
		);

		if (bAny)
		{
			for (const FOverlapResult& Ovr : Overlaps)
			{
				AActor* A = Ovr.GetActor();
				if (!IsActorSafe(A)) continue;
				if (ShouldIgnore(A)) continue;
				Victims.AddUnique(A);
			}
		}
	}

	// 2) 구조물 보강 (Overlap로 누락될 수 있어서 AABB 체크)
	if (bIncludeStructuresInExplode)
	{
		GatherStructuresInRadius_AABB(Origin, R, Victims);
	}

	// 3) DOT 적용
	for (AActor* V : Victims)
	{
		if (!IsActorSafe(V)) continue;
		if (ShouldIgnore(V)) continue;
		ApplyDotToActor(V);
	}
}

void APotatoMonsterProjectile::DoSweep(const FVector& From, const FVector& To)
{
	if (!IsWorldSafe()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterTraceProjectile), false);
	Params.AddIgnoredActor(this);
	if (AActor* O = GetOwner()) Params.AddIgnoredActor(O);
	if (APawn* I = GetInstigator()) Params.AddIgnoredActor(I);

	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);

	const bool bAnyHit = World->SweepMultiByChannel(
		Hits,
		From,
		To,
		FQuat::Identity,
		TraceChannel,
		Shape,
		Params
	);

	if (!bAnyHit) return;

	Hits.Sort([](const FHitResult& A, const FHitResult& B)
	{
		return A.Distance < B.Distance;
	});

	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other) continue;
		if (ShouldIgnore(Other)) continue;
		if (!Other->CanBeDamaged()) continue;

		bHitOnce = true;

		// ✅ 스킬 모드(독침): DOT / 폭발 DOT
		if (bSkillDotMode)
		{
			// 서버만 (구조물/닷 안정)
			if (HasAuthority())
			{
				if (ExplodeRadius > 0.f)
				{
					ExplodeApplyDot(Hit.ImpactPoint);
				}
				else
				{
					ApplyDotToActor(Other);
				}
			}

			Destroy();
			return;
		}

		// ✅ 기본 평타: 단일 Damage
		if (HasAuthority())
		{
			ApplyDirectDamage(Other);
		}

		Destroy();
		return;
	}
}