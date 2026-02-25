// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/BTService_TrySkillCooldown.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PotatoMonster.h"
#include "Monster/SpecialSkillComponent.h"

UBTService_TrySkillCooldown::UBTService_TrySkillCooldown()
{
	NodeName = TEXT("Try Special Skill (OnCooldown)");
	Interval = 0.3f;
	RandomDeviation = 0.1f;
}

void UBTService_TrySkillCooldown::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return;

	APotatoMonster* Monster = Cast<APotatoMonster>(AICon->GetPawn());
	if (!IsValid(Monster)) return;

	USpecialSkillComponent* SkillComp = Monster->FindComponentByClass<USpecialSkillComponent>();
	if (!IsValid(SkillComp)) return;

	// 1) Busy 동기화 (항상)
	if (bIsCastingSpecialKey.SelectedKeyType)
	{
		BB->SetValueAsBool(bIsCastingSpecialKey.SelectedKeyName, SkillComp->IsBusy());
	}

	// 2) OnCooldown SkillId
	const FName SkillId = Monster->SpecialSkill_OnCooldown;
	if (SkillId.IsNone()) return;

	// 3) "쿨다운(Ready) 조건일 때만" TryStart 시도
	if (!SkillComp->CanTryStartSkill(SkillId))
	{
		return;
	}

	// 4) CurrentTarget
	AActor* Target = nullptr;
	if (CurrentTargetKey.SelectedKeyType)
	{
		Target = Cast<AActor>(BB->GetValueAsObject(CurrentTargetKey.SelectedKeyName));
	}

	const bool bStarted = SkillComp->TryStartSkill(SkillId, Target);

	if (bDebugLog && bStarted)
	{
		UE_LOG(LogTemp, Log, TEXT("[BTService][OnCooldown] Started Skill=%s Target=%s Monster=%s"),
			*SkillId.ToString(),
			*GetNameSafe(Target),
			*GetNameSafe(Monster));
	}
}