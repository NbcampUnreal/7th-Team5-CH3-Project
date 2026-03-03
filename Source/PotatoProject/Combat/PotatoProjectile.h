#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PotatoProjectile.generated.h"

class USphereComponent;
class UPotatoWeaponData;
class UNiagaraComponent;

UCLASS()
class POTATOPROJECT_API APotatoProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoProjectile();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComp;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> TrailComponent;
	
	float Damage = 0.0f;
	int32 PierceCount = 0;
	float ExplosionRadius = 0.0f;
	
	/** WeaponComponent에 의해 호출됨 */
	void InitializeProjectile(UPotatoWeaponData* WeaponData);

    // 탄도학 계산을 사용하여 초기 속도를 설정하는 버전
    void InitializeProjectileWithBallistics(UPotatoWeaponData* WeaponData, const FVector& InitialVelocity);

protected:
	// 충돌 델리게이트
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);
	
	void Explode(const FVector& ImpactLocation);
	
	void PlayImpactEffects(const FVector& ImpactLocation);
	
private:
	UPROPERTY()
	TObjectPtr<UPotatoWeaponData> CachedWeaponData;
};
