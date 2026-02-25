// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_TrySkillCooldown.generated.h"

UCLASS()
class POTATOPROJECT_API UBTService_TrySkillCooldown : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_TrySkillCooldown();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
public:
	// Target BBKey(네 프로젝트 키 이름에 맞춰서)
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector CurrentTargetKey;

	// 선택: 성공 시 세팅할 플래그
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector bIsCastingSpecialKey;
	
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDebugLog = true;
};
