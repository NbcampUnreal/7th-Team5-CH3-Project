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
	if (!AIC || !BB)
	{
		return EBTNodeResult::Failed;
	}

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M || M->bIsDead || !M->CombatComp)
	{
		return EBTNodeResult::Failed;
	}

	//  공격중이면 실패가 아니라 "이미 공격 진행중"으로 보고 성공 처리
	// (CombatBranch가 우선순위라면 특히 안정적)
	if (M->CombatComp->IsAttacking())
	{
		return EBTNodeResult::Succeeded;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(AttackTargetKey.SelectedKeyName));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	const bool bOk = M->CombatComp->RequestBasicAttack(Target);

	// 선택: 디버그 로그(원하면 주석 해제)
	/*
	UE_LOG(LogTemp, Verbose, TEXT("[BTTask_Attack] RequestBasicAttack -> %s | Owner=%s Target=%s"),
		bOk ? TEXT("OK") : TEXT("FAIL"),
		*GetNameSafe(M),
		*GetNameSafe(Target));
	*/

	return bOk ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
