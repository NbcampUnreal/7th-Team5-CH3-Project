// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PotatoBTTask_Attack.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UPotatoBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UPotatoBTTask_Attack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AttackTargetKey;

	
};
