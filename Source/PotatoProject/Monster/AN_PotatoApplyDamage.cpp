#include "AN_PotatoApplyDamage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PotatoCombatComponent.h"
#include "GameFramework/Actor.h"


void UAN_PotatoApplyDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UPotatoCombatComponent* Combat = Owner->FindComponentByClass<UPotatoCombatComponent>();
	if (!Combat) return;

	// 중복 예약 방지(이번 프레임에 이미 예약했으면 스킵)
	if (Combat->bQueuedApplyDamageNextTick) return;
	Combat->bQueuedApplyDamageNextTick = true;

	TWeakObjectPtr<UPotatoCombatComponent> WeakCombat(Combat);
	TWeakObjectPtr<AActor> WeakOwner(Owner);

	World->GetTimerManager().SetTimerForNextTick([WeakCombat, WeakOwner]()
		{
			if (!WeakCombat.IsValid()) return;
			if (!WeakOwner.IsValid()) return;
			if (!WeakOwner->GetWorld()) return;

			WeakCombat->ApplyPendingBasicDamage();
		});
}