#include "PotatoMonsterProjectile.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "PotatoMonster.h"
#include "Engine/World.h"

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

	const FVector Start = GetActorLocation();
	const FVector End = Start + MoveDir * Speed * DeltaSeconds;

	SetActorLocation(End, false);

	DoSweep(Start, End);
}

void APotatoMonsterProjectile::DoSweep(const FVector& From, const FVector& To)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterTraceProjectile), false);
	Params.AddIgnoredActor(this);
	if (AActor* O = GetOwner()) Params.AddIgnoredActor(O);
	if (APawn* I = GetInstigator()) Params.AddIgnoredActor(I);

	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);

	bool bAnyHit = World->SweepMultiByChannel(
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

		if (!Other->CanBeDamaged())
			continue;

		bHitOnce = true;

		AController* InstigatorController = nullptr;
		if (APawn* InstPawn = GetInstigator())
			InstigatorController = InstPawn->GetController();

		UGameplayStatics::ApplyDamage(
			Other,
			Damage,
			InstigatorController,
			this,
			UDamageType::StaticClass()
		);

		Destroy();
		return;
	}
}