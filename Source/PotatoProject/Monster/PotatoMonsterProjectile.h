#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectileDamageable.h"
#include "PotatoMonsterProjectile.generated.h"

class UNiagaraComponent;

UCLASS()
class POTATOPROJECT_API APotatoMonsterProjectile
	: public AActor
	, public IPotatoProjectileDamageable
{
	GENERATED_BODY()

public:
	APotatoMonsterProjectile();

	// 데미지 주입 (CombatComponent에서 호출)
	virtual void SetProjectileDamage_Implementation(float InDamage) override;

	// 스폰 직후 방향/속도 세팅
	void InitVelocity(const FVector& InDirection, float InSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool ShouldIgnore(AActor* Other) const;
	void DoSweep(const FVector& From, const FVector& To);

private:

	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* VFX;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float Speed = 1800.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float TraceRadius = 12.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	float LifeSeconds = 5.f;

	UPROPERTY(EditAnywhere, Category="Projectile")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldDynamic;

	UPROPERTY(EditAnywhere, Category="Projectile")
	bool bIgnoreAllMonsters = true;

	float Damage = 0.f;
	FVector MoveDir = FVector::ForwardVector;
	bool bHitOnce = false;
};