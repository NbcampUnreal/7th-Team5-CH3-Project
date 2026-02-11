#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PotatoAIController.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoAIController : public AAIController
{
	GENERATED_BODY()

public:
	APotatoAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class UBlackboardComponent> BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class UBehaviorTreeComponent> BehaviorComp;

	// Blackboard Keys
	static const FName Key_WarehouseActor;
	static const FName Key_CurrentTarget;
	static const FName Key_bIsDead;
	static const FName Key_SpecialLogic; // int로 저장
	static const FName Key_MoveTarget;
};
