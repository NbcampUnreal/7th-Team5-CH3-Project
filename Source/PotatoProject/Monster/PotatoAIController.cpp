#include "PotatoAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"

#include "PotatoMonster.h"

const FName APotatoAIController::Key_WarehouseActor(TEXT("WarehouseActor"));
const FName APotatoAIController::Key_CurrentTarget(TEXT("CurrentTarget"));
const FName APotatoAIController::Key_bIsDead(TEXT("bIsDead"));
const FName APotatoAIController::Key_SpecialLogic(TEXT("SpecialLogic"));
const FName APotatoAIController::Key_MoveTarget(TEXT("MoveTarget"));

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

    // =========================================================
    // 0) 스폰 1회 프리셋 적용 (진행사항 반영: ApplyPresetsOnce)
    // =========================================================
    Monster->ApplyPresetsOnce();

    // =========================================================
    // 1) 실행할 BT 결정 (Resolved -> Default fallback)
    // =========================================================
    UBehaviorTree* BT = Monster->GetBehaviorTreeToRun();
    if (!BT || !BT->BlackboardAsset) return;

    // =========================================================
    // 2) Blackboard 초기화 -> BT 실행
    // =========================================================
    BlackboardComp->InitializeBlackboard(*BT->BlackboardAsset);
    RunBehaviorTree(BT);

    // =========================================================
    // 3) WarehouseActor 세팅 (스포너 주입값 사용)
    // =========================================================
    if (IsValid(Monster->WarehouseActor))
    {
        BlackboardComp->SetValueAsObject(Key_WarehouseActor, Monster->WarehouseActor);
    }
    else
    {
        BlackboardComp->ClearValue(Key_WarehouseActor);
    }

    // =========================================================
    // 4) MoveTarget 1회 초기화 (레인 첫 포인트 or Warehouse)
    //    - 이미 값이 있으면 덮어쓰지 않음(리셋 방지)
    // =========================================================
    {
        UObject* Existing = BlackboardComp->GetValueAsObject(Key_MoveTarget);
        if (!IsValid(Cast<AActor>(Existing)))
        {
            AActor* FirstTarget = nullptr;

            // 레인이 있으면 현재 레인 타겟(없으면 Warehouse fallback)
            if (Monster->LanePoints.Num() > 0)
            {
                FirstTarget = Monster->GetCurrentLaneTarget();
            }

            if (!IsValid(FirstTarget))
            {
                FirstTarget = Monster->WarehouseActor;
            }

            if (IsValid(FirstTarget))
            {
                BlackboardComp->SetValueAsObject(Key_MoveTarget, FirstTarget);
            }
            else
            {
                BlackboardComp->ClearValue(Key_MoveTarget);
            }
        }
    }

    // =========================================================
    // 5) 상태 동기화 (진행사항 반영: SpecialLogic은 FinalStats 기준)
    // =========================================================
    BlackboardComp->SetValueAsBool(Key_bIsDead, Monster->bIsDead);

    // SpecialLogic 키를 계속 쓸 거면 FinalStats에서 가져오기
    // (BB 타입이 Int라면 아래처럼 유지)
    BlackboardComp->SetValueAsInt(
        Key_SpecialLogic,
        static_cast<int32>(Monster->FinalStats.SpecialLogic)
    );

    // 전투 타겟은 BT/서비스가 갱신하게 두고, possess 시점에는 비움
    BlackboardComp->ClearValue(Key_CurrentTarget);
}
