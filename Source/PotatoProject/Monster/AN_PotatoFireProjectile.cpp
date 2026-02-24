// Fill out your copyright notice in the Description page of Project Settings.


#include "AN_PotatoFireProjectile.h"
#include "PotatoCombatComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UAN_PotatoFireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UPotatoCombatComponent* Combat = Owner->FindComponentByClass<UPotatoCombatComponent>();
	if (!Combat) return;

	if (Combat->bQueuedFireProjectileNextTick) return;
	Combat->bQueuedFireProjectileNextTick = true;

	TWeakObjectPtr<UPotatoCombatComponent> WeakCombat(Combat);
	TWeakObjectPtr<AActor> WeakOwner(Owner);

	World->GetTimerManager().SetTimerForNextTick([WeakCombat, WeakOwner]()
		{
			if (!WeakCombat.IsValid()) return;
			if (!WeakOwner.IsValid()) return;
			if (!WeakOwner->GetWorld()) return;

			WeakCombat->FirePendingRangedProjectile();
		});
}