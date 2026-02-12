#include "AN_PotatoApplyDamage.h"

#include "PotatoCombatComponent.h"
#include "GameFramework/Actor.h"

void UAN_PotatoApplyDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	if (UPotatoCombatComponent* Combat = Owner->FindComponentByClass<UPotatoCombatComponent>())
	{
		Combat->ApplyPendingBasicDamage();
	}
}