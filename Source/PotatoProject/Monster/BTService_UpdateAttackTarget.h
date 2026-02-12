#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAttackTarget.generated.h"

class UPrimitiveComponent;

UCLASS()
class POTATOPROJECT_API UBTService_UpdateAttackTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateAttackTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AttackTargetKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector InAttackRangeKey;

	// 피벗 거리로 먼저 컷할 임계값(AttackRange보다 조금 크게)
	UPROPERTY(EditAnywhere, Category = "Range")
	float CollisionCheckPadding = 200.f;

	// Target의 PrimitiveComponent를 캐싱해서 매 틱 탐색 비용 줄이기
	struct FUpdateAttackTargetMemory
	{
		TWeakObjectPtr<AActor> CachedTarget;
		TWeakObjectPtr<UPrimitiveComponent> CachedPrim;
	};
};
