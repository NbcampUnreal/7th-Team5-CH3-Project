// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PotatoBTTask_FaceToBuilding.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UPotatoBTTask_FaceToBuilding : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UPotatoBTTask_FaceToBuilding();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// 건물 BB 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector BuildingKey;

	// 회전 속도
    UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "10.0"))
	float RotationSpeed = 180.0f;

	// 완료 판정 허용 각도
    UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "1.0"))
	float AcceptanceAngle = 5.0f;	
};
