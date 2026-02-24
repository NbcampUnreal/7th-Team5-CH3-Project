#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_UpdateAttackTarget.generated.h"

class APotatoMonster;

USTRUCT()
struct FUpdateAttackTargetMemory
{
	GENERATED_BODY()

	float LastDistToMoveTarget2D = -1.f;
	float NoProgressTime = 0.f;
	bool  bBlocked = false;
	TWeakObjectPtr<AActor> LastTarget;
};

UCLASS()
class POTATOPROJECT_API UBTService_UpdateAttackTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_UpdateAttackTarget();

	virtual uint16 GetInstanceMemorySize() const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// ===== Blackboard Keys =====
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AttackTargetKey;     // CurrentTarget (Object)

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector InAttackRangeKey;    // bInAttackRange (Bool)

	// ✅ WarehouseActor Key (Object) - 없으면 방향 기준이 흔들림
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Move")
	FName WarehouseKeyName = TEXT("WarehouseActor");

	// (선택) 디버그/표시용 Actor Goal (BT MoveTo로 쓰지 말 것)
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Move")
	FName MoveGoalKeyName = TEXT("MoveGoal");

	// ✅ 정석: MoveTo는 Vector Goal만 사용
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Move")
	FName MoveGoalLocationKeyName = TEXT("MoveGoalLocation");

	// ===== InRange 계산 보정 =====
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Range")
	float BoundsRangePadding = 0.f;  // ✅ 우선 0 권장

	// ===== 막힘 감지 튜닝(옵션) =====
	UPROPERTY(EditAnywhere, Category = "Blocked")
	float NoProgressMinDelta = 15.f;

	UPROPERTY(EditAnywhere, Category = "Blocked")
	float NoProgressHoldTime = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Blocked")
	float BlockedSearchExtra = 300.f;

	// ===== 차단 구조물 필터 튜닝 =====
	UPROPERTY(EditAnywhere, Category = "Blocked")
	float BlockerWidth = 280.f;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float KeepTargetExtraWidth = 120.f;

	// ===== 코리더 기반 탐색 =====
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Corridor")
	float CorridorSearchExtraRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "AttackTarget|Corridor")
	float MinCorridorSearchRange = 350.f;

	// ===== 접근점 오프셋 =====
	UPROPERTY(EditAnywhere, Category = "AttackTarget|Move")
	float ApproachExtraOffset = 20.f;

	// Player 관련 BB 키
	UPROPERTY(EditAnywhere, Category = "Player")
	FBlackboardKeySelector PlayerActorKey;

	// 플레이어 무시 거리 (몬스터가 Warehouse 근처일 때)
	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = "0"))
	float IgnorePlayerWhenNearWarehouseDist = 900.f;

	// 플레이어 사거리 여유값
	UPROPERTY(EditAnywhere, Category = "Player", meta = (ClampMin = "0"))
	float PlayerSenseExtraRange = 0.f;

protected:
	bool ComputeInAttackRange(APotatoMonster* M, AActor* Target, float Range) const;

	AActor* FindBestBlockerOnCorridor(APotatoMonster* M, float SearchRange, const FVector& MoveTargetLoc) const;

	bool ShouldKeepCurrentStructureTarget(APotatoMonster* M, AActor* CurrentTarget, const FVector& MoveTargetLoc) const;

	void UpdateBlockedState(
		FUpdateAttackTargetMemory& Mem,
		APotatoMonster* M,
		const FVector& MoveTargetLoc,
		float DeltaSeconds) const;
};
