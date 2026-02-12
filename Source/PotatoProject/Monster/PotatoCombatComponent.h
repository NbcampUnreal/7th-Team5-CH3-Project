#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoMonsterFinalStats.h"
#include "PotatoCombatComponent.generated.h"

UCLASS(ClassGroup = (Potato), meta = (BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoCombatComponent();

	void InitFromStats(const FPotatoMonsterFinalStats& InStats);

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	bool RequestBasicAttack(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	void ApplyPendingBasicDamage();

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	void EndBasicAttack();

	UFUNCTION(BlueprintCallable, Category = "Potato|Combat")
	bool IsAttacking() const { return bIsAttacking; }

private:
	FPotatoMonsterFinalStats Stats;

	bool bIsAttacking = false;
	double NextAttackTime = 0.0;
	TWeakObjectPtr<AActor> PendingBasicTarget;

	bool IsTargetInRange(AActor* Target) const;
	bool IsStructureTarget(AActor* Target) const;
	void ApplyBasicDamage(AActor* Target) const;
};
