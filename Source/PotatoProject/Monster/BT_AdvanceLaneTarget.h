// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BT_AdvanceLaneTarget.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UBT_AdvanceLaneTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBT_AdvanceLaneTarget();

protected:
	/** Blackboard key: MoveTarget (Actor) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector MoveTargetKey;

	/** Blackboard key: WarehouseActor (Actor) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector WarehouseActorKey;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
