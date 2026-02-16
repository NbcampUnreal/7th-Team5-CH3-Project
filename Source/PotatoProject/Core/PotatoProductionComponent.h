// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoProductionComponent.generated.h"

class UPotatoDayNightCycle;
class UPotatoResourceManager;

/**
 * 자원 생산/비용/환급 담당 범용 컴포넌트
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UPotatoProductionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPotatoProductionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Buy Cost 
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 BuyCostWood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 BuyCostStone = 0;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 BuyCostCrop = 0;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 BuyCostLivestock = 0;

	// Production
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Production", meta=(ClampMin="0"))
	int32 ProductionPerMinuteWood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Production", meta=(ClampMin="0"))
	int32 ProductionPerMinuteStone = 0;	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Production", meta=(ClampMin="0"))
	int32 ProductionPerMinuteCrop = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Production", meta=(ClampMin="0"))
	int32 ProductionPerMinuteLivestock = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Production")
	bool bProduceOnlyAtDay = true;

	// Refund
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 RefundWood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 RefundStone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 RefundCrop = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animal|Economy", meta=(ClampMin="0"))
	int32 RefundLivestock = 0;

public:
	// API
	// 구매 비용 ResourceManager에서 차감 시도, 성공 시 true
    UFUNCTION(BlueprintCallable, Category = "Production")
	bool TryPurchase();

    UFUNCTION(BlueprintCallable, Category = "Production")
    bool TryPurchaseWithWorld(UPotatoResourceManager* OuterResourceManager);

	// 환급 비용만큼 ResourceManager에 자원 추가
	UFUNCTION(BlueprintCallable, Category = "Production")
	void Refund();

    UFUNCTION(BlueprintCallable, Category = "Production")
    void RefundWithWorld(UPotatoResourceManager* OuterResourceManager);

protected:
	UPROPERTY()
	UPotatoResourceManager* ResourceManager;
};
