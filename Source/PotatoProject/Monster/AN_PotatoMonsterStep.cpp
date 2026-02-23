// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/AN_PotatoMonsterStep.h"

#include "PotatoMonster.h"

void UAN_PotatoMonsterStep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	if (APotatoMonster* M = Cast<APotatoMonster>(Owner))
	{
		/*M->PlayFootstepSFX();*/
	}
}