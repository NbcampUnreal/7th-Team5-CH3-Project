// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PotatoRewardGenerator.h"
#include "Core/PotatoResourceManager.h"
#include "Core/PotatoGameMode.h"
#include "Core/PotatoGameStateBase.h"
#include "Core/PotatoDayRewardRow.h"
#include "Building/PotatoNPCManagementComp.h"
#include "Player/PotatoPlayerController.h"
#include "UI/ResultScreen.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APotatoRewardGenerator::APotatoRewardGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

}

void APotatoRewardGenerator::BeginPlay()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Log, TEXT("APotatoRewardGenerator::BeginPlay - Invalid World"));
        return;
    }

    ResourceManager = World->GetSubsystem <UPotatoResourceManager>();
    if (!ResourceManager)
    {
        UE_LOG(LogTemp, Log, TEXT("APotatoRewardGenerator::BeginPlay - Failed to get PotatoResourceManager"));
        return;
    }

    PotatoGameMode = Cast<APotatoGameMode>(World->GetAuthGameMode());
    if (!PotatoGameMode)
    {
        UE_LOG(LogTemp, Log, TEXT("APotatoRewardGenerator::BeginPlay - Failed to get PotatoGameMode"));
        return;
    }

    PotatoGameMode->OnResultPhase.AddDynamic(this, &APotatoRewardGenerator::OnResultPhaseAction);
    UE_LOG(LogTemp, Log, TEXT("APotatoRewardGenerator::BeginPlay - Successfully initialized references and bound to OnResultPhase"));
}

void APotatoRewardGenerator::GrantReward(int32 CurrentDay)
{
    if (!RewardTable || !ResourceManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::GrantReward - Missing RewardTable or ResourceManager"));
        return;
    }

    static const FString ContextString(TEXT("RewardTableContext"));
    FPotatoDayRewardRow* RewardData = RewardTable->FindRow<FPotatoDayRewardRow>(*FString::FromInt(CurrentDay), ContextString);

    if (!RewardData)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::GrantReward - No reward data found for Day %d"), CurrentDay);
        return;
    }

    if (RewardData->WoodReward > 0)
        ResourceManager->AddResource(EResourceType::Wood, RewardData->WoodReward);

    if (RewardData->StoneReward > 0)
        ResourceManager->AddResource(EResourceType::Stone, RewardData->StoneReward);
    
    if (RewardData->CropReward > 0)
        ResourceManager->AddResource(EResourceType::Crop, RewardData->CropReward);
    
    if (RewardData->LivestockReward > 0)
        ResourceManager->AddResource(EResourceType::Livestock, RewardData->LivestockReward);

    UE_LOG(LogTemp, Log, TEXT("Successfully granted rewards for Day %d: Wood=%d, Stone=%d, Crop=%d, Livestock=%d"),
        CurrentDay, RewardData->WoodReward, RewardData->StoneReward, RewardData->CropReward, RewardData->LivestockReward);

}

void APotatoRewardGenerator::ShowResultScreen(int32 InCurrentDay, int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss)
{
    if (!ResultScreenClass) return;

    if (!ResultScreen)
    {
        ResultScreen = CreateWidget<UResultScreen>(GetWorld(), ResultScreenClass);
    }
    if (!ResultScreen) return;

    FPotatoDayRewardRow RewardData;
    bool bHasReward = false;
    if (RewardTable)
    {
        FPotatoDayRewardRow* Found = RewardTable->FindRow<FPotatoDayRewardRow>(
            *FString::FromInt(InCurrentDay), TEXT("ShowResultContext"));
        if (Found)
        {
            RewardData = *Found;
            bHasReward = true;
        }
    }

    ResultScreen->InitScreen(
        InCurrentDay,
        InKilledMonster, InKilledElite, InKilledBoss,
        bHasReward ? RewardData.WoodReward      : 0,
        bHasReward ? RewardData.StoneReward     : 0,
        bHasReward ? RewardData.CropReward      : 0,
        bHasReward ? RewardData.LivestockReward : 0,
        TotalMaintenanceCost,
        TotalRetiredNPCCount
    );

    
    if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
    {
        PC->SetPause(true);
        ResultScreen->AddToViewport();
        PC->SetUIMode(true, ResultScreen);
    }
}

int32 APotatoRewardGenerator::CollectMainteanceCosts(int32& OutRetiredCount)
{
    int32 TotalCost = 0;
    OutRetiredCount = 0;

    for (UPotatoNPCManagementComp* Comp : RegisteredNPCManagementComps)
    {
        if (!Comp) continue;

        int32 CompRetired = 0;
        int32 Cost = Comp->ProcessNPCMaintenance(CompRetired);
        TotalCost += Cost;
        OutRetiredCount += CompRetired;
    }

    UE_LOG(LogTemp, Log, TEXT("CollectMainteanceCosts - Total: %d, Retired NPCs: %d"), TotalCost, OutRetiredCount);
    return TotalCost;
}

void APotatoRewardGenerator::RegisterNPCManagementComp(UPotatoNPCManagementComp* NPCManagementComp)
{
    if (NPCManagementComp && !RegisteredNPCManagementComps.Contains(NPCManagementComp))
    {
        RegisteredNPCManagementComps.Add(NPCManagementComp);
        UE_LOG(LogTemp, Log, TEXT("RewardGenerator: NPCManagementComp registered - Total Count: %d"), RegisteredNPCManagementComps.Num());
    }

}

void APotatoRewardGenerator::UnregisterNPCManagementComp(UPotatoNPCManagementComp* NPCManagementComp)
{
    if (RegisteredNPCManagementComps.Remove(NPCManagementComp))
    {
        UE_LOG(LogTemp, Log, TEXT("RewardGenerator: NPCManagementComp unregistered - Total Count: %d"), RegisteredNPCManagementComps.Num());
    }
}

void APotatoRewardGenerator::OnResultPhaseAction()
{
    if (!PotatoGameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::OnResultPhaseAction - PotatoGameMode is not valid"));
        return;
    }

    int32 CurrentDay = PotatoGameMode->GetCurrentDay();

    // 1. 유지비 계산 및 차감 + 퇴직 NPC 수 수집
    int32 RetiredCount = 0;
    TotalMaintenanceCost = CollectMainteanceCosts(RetiredCount);
    TotalRetiredNPCCount = RetiredCount;

    // 2. 킬 통계 조회
    APotatoGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<APotatoGameStateBase>() : nullptr;
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::OnResultPhaseAction - Failed to get PotatoGameStateBase"));
        return;
    }
    
    int32 KilledNormal = GameState->GetKilledNormal();
    int32 KilledElite = GameState->GetKilledElite();
    int32 KilledBoss = GameState->GetKilledBoss();

    // 3. 보상 지급
    GrantReward(CurrentDay);

    // 4. 결과 화면 표시
    ShowResultScreen(CurrentDay, KilledNormal, KilledElite, KilledBoss);

    // 5. 다음 Day 준비(GameState의 데이터 초기화 수행)
    GameState->ResetKillStats();
}