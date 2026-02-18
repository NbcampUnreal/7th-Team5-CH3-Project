#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectileDamageable.h"
#include "PotatoMonsterProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class POTATOPROJECT_API APotatoMonsterProjectile : public AActor, public IPotatoProjectileDamageable
{
	GENERATED_BODY()

public:
	APotatoMonsterProjectile();

	// 인터페이스 구현
	virtual void SetProjectileDamage_Implementation(float InDamage) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> Movement = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float Damage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float LifeSeconds = 5.f;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// 팀/아군 무시(최소)
	bool ShouldIgnore(AActor* OtherActor) const;
};
