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

    // 프리셋 적용(기존 유지)
    Monster->ApplyPresets();

    UBehaviorTree* BT = Monster->GetBehaviorTreeToRun();
    if (!BT || !BT->BlackboardAsset) return;

    // 1) Blackboard 초기화
    BlackboardComp->InitializeBlackboard(*BT->BlackboardAsset);

    // 2) BT 실행 (엔진 표준 흐름에 맞추기 위해 초기화 후 바로 실행)
    //    - 아래에서 BB 값 세팅해도 BT는 다음 틱/재평가에서 따라온다
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
    // 4) MoveTarget 초기값 세팅 (정석: 레인 있으면 첫 포인트, 없으면 Warehouse)
    //    - "MoveTarget은 BT만" 원칙으로 가려면,
    //      여기(=AI 주체인 Controller)에서 딱 1회 초기화하고,
    //      이후 갱신은 BTTask_AdvanceLaneTarget이 책임진다.
    //    - OnPossess가 다시 불릴 수도 있으니,
    //      이미 값이 있으면 덮어쓰지 않는 가드 추가(리셋 방지)
    // =========================================================
    {
        UObject* Existing = BlackboardComp->GetValueAsObject(Key_MoveTarget);
        if (!IsValid(Cast<AActor>(Existing)))
        {
            // LaneIndex는 스포너가 0으로 넣는 게 정상.
            // 혹시라도 꼬였을 때를 대비해 0으로만 고정하고 싶으면 주석 해제:
            // Monster->LaneIndex = 0;

            AActor* FirstTarget = nullptr;

            // LanePoints가 있으면 현재 레인 타겟
            if (Monster->LanePoints.Num() > 0)
            {
                FirstTarget = Monster->GetCurrentLaneTarget(); // LanePoints[LaneIndex] or WarehouseActor fallback
            }

            // 레인이 없거나 유효 타겟이 없으면 Warehouse로 바로
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
        // else: 이미 세팅돼 있으면 절대 덮어쓰지 않음 (리셋 방지)
    }

    // =========================================================
    // 5) 상태값 초기 동기화 (기존 유지)
    // =========================================================
    BlackboardComp->SetValueAsBool(Key_bIsDead, Monster->bIsDead);
    BlackboardComp->SetValueAsInt(Key_SpecialLogic, static_cast<int32>(Monster->AppliedSpecialLogic));
    BlackboardComp->ClearValue(Key_CurrentTarget);
}
