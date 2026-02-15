#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectile.generated.h"

class USphereComponent;

UCLASS()
class POTATOPROJECT_API APotatoProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FVector Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool IsExplosive;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ExplosionRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Gravity;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool HitEnable;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	//bool IsOverlap;
	//폭발시 범위 판정용 구체
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	USphereComponent* SphereComponent;
	//처음 맞았을 때 맞는 판정. 도탄은 피격 판정 없음.
	bool IsMonsterHit;

	virtual void Tick(float DeltaTime) override;

	void Launch(FVector Direction);
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	//void Overlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,bool bFromSweep, const FHitResult& SweepResult);
	void Explode(float DeltaTime);
};
