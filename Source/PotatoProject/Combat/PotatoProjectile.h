#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectile.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	float Damage;
	float Speed;
	FVector Velocity;
	bool IsExplosive;
	float ExplosionRadius;

	virtual void Tick(float DeltaTime) override;

	void Launch(FVector Direction);
	void OnHit(AActor * HitActor);
	void Explode();
};
