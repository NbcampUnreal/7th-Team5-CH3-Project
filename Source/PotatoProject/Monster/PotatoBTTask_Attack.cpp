#include "PotatoBTTask_Attack.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PotatoMonster.h"
#include "PotatoCombatComponent.h"

UPotatoBTTask_Attack::UPotatoBTTask_Attack()
{
	NodeName = TEXT("Basic Attack");
	AttackTargetKey.SelectedKeyName = TEXT("CurrentTarget");
}

EBTNodeResult::Type UPotatoBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB) return EBTNodeResult::Failed;

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M || M->bIsDead || !M->CombatComp) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(AttackTargetKey.SelectedKeyName));
	if (!Target) return EBTNodeResult::Failed;

	const bool bOk = M->CombatComp->RequestBasicAttack(Target);
	return bOk ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
