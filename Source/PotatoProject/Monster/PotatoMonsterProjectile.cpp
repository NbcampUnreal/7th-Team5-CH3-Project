#include "PotatoMonsterProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "PotatoMonster.h"

APotatoMonsterProjectile::APotatoMonsterProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);

	Collision->InitSphereRadius(8.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetNotifyRigidBodyCollision(true);

	Collision->OnComponentHit.AddDynamic(this, &APotatoMonsterProjectile::OnHit);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 1800.f;
	Movement->MaxSpeed = 1800.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
	LifeSeconds = 5.f;
}

void APotatoMonsterProjectile::BeginPlay()
{
	Super::BeginPlay();
	InitialLifeSpan = LifeSeconds;
}

void APotatoMonsterProjectile::SetProjectileDamage_Implementation(float InDamage)
{
	Damage = InDamage;
}

bool APotatoMonsterProjectile::ShouldIgnore(AActor* OtherActor) const
{
	if (!OtherActor) return true;
	if (OtherActor == this) return true;
	if (OtherActor == GetOwner()) return true;

	// 몬스터가 쏜 탄 -> 몬스터는 무시(아군 오사 방지)
	if (OtherActor->IsA(APotatoMonster::StaticClass()))
		return true;

	return false;
}

void APotatoMonsterProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShouldIgnore(OtherActor))
	{
		Destroy();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[MonsterProjectile] Hit -> %s  Damage=%.2f"), *GetNameSafe(OtherActor), Damage);

	AController* InstigatorController = nullptr;
	if (APawn* InstPawn = Cast<APawn>(GetOwner()))
	{
		InstigatorController = InstPawn->GetController();
	}

	if (OtherActor->CanBeDamaged())
	{
		UGameplayStatics::ApplyDamage(
			OtherActor,
			Damage,
			InstigatorController,
			GetOwner(),
			UDamageType::StaticClass()
		);
	}

	Destroy();
}
