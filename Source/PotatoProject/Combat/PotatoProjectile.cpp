#include "PotatoProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "../Monster/PotatoMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"

APotatoProjectile::APotatoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APotatoProjectile::BeginPlay()
{
	Super::BeginPlay();
	Mesh = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (HitEnable) {
		Mesh->OnComponentHit.AddDynamic(this, &APotatoProjectile::OnHit);
	}
	if (SphereComponent == nullptr)
	{
		SphereComponent = Cast<USphereComponent>(GetComponentByClass(USphereComponent::StaticClass()));
	}
}

void APotatoProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//FVector Location = GetActorLocation();

	////float Gravity = -980.f; // UE 기본 중력 (cm/s^2)
	//Velocity.Z += -980.f * Gravity * DeltaTime;

	//Location += Velocity * DeltaTime;

	//SetActorLocation(Location);
	if (IsExplosive) {
		Explode(DeltaTime);
	}
}

void APotatoProjectile::Launch(FVector Direction)
{

	if (Mesh)
	{
		//FVector LaunchDirection = GetActorLocation() - GetActorLocation().RightVector;
		Direction.Normalize();
		Mesh->AddImpulse(Direction * Speed , NAME_None, true);
		//Mesh->SetLinearDamping(10.0f* Gravity);

	}
}

void APotatoProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsMonsterHit) return;
	if (OtherActor && (OtherActor != this)) {
		APotatoMonster* Monster = Cast<APotatoMonster>(OtherActor);
		if (Monster)
		{
			//UE_LOG(LogTemp, Warning, TEXT("몬스터 맞음! %f"), Damage);
			UGameplayStatics::ApplyDamage(
				Monster,
				Damage,        
				GetInstigatorController(),
				this,                 
				UDamageType::StaticClass()
			);
		}
		IsExplosive = true;
		IsMonsterHit = true;
	}
}

//void APotatoProjectile::Overlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
//	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
//	bool bFromSweep, const FHitResult& SweepResult)
//{
//	if (OtherActor && (OtherActor != this))
//	{
//		FString f = OtherActor->GetFName().ToString();
//		UE_LOG(LogTemp, Log, TEXT("관통함!%s"), *f);
//		//// Monster인지 체크 (Cast 사용)
//		//AMonster* Monster = Cast<AMonster>(OtherActor);
//		//if (Monster)
//		//{
//		//	// 관통하면서 실행될 로직
//		//	
//		//}
//	}
//}

void APotatoProjectile::Explode(float DeltaTime)
{
	/*if (SphereComponent)
	{
		float CurrentRadius = SphereComponent->GetUnscaledSphereRadius();
		UE_LOG(LogTemp, Warning, TEXT("Radius: %f"), CurrentRadius);
		float NewRadius = CurrentRadius + 5.f*DeltaTime;
		SphereComponent->SetSphereRadius(NewRadius);
		if (NewRadius > 15.f)
		{
			IsExplosive = false;
		}
	}*/
}