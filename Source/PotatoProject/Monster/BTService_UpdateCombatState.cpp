// BTS_UpdateCombatState.cpp (최소 정석)
#include "BTService_UpdateCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PotatoMonster.h"
#include "PotatoCombatComponent.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = TEXT("Update Combat State");
	Interval = 0.15f;
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M) return;

	BB->SetValueAsBool(TEXT("bIsDead"), M->bIsDead);

	// bIsAttacking은 CombatComponent가 “진짜 공격 중”일 때만 true로 유지하게(권장)
	if (UPotatoCombatComponent* CC = M->FindComponentByClass<UPotatoCombatComponent>())
	{
		BB->SetValueAsBool(TEXT("bIsAttacking"), CC->IsAttacking()); // 너 함수명에 맞춰 수정
	}
}
