// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PotatoNPCRegistrySubsystem.generated.h"

class APotatoRewardGenerator;
class UPotatoNPCManagementComp;

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UPotatoNPCRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
    // UWorldSubsystem이 소멸되기 직전 호출 — 참조를 명시적으로 비움
    virtual void Deinitialize() override;

    // MamagementComp가 BeginPlay에서 자신을 등록
    void RegisterNPCComp(UPotatoNPCManagementComp* NPCManagementComp);
    void UnregisterNPCComp(UPotatoNPCManagementComp* NPCManagementComp);

    // Generator가 결산 시점에 참조하는 목록
    const TArray<TObjectPtr<UPotatoNPCManagementComp>>& GetRegisteredComps() const
    {
        return RegisteredComps;
    }

private:

    // NPC 관리 객체(건물) 목록
    UPROPERTY()
    TArray<TObjectPtr<UPotatoNPCManagementComp>> RegisteredComps;
};
