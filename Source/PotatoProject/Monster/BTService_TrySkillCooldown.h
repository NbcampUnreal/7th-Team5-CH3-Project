#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_TrySkillCooldown.generated.h"

UCLASS()
class POTATOPROJECT_API UBTService_TrySkillCooldown : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_TrySkillCooldown();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector CurrentTargetKey;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector bIsCastingSpecialKey;

	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDebugLog = false;
};