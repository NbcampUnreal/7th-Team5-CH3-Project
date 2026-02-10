#include "PotatoAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"

#include "PotatoMonster.h"

const FName APotatoAIController::Key_WarehouseActor(TEXT("WarehouseActor"));
const FName APotatoAIController::Key_CurrentTarget(TEXT("CurrentTarget"));
const FName APotatoAIController::Key_bIsDead(TEXT("bIsDead"));
const FName APotatoAIController::Key_SpecialLogic(TEXT("SpecialLogic"));

APotatoAIController::APotatoAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
	BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

void APotatoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	APotatoMonster* Monster = Cast<APotatoMonster>(InPawn);
	if (!Monster) return;

	Monster->ApplyPresets();

	UBehaviorTree* BT = Monster->GetBehaviorTreeToRun();
	if (!BT || !BT->BlackboardAsset) return;

	BlackboardComp->InitializeBlackboard(*BT->BlackboardAsset);

	// =========================================================
	// Spawner가 Monster->WarehouseActor를 "사전에 주입"해주는 방식
	// => AIController는 더 이상 검색/등록 로직이 필요 없음
	// =========================================================
	if (Monster->WarehouseActor)
	{
		BlackboardComp->SetValueAsObject(Key_WarehouseActor, Monster->WarehouseActor);
	}
	else
	{
		// 테스트 단계 default: 비워둠 (임의 대상 지정 금지)
		BlackboardComp->ClearValue(Key_WarehouseActor);
	}

	// 상태값 초기 동기화
	BlackboardComp->SetValueAsBool(Key_bIsDead, Monster->bIsDead);
	BlackboardComp->SetValueAsInt(Key_SpecialLogic, static_cast<int32>(Monster->AppliedSpecialLogic));
	BlackboardComp->ClearValue(Key_CurrentTarget);

	RunBehaviorTree(BT);
}