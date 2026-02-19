#include "PotatoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "../Monster/PotatoMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "PotatoWeaponData.h"
#include "Engine/OverlapResult.h"

APotatoProjectile::APotatoProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(FName("SphereComponent"));
	CollisionComp->SetupAttachment(RootComponent);
	CollisionComp->InitSphereRadius(15.0f);
	CollisionComp->SetCollisionProfileName(TEXT("Projectile"));

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComp);
	ProjectileMesh->SetCollisionProfileName(TEXT("NoCollision"));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.0f;
	ProjectileMovement->MaxSpeed = 1000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void APotatoProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 충돌 이벤트 바인딩
	CollisionComp->OnComponentHit.AddDynamic(this, &APotatoProjectile::OnHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APotatoProjectile::OnOverlap);

	SetLifeSpan(3.0f);
}

void APotatoProjectile::InitializeProjectile(const UPotatoWeaponData* WeaponData)
{
	if (!WeaponData)
	{
		return;
	}

	Damage = WeaponData->BaseDamage;
	PierceCount = WeaponData->MaxPierceCount;
	ExplosionRadius = WeaponData->ExplosionRadius;

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = WeaponData->ProjectileSpeed;
		ProjectileMovement->MaxSpeed = WeaponData->ProjectileSpeed;
		ProjectileMovement->ProjectileGravityScale = WeaponData->ProjectileGravityScale;
		ProjectileMovement->Velocity = GetActorForwardVector() * WeaponData->ProjectileSpeed;
	}
}

void APotatoProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == this)
	{
		return;
	}

	// 호박: 지면/벽 충돌 시 폭발
	if (ExplosionRadius > 0.0f)
	{
		Explode(Hit.Location);
		return;
	}

	// 감자/옥수수: 지면/벽 충돌 시 소멸
	if (OtherActor->ActorHasTag(TEXT("Monster")))
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	}
	Destroy();
}

void APotatoProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                  const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == this)
	{
		return;
	}

	if (!OtherActor->ActorHasTag(TEXT("Monster")))
	{
		return;
	}

	// 호박: 적 충돌 시에도 폭발
	if (ExplosionRadius > 0.0f)
	{
		Explode(GetActorLocation());
		return;
	}

	// 감자/옥수수: 데미지 적용
	if (OtherActor->ActorHasTag(TEXT("Monster")))
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	}

	// 옥수수: 관통 적용
	if (PierceCount > 0)
	{
		PierceCount--;

		CollisionComp->IgnoreActorWhenMoving(OtherActor, true);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("관통: 남은 횟수 %d"), PierceCount));
	}
	else
	{
		// 감자 또는 관통 횟수를 모두 소진한 옥수수 파괴
		Destroy();
	}
}

void APotatoProjectile::Explode(const FVector& ImpactLocation)
{
	if (IsPendingKillPending())
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ImpactLocation,
		FQuat::Identity,
		ObjectQueryParams,
		CollisionShape
	);

	if (bHit)
	{
		TArray<AActor*> OverlappedActors;

		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* HitActor = OverlapResult.GetActor();

			if (!HitActor || !HitActor->IsValidLowLevel() || OverlappedActors.Contains(HitActor))
			{
				continue;
			}

			if (HitActor == this || HitActor == GetOwner())
			{
				continue;
			}

			if (HitActor->ActorHasTag(TEXT("Monster")))
			{
				UGameplayStatics::ApplyDamage(HitActor, Damage, GetInstigatorController(), this,
				                              UDamageType::StaticClass());
				OverlappedActors.Add(HitActor);
			}
		}
	}

	DrawDebugSphere(GetWorld(), ImpactLocation, ExplosionRadius, 12, FColor::Orange, false, 2.0f);

	// TODO: 파티클 및 사운드 재생

	Destroy();
}
