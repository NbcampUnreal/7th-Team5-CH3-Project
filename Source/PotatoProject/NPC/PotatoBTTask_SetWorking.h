// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PotatoBTTask_SetWorking.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UPotatoBTTask_SetWorking : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UPotatoBTTask_SetWorking();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bWorking = true;
};
