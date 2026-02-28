#include "PotatoFirePillarActor.h"

#include "Components/SphereComponent.h"

// Niagara
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

// Cascade
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"

#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "EngineUtils.h"

//  VFX 슬롯 구조체는 cpp에서 include
#include "PotatoMonsterSpecialSkillPresetRow.h"

APotatoFirePillarActor::APotatoFirePillarActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	DamageSphere->SetupAttachment(Root);

	//  Overlap 안정 세팅
	DamageSphere->SetGenerateOverlapEvents(true);
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageSphere->SetCollisionObjectType(ECC_WorldDynamic);

	// 기본: Pawn만 Overlap (구조물은 AABB iterator로 보강)
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DamageSphere->SetSphereRadius(DamageRadius);

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(Root);
	NiagaraComp->bAutoActivate = false;

	CascadeComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("CascadeComp"));
	CascadeComp->SetupAttachment(Root);
	CascadeComp->bAutoActivate = false;
}

void APotatoFirePillarActor::BeginPlay()
{
	Super::BeginPlay();

	// 디폴트 반경 동기화
	if (DamageSphere)
	{
		DamageSphere->SetSphereRadius(DamageRadius);
	}

	SpawnCenter = GetActorLocation();
	MoveTarget = SpawnCenter;
}

void APotatoFirePillarActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnableMove) return;
	if (!HasAuthority()) return;

	RepathAcc += DeltaSeconds;
	if (RepathAcc >= RepathInterval)
	{
		RepathAcc = 0.f;
		PickNewMoveTarget();
	}

	const FVector Loc = GetActorLocation();
	const FVector To = MoveTarget - Loc;
	const float Dist = To.Size();

	if (Dist > KINDA_SMALL_NUMBER && MoveSpeed > 0.f)
	{
		const FVector Step = To.GetSafeNormal() * MoveSpeed * DeltaSeconds;
		SetActorLocation(Loc + (Step.Size() >= Dist ? To : Step), false);
	}
}

void APotatoFirePillarActor::InitPillar(
	AActor* InDamageCauser,
	AController* InInstigator,
	float InDps,
	float InDuration,
	float InTickInterval,
	EMonsterDotStackPolicy InPolicy,
	float InRadius,
	bool bInPlayerOnly,
	bool bInEnableMove,
	float InLifeTime,
	float InMoveSpeed,
	float InWanderRadius,
	float InRepathInterval,
	const FPotatoSkillVFXSlot* InVfxSlot
)
{
	DamageCauser = InDamageCauser;
	InstigatorController = InInstigator;

	//  “불기둥 영역/유지시간”과 “데미지”를 정확히 매칭시키기 위해
	//    DotComponent(지속형) 대신 "틱당 ApplyDamage"로 고정.
	//    - InDuration/InPolicy는 더 이상 불기둥 딜에 영향 없음(호환 유지용으로만 저장)
	DotDps = FMath::Max(0.f, InDps);
	DotDuration = FMath::Max(0.f, InDuration);
	TickInterval = FMath::Max(0.05f, InTickInterval);
	DotPolicy = InPolicy;

	DamageRadius = FMath::Max(0.f, InRadius);
	if (DamageSphere) DamageSphere->SetSphereRadius(DamageRadius);

	bPlayerOnly = bInPlayerOnly;
	bEnableMove = bInEnableMove;

	LifeTime = FMath::Max(0.05f, InLifeTime);
	MoveSpeed = FMath::Max(0.f, InMoveSpeed);
	WanderRadius = FMath::Max(0.f, InWanderRadius);
	RepathInterval = FMath::Max(0.05f, InRepathInterval);

	SpawnCenter = GetActorLocation();
	MoveTarget = SpawnCenter;
	RepathAcc = 0.f;

	// -------------------------
	// VFX: Row 슬롯 우선, 없으면 BP 디폴트 템플릿
	// -------------------------
	if (InVfxSlot && InVfxSlot->HasAny())
	{
		ApplyVfxFromSlot(*InVfxSlot);
	}
	else
	{
		// Niagara 디폴트
		if (NiagaraTemplate && NiagaraComp)
		{
			NiagaraComp->SetAsset(NiagaraTemplate);
			NiagaraComp->Activate(true);
		}
		// Cascade 디폴트
		if (CascadeTemplate && CascadeComp)
		{
			CascadeComp->SetTemplate(CascadeTemplate);
			CascadeComp->Activate(true);
		}
	}

	// 서버에서만 의미있게
	UWorld* W = GetWorld();
	if (!HasAuthority() || !W || W->bIsTearingDown)
	{
		return;
	}

	FTimerManager& TM = W->GetTimerManager();
	TM.ClearTimer(TimerLife);
	TM.ClearTimer(TimerTick);

	//  1) 스폰 타이밍 매칭: 스폰되자마자 1틱 들어가게(딜 체감/연출 매칭)
	//  2) 유지시간 매칭: LifeTime 끝나면 Tick 타이머도 즉시 종료
	TM.SetTimer(TimerLife, this, &APotatoFirePillarActor::EndLife, LifeTime, false);

	// 첫 틱 즉시(InitialDelay=0)
	TM.SetTimer(TimerTick, this, &APotatoFirePillarActor::TickDamage, TickInterval, true, 0.f);
}

void APotatoFirePillarActor::ApplyVfxFromSlot(const FPotatoSkillVFXSlot& Slot)
{
	UNiagaraSystem* NS = Slot.Niagara.IsNull() ? nullptr : Slot.Niagara.LoadSynchronous();
	UParticleSystem* PS = Slot.Cascade.IsNull() ? nullptr : Slot.Cascade.LoadSynchronous();

	const FVector  LocOffset = Slot.LocationOffset;
	const FRotator RotOffset = Slot.RotationOffset;

	if (NiagaraComp)
	{
		NiagaraComp->DeactivateImmediate();

		if (NS)
		{
			NiagaraComp->SetAsset(NS);
			NiagaraComp->SetRelativeLocation(LocOffset);
			NiagaraComp->SetRelativeRotation(RotOffset);
			NiagaraComp->SetRelativeScale3D(Slot.Scale);
			NiagaraComp->Activate(true);
		}
	}

	if (CascadeComp)
	{
		CascadeComp->DeactivateImmediate();

		if (PS)
		{
			CascadeComp->SetTemplate(PS);
			CascadeComp->SetRelativeLocation(LocOffset);
			CascadeComp->SetRelativeRotation(RotOffset);
			CascadeComp->SetRelativeScale3D(Slot.Scale);
			CascadeComp->Activate(true);
		}
	}
}

void APotatoFirePillarActor::EndLife()
{
	//  종료 시점에 “추가 틱”이 더 들어가는 걸 방지
	if (UWorld* W = GetWorld())
	{
		FTimerManager& TM = W->GetTimerManager();
		TM.ClearTimer(TimerTick);
		TM.ClearTimer(TimerLife);
	}

	Destroy();
}

static bool IsPlayerPawn(const AActor* A)
{
	const APawn* P = Cast<APawn>(A);
	return P && P->IsPlayerControlled();
}

bool APotatoFirePillarActor::ShouldAffectActor(AActor* Other) const
{
	if (!IsValid(Other) || Other->IsActorBeingDestroyed()) return false;
	if (Other == this) return false;

	// DamageCauser(시전자) 자신 제외
	if (DamageCauser.IsValid() && Other == DamageCauser.Get()) return false;

	if (bPlayerOnly)
	{
		return IsPlayerPawn(Other);
	}

	// Player + destructible structure
	if (IsPlayerPawn(Other)) return true;

	if (APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(Other))
	{
		return (S->StructureData && S->StructureData->bIsDestructible && S->CurrentHealth > 0.f);
	}

	return false;
}

void APotatoFirePillarActor::GatherStructuresInRadius_AABB(const FVector& Origin, float Radius, TArray<AActor*>& OutVictims) const
{
	if (Radius <= 0.f) return;

	const float R2 = Radius * Radius;

	UWorld* W = GetWorld();
	if (!W) return;

	for (TActorIterator<APotatoPlaceableStructure> It(W); It; ++It)
	{
		APotatoPlaceableStructure* S = *It;
		if (!IsValid(S) || S->IsActorBeingDestroyed()) continue;

		if (!S->StructureData || !S->StructureData->bIsDestructible) continue;
		if (S->CurrentHealth <= 0.f) continue;

		// AABB 최소거리(2D)
		FVector C, E;
		S->GetActorBounds(true, C, E);

		const float MinX = C.X - E.X;
		const float MaxX = C.X + E.X;
		const float MinY = C.Y - E.Y;
		const float MaxY = C.Y + E.Y;

		const float ClampedX = FMath::Clamp(Origin.X, MinX, MaxX);
		const float ClampedY = FMath::Clamp(Origin.Y, MinY, MaxY);

		const float Dx = Origin.X - ClampedX;
		const float Dy = Origin.Y - ClampedY;
		const float D2 = Dx * Dx + Dy * Dy;

		if (D2 <= R2)
		{
			OutVictims.AddUnique(S);
		}
	}
}

void APotatoFirePillarActor::TickDamage()
{
	if (!HasAuthority()) return;

	UWorld* W = GetWorld();
	if (!W || W->bIsTearingDown) return;

	if (!DamageSphere) return;
	if (DotDps <= 0.f || TickInterval <= 0.f) return;
	if (DamageRadius <= 0.f) return;

	//  틱당 데미지 = DPS * TickInterval
	const float DamagePerTick = DotDps * TickInterval;
	if (DamagePerTick <= 0.f) return;

	TArray<AActor*> Victims;

	// 1) Pawn overlap
	DamageSphere->GetOverlappingActors(Victims);

	// 2) 구조물 보강
	if (!bPlayerOnly && bIncludeStructuresAABB)
	{
		GatherStructuresInRadius_AABB(GetActorLocation(), DamageRadius, Victims);
	}

	AActor* Causer = DamageCauser.IsValid() ? DamageCauser.Get() : this;
	AController* InstCon = InstigatorController.IsValid() ? InstigatorController.Get() : nullptr;

	for (AActor* V : Victims)
	{
		if (!ShouldAffectActor(V)) continue;
		if (!V->CanBeDamaged()) continue;

		UGameplayStatics::ApplyDamage(
			V,
			DamagePerTick,
			InstCon,
			Causer,
			UDamageType::StaticClass()
		);
	}
}

void APotatoFirePillarActor::PickNewMoveTarget()
{
	if (WanderRadius <= 0.f)
	{
		MoveTarget = SpawnCenter;
		return;
	}

	const FVector2D Rand2 = FMath::RandPointInCircle(WanderRadius);
	MoveTarget = SpawnCenter + FVector(Rand2.X, Rand2.Y, 0.f);
}