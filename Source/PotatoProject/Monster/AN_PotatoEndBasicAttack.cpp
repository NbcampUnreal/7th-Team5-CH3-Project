#include "AN_PotatoEndBasicAttack.h"

#include "PotatoCombatComponent.h"
#include "GameFramework/Actor.h"

void UAN_PotatoEndBasicAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	if (UPotatoCombatComponent* Combat = Owner->FindComponentByClass<UPotatoCombatComponent>())
	{
		Combat->EndBasicAttack();
	}
}
