// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/PotatoEnums.h"
#include "PotatoNPCManagementComp.generated.h"

class UBoxComponent;
class APotatoNPC;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UPotatoNPCManagementComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoNPCManagementComp();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UPROPERTY(EditAnywhere, Category = "Config")
    int32 MaxNPCCount = 3;

    /** Enum -> BP 클래스 매핑 (에디터에서 설정) */
    UPROPERTY(EditAnywhere, Category = "Config")
    TMap<ENPCType, TSubclassOf<APotatoNPC>> NPCClassMap;

    UPROPERTY(VisibleInstanceOnly, Category = "Runtime")
    TArray<TObjectPtr<APotatoNPC>> AssignedNPCs;

private:
    /** BeginPlay에서 Owner의 BoxComponent를 자동 캐싱 */
    UPROPERTY()
    TObjectPtr<UBoxComponent> CachedBoxComp;

public:
    UFUNCTION(BlueprintCallable)
    bool HireNPC(ENPCType NPCType);

    // 해고는 기획 볼륨상 삭제되었음 - 레거시 코드
    UFUNCTION(BlueprintCallable)
    void FireNPC(APotatoNPC* NPC);

    // RewardGenerator에서 유지비용 지불 시도할 때 참조하는 함수
    UFUNCTION()
    int32 ProcessNPCMaintenance(TMap<ENPCType, int32>& OutRetiredByType);
};
