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
#include "Core/PotatoNPCRegistrySubsystem.h"

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

    UPotatoNPCRegistrySubsystem* RegistrySystem = World->GetSubsystem<UPotatoNPCRegistrySubsystem>();
    if (!RegistrySystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::BeginPlay - Failed to get PotatoNPCRegistrySubsystem"));
        return;
    }
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

static EIconItemType ResourceTypeToIconType(EResourceType InType)
{
    switch (InType)
    {
        case EResourceType::Wood:      return EIconItemType::Wood;
        case EResourceType::Stone:     return EIconItemType::Stone;
        case EResourceType::Crop:      return EIconItemType::Crop;
        case EResourceType::Livestock: return EIconItemType::Livestock;
        default:                       return EIconItemType::Wood;
    }
}

// ENPCType -> EIconItemType 변환
static EIconItemType NPCTypeToIconType(ENPCType InType)
{
    switch (InType)
    {
        case ENPCType::Lumberjack: return EIconItemType::Lumberjack;
        case ENPCType::Miner:      return EIconItemType::Miner;
        default:                   return EIconItemType::Lumberjack;
    }
}

void APotatoRewardGenerator::ShowResultScreen(
    int32 InCurrentDay,
    int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss,
    const TMap<ENPCType, int32>& InRetiredByType, int32 InTotalRetired)
{
    if (!ResultScreenClass) return;

    // 이전 ResultScreen이 남아있으면 제거 후 새로 생성
    // (재사용 시 NativeConstruct가 재호출되어 AddDynamic 중복 바인딩 발생)
    if (ResultScreen)
    {
        ResultScreen = nullptr;
    }

    ResultScreen = CreateWidget<UResultScreen>(GetWorld(), ResultScreenClass);
    if (!ResultScreen)
    {
        return;
    }

    // 보상 배열 조립
    TArray<TPair<EIconItemType, int32>> RewardItems;

    if (RewardTable)
    {
        FPotatoDayRewardRow* Row = RewardTable->FindRow<FPotatoDayRewardRow>(
            *FString::FromInt(InCurrentDay), TEXT("ShowResultContext"));
        if (Row)
        {
            if (Row->WoodReward > 0)
                RewardItems.Add({ ResourceTypeToIconType(EResourceType::Wood), Row->WoodReward });
            if (Row->StoneReward > 0)
                RewardItems.Add({ ResourceTypeToIconType(EResourceType::Stone), Row->StoneReward });
            if (Row->CropReward > 0)
                RewardItems.Add({ ResourceTypeToIconType(EResourceType::Crop), Row->CropReward });
            if (Row->LivestockReward > 0)
                RewardItems.Add({ ResourceTypeToIconType(EResourceType::Livestock), Row->LivestockReward });
        }
    }

    // 비용 배열 조립 
    TArray<TPair<EIconItemType, int32>> CostItems;

    // 유지비 (축산물 아이콘)
    if (TotalMaintenanceCost > 0)
        CostItems.Add({ ResourceTypeToIconType(EResourceType::Livestock), TotalMaintenanceCost });

    // 퇴직 NPC (타입별)
    for (const auto& Pair : InRetiredByType)
    {
        if (Pair.Value > 0)
            CostItems.Add({ NPCTypeToIconType(Pair.Key), Pair.Value });
    }

    // ResultScreen 초기화
    ResultScreen->InitScreen(
        InCurrentDay,
        InKilledMonster, InKilledElite, InKilledBoss,
        RewardItems, CostItems);

    if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
    {
        ResultScreen->AddToViewport();
        PC->SetUIMode(true, ResultScreen);
    }

    // Day 페이즈가 시작되면 ResultScreen을 자동으로 닫음
    // (버튼을 누르지 않아도 다음 Day가 되면 화면이 사라짐)
    PotatoGameMode->OnDayPhase.AddDynamic(ResultScreen, &UResultScreen::CloseScreen);
}

int32 APotatoRewardGenerator::CollectMainteanceCosts(TMap<ENPCType, int32>& OutRetiredByType)
{
    int32 TotalCost = 0;

    UPotatoNPCRegistrySubsystem* RegistrySystem = GetWorld()->GetSubsystem<UPotatoNPCRegistrySubsystem>();
    if (!RegistrySystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("APotatoRewardGenerator::CollectMainteanceCosts - Failed to get PotatoNPCRegistrySubsystem"));
        return TotalCost;
    }

    for (UPotatoNPCManagementComp* Comp : RegistrySystem->GetRegisteredComps())
    {
        if (!Comp) continue;

        int32 Cost = Comp->ProcessNPCMaintenance(OutRetiredByType);
        TotalCost += Cost;
    }

    int32 TotalRetired = 0;
    for (auto& Pair : OutRetiredByType)
    {
        TotalRetired += Pair.Value;
    }
    UE_LOG(LogTemp, Log, TEXT("CollectMainteanceCosts - Total: %d, Retired NPCs: %d"), TotalCost, TotalRetired);
    return TotalCost;
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
    TMap<ENPCType, int32> MaintenanceCollectMap;
    TotalMaintenanceCost = CollectMainteanceCosts(MaintenanceCollectMap);

    // 퇴직 NPC 총 수 집계
    int32 TotalRetired = 0;
    for (auto& Pair : MaintenanceCollectMap)
    {
        TotalRetired += Pair.Value;
    }

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

    // 4. 결과 화면 표시 (MaintenanceCollectMap 전달)
    ShowResultScreen(CurrentDay, KilledNormal, KilledElite, KilledBoss, MaintenanceCollectMap, TotalRetired);

    // 5. 다음 Day 준비(GameState의 데이터 초기화 수행)
    GameState->ResetKillStats();
}

void APotatoRewardGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}