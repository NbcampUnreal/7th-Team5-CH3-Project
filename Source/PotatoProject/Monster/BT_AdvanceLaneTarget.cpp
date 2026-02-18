#include "BT_AdvanceLaneTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "PotatoMonster.h"

UBT_AdvanceLaneTarget::UBT_AdvanceLaneTarget()
{
	NodeName = TEXT("Advance Lane Target");

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
		return EBTNodeResult::Failed;
	}

	// ---------------------------------------------------------
	// ✅ (선택) 전투/접근 중이면 레인 전진 금지 (MoveGoal != MoveTarget)
	// ---------------------------------------------------------
	{
		static const FName MoveGoalKeyName = TEXT("MoveGoal");
		AActor* MoveGoal = Cast<AActor>(BB->GetValueAsObject(MoveGoalKeyName));
		AActor* MoveTarget = Cast<AActor>(BB->GetValueAsObject(MoveTargetKey.SelectedKeyName));

		if (IsValid(MoveGoal) && IsValid(MoveTarget) && MoveGoal != MoveTarget)
		{
			return EBTNodeResult::Succeeded; // 아무것도 안 하고 통과
		}
	}

	const int32 NumPoints = Monster->LanePoints.Num();
	if (NumPoints <= 0)
	{
		return EBTNodeResult::Failed;
	}

	// (0) 이동 중이면 실행 차단
	// ❗️중요: 여기서 Failed를 내면 시퀀스/셀렉터가 흔들려 MoveTo가 불안정해질 수 있음
	// -> 조용히 통과(Succeeded)
	if (UPathFollowingComponent* PFC = AICon->GetPathFollowingComponent())
	{
		const EPathFollowingStatus::Type Status = PFC->GetStatus();
		if (Status == EPathFollowingStatus::Moving || Status == EPathFollowingStatus::Paused)
		{
			return EBTNodeResult::Succeeded;
		}
	}

	// (1) LaneIndex 유효성 보정
	if (!Monster->LanePoints.IsValidIndex(Monster->LaneIndex))
	{
		AActor* LastPoint = Monster->LanePoints.Last().Get();
		if (IsValid(LastPoint))
		{
			BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, LastPoint);
		}
		return EBTNodeResult::Succeeded;
	}

	AActor* ExpectedTarget = Monster->LanePoints[Monster->LaneIndex].Get();
	if (!IsValid(ExpectedTarget))
	{
		return EBTNodeResult::Failed;
	}

	AActor* CurMoveTargetBB = Cast<AActor>(BB->GetValueAsObject(MoveTargetKey.SelectedKeyName));

	const float Dist2D = FVector::Dist2D(
		Monster->GetActorLocation(),
		ExpectedTarget->GetActorLocation());

	// ✅ 도착 판정
	// MoveTo AcceptanceRadius와 비슷한 값으로 맞추는 게 가장 안정적
	const float ArrivedThreshold = 60.f; // 기존 50보다 약간 여유

	// 아직 도착 안 했으면: BB 정렬만 하고 통과 (Failed 금지)
	if (Dist2D > ArrivedThreshold)
	{
		// BB MoveTarget이 어긋났다면 복구
		if (CurMoveTargetBB != ExpectedTarget)
		{
			BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, ExpectedTarget);
		}
		return EBTNodeResult::Succeeded;
	}

	// (2) 마지막 포인트면 유지
	if (Monster->LaneIndex >= NumPoints - 1)
	{
		AActor* LastPoint = Monster->LanePoints.Last().Get();
		if (IsValid(LastPoint))
		{
			BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, LastPoint);
		}
		return EBTNodeResult::Succeeded;
	}

	// (3) 정상 도착 → LaneIndex 증가
	Monster->AdvanceLaneIndex();

	AActor* NextTarget = Monster->GetCurrentLaneTarget();
	if (IsValid(NextTarget))
	{
		BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, NextTarget);
		return EBTNodeResult::Succeeded;
	}

	// 예외: NextTarget 없으면 마지막 유지
	AActor* LastPoint = Monster->LanePoints.Last().Get();
	if (IsValid(LastPoint))
	{
		BB->SetValueAsObject(MoveTargetKey.SelectedKeyName, LastPoint);
	}

	return EBTNodeResult::Succeeded;
}
