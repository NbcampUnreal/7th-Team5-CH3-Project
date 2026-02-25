#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Character.h"
#include "Core/PotatoProductionComponent.h"
#include "PotatoNPC.generated.h"

class APotatoBuilding;
class UBoxComponent;

UCLASS()
class POTATOPROJECT_API APotatoNPC : public ACharacter
{
	GENERATED_BODY()
	
public:	
	APotatoNPC();

protected:
	virtual void BeginPlay() override;


public:	
	// Working State (Idle/Working)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	bool bIsWorking = false;
	
	// Type
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")	
	ENPCType Type;

	// 유지비용 축산물 (매일 밤 차감)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	int32 MaintenanceCostLivestock;

	// 생산/경제 컴포넌트 (비용, 분당 생산량, 환급 모두 여기서 관리)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime")
	TObjectPtr<UPotatoProductionComponent> ProductionComp;
	
	// 할당된 건물 (작업지시, 퇴직 시 참조)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Runtime")
	TObjectPtr<AActor> AssignedBuilding = nullptr;

	// 배회 영역 (건물 BP의 BoxComponent)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Runtime")
	TObjectPtr<UBoxComponent> MovingArea = nullptr;

	// 작업지시: 건물 참조 저장 + 생산 시작
	UFUNCTION(BlueprintCallable, Category = "NPC|Runtime")
	void InitializeWithBuilding(AActor* InBuilding, UBoxComponent* InMovingArea);

	// 유지비용 지불 시도, 성공 시 true 반환
	UFUNCTION(BlueprintCallable, Category = "NPC|Economy")
	bool TryPayMaintenance();

	// 퇴직: 건물 제거 + Destroy
    UFUNCTION(BlueprintCallable, Category = "NPC|Runtime")
	void Retire();

// Getter
public:
    UFUNCTION(BlueprintPure, Category = "NPC|Economy")
    int32 GetMaintenanceCost() const { return MaintenanceCostLivestock; }
};
