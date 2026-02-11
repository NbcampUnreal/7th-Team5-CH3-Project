#include "BT_AdvanceLaneTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PotatoMonster.h"

UBT_AdvanceLaneTarget::UBT_AdvanceLaneTarget()
{
	NodeName = TEXT("Advance Lane Target");

	// 실수 방지: Actor만 선택 가능하게
	MoveTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBT_AdvanceLaneTarget, MoveTargetKey), AActor::StaticClass());
	WarehouseActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBT_AdvanceLaneTarget, WarehouseActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBT_AdvanceLaneTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    if (!AICon) return EBTNodeResult::Failed;

    APotatoMonster* Monster = Cast<APotatoMonster>(AICon->GetPawn());
    if (!Monster) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    if (MoveTargetKey.SelectedKeyName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[AdvanceLaneTarget] MoveTargetKey is NONE. Set it in BT Task Details."));
        return EBTNodeResult::Failed;
    }

    const FName WarehouseKeyName = WarehouseActorKey.SelectedKeyName;

    // Warehouse 참조 확보 (우선 Monster 주입값, 없으면 BB에서)
    AActor* Warehouse = Monster->WarehouseActor;
    if (!Warehouse && !WarehouseKeyName.IsNone())
    {
        Warehouse = Cast<AActor>(BB->GetValueAsObject(WarehouseKeyName));
    }

    if (!IsValid(Warehouse))
    {
        UE_LOG(LogTemp, Error, TEXT("[AdvanceLaneTarget] WarehouseActor is NULL/Invalid."));
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Warning, TEXT("[AdvanceLaneTarget] Before: LaneIndex=%d Points=%d PawnLoc=%s"),
        Monster->LaneIndex, Monster->LanePoints.Num(), *Monster->GetActorLocation().ToString());

    // ✅ 끝 이후 무한 증가 방지 (선택이지만 추천)
    if (Monster->LaneIndex >= Monster->LanePoints.Num())
    {
        // 이미 Lane이 끝났다면 그냥 Warehouse로 유지
        BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, Warehouse);
        UE_LOG(LogTemp, Warning, TEXT("[AdvanceLaneTarget] Lane already finished -> MoveTarget=Warehouse (%s)"),
            *Warehouse->GetPathName());
        return EBTNodeResult::Succeeded;
    }

    // LaneIndex 전진
    Monster->AdvanceLaneIndex();

    AActor* NextTarget = Monster->GetCurrentLaneTarget();

    UE_LOG(LogTemp, Warning, TEXT("[AdvanceLaneTarget] After: LaneIndex=%d Next=%s"),
        Monster->LaneIndex,
        NextTarget ? *NextTarget->GetPathName() : TEXT("NULL"));

    // NextTarget이 유효하고 Warehouse가 아니면 다음 LanePoint로
    if (IsValid(NextTarget) && NextTarget != Warehouse)
    {
        BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, NextTarget);

        AActor* After = Cast<AActor>(BB->GetValueAsObject(MoveTargetKey.SelectedKeyName));
        UE_LOG(LogTemp, Warning, TEXT("[AdvanceLaneTarget] Set MoveTarget -> LanePoint=%s | AfterRead=%s"),
            *NextTarget->GetPathName(),
            After ? *After->GetPathName() : TEXT("NULL"));

        return EBTNodeResult::Succeeded;
    }

    // ✅ Lane 끝(또는 NextTarget이 Warehouse로 판정) -> Abort하지 말고 "이어가기"
    BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, Warehouse);

    AActor* After = Cast<AActor>(BB->GetValueAsObject(MoveTargetKey.SelectedKeyName));
    UE_LOG(LogTemp, Warning, TEXT("[AdvanceLaneTarget] Lane finished -> MoveTarget=Warehouse=%s | AfterRead=%s"),
        *Warehouse->GetPathName(),
        After ? *After->GetPathName() : TEXT("NULL"));

    return EBTNodeResult::Succeeded;
}