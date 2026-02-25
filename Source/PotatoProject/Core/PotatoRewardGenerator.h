// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoRewardGenerator.generated.h"

class UPotatoResourceManager;
class APotatoMonsterSpawner;
class APotatoGameMode;
class UDataTable;
class UResultScreen;
class UPotatoNPCManagementComp;

UCLASS()
class POTATOPROJECT_API APotatoRewardGenerator : public AActor
{
	GENERATED_BODY()

public:
    APotatoRewardGenerator();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    // ResourceManager를 참조 (BeginPlay -> GetWorld()->GetWorldSubsystem()
    UPROPERTY()
    TObjectPtr<UPotatoResourceManager> ResourceManager;

    // 캐싱
    UPROPERTY()
    TObjectPtr<APotatoGameMode> PotatoGameMode;

#pragma region RewardGain

public:
    // 보상 데이터 테이블
    // Day 번호(RowName="1","2"...)를 키로 보상량을 조회
    UPROPERTY(EditDefaultsOnly, Category = "Reward|Data")
    TObjectPtr<UDataTable> RewardTable;

    UPROPERTY(EditDefaultsOnly, Category = "Reward|Data")
    TSubclassOf<UResultScreen> ResultScreenClass;

    UPROPERTY()
    TObjectPtr<UResultScreen> ResultScreen;

private:
    // Widget에 접근하여 결과 화면을 띄우는 함수, 수여된 보상 정보와 소모된 유지비를 표시함
    void ShowResultScreen(int32 InCurrentDay, int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss);

    // 실제로 ResourceManager에 보상을 추가하는 함수
    void GrantReward(int32 CurrentDay);

#pragma endregion RewardGain

#pragma region MaintenanceCost

private:
    UPROPERTY()
    TArray<TObjectPtr<UPotatoNPCManagementComp>> RegisteredNPCManagementComps;

    int32 TotalMaintenanceCost = 0;
    int32 TotalRetiredNPCCount = 0;

    int32 CollectMainteanceCosts(int32& OutRetiredCount);

public:
    void RegisterNPCManagementComp(UPotatoNPCManagementComp* NPCManagementComp);
    void UnregisterNPCManagementComp(UPotatoNPCManagementComp* NPCManagementComp);

#pragma endregion MaintenanceCost

public:
    // GameMode의 OnResultPhase 델리게이트에 바인딩 될 함수
    UFUNCTION()
    void OnResultPhaseAction();
};
