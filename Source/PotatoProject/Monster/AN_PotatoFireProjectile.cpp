// Fill out your copyright notice in the Description page of Project Settings.


#include "AN_PotatoFireProjectile.h"
#include "PotatoCombatComponent.h"
#include "GameFramework/Actor.h"

void UAN_PotatoFireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UPotatoCombatComponent* Combat = Owner->FindComponentByClass<UPotatoCombatComponent>();
	if (!Combat) return;

	Combat->FirePendingRangedProjectile();
}
