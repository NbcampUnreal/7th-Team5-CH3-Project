#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Core/PotatoTypes.h"
#include "PotatoMonster.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoMonster : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoMonster();

protected:
	virtual void BeginPlay() override;

public:	
	EMonsterType Type;
	float Health;
	float MaxHealth;
	float Speed;
	float AttackDamage;
	float AttackRange;
	AActor* CurrentTarget;

	virtual void Tick(float DeltaTime) override;

	void Move();
	void Attack(AActor* Target);
	void TakeDamage(float Damage);
	void Die();
	AActor* FindTarget();
};
