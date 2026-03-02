// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/PotatoProductionComponent.h"
#include "Core/PotatoResourceManager.h"

UPotatoProductionComponent::UPotatoProductionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoProductionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (UWorld* World = GetWorld())
	{
		ResourceManager = World->GetSubsystem<UPotatoResourceManager>();
	}

	if (bSkipProductionRegistration)
	{
		return;
	}

	if (ResourceManager)
	{
		ResourceManager->RegisterProduction(
			ProductionPerMinuteWood,
			ProductionPerMinuteStone,
			ProductionPerMinuteCrop,
			ProductionPerMinuteLivestock
		);
	}
}

void UPotatoProductionComponent::EnableProductionRegistration()
{
	bSkipProductionRegistration = false;

	if (!ResourceManager)
	{
		if (UWorld* World = GetWorld())
		{
			ResourceManager = World->GetSubsystem<UPotatoResourceManager>();
		}
	}

	if (ResourceManager)
	{
		ResourceManager->RegisterProduction(
			ProductionPerMinuteWood,
			ProductionPerMinuteStone,
			ProductionPerMinuteCrop,
			ProductionPerMinuteLivestock
		);
	}
}

void UPotatoProductionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!bSkipProductionRegistration && ResourceManager)
	{
		ResourceManager->UnregisterProduction(
			ProductionPerMinuteWood,
			ProductionPerMinuteStone,
			ProductionPerMinuteCrop,
			ProductionPerMinuteLivestock
		);
	}

	Super::EndPlay(EndPlayReason);
}

bool UPotatoProductionComponent::TryPurchase()
{
	// Actor 배치 전 호출될 수 있으므로 캐싱된 ResourceManager 대신 즉시 가져옴
	UPotatoResourceManager* RM = GetWorld() ? GetWorld()->GetSubsystem<UPotatoResourceManager>() : nullptr;
	if (!RM)
	{
        UE_LOG(LogTemp, Warning, TEXT("TryPurchase failed: No ResourceManager found in world."));
		return false;
	}

	if (!RM->HasEnoughResource(EResourceType::Wood, BuyCostWood)) return false;
	if (!RM->HasEnoughResource(EResourceType::Stone, BuyCostStone)) return false;
	if (!RM->HasEnoughResource(EResourceType::Crop, BuyCostCrop)) return false;
	if (!RM->HasEnoughResource(EResourceType::Livestock, BuyCostLivestock)) return false;

	RM->RemoveResource(EResourceType::Wood, BuyCostWood);
	RM->RemoveResource(EResourceType::Stone, BuyCostStone);
	RM->RemoveResource(EResourceType::Crop, BuyCostCrop);
	RM->RemoveResource(EResourceType::Livestock, BuyCostLivestock);

    return true;
}

// CDO 호출용 헬퍼함수
bool UPotatoProductionComponent::TryPurchaseWithWorld(UPotatoResourceManager* OuterResourceManager)
{
    if (!OuterResourceManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("TryPurchase failed: OuterResourceManager is null."));
        return false;
    }

    if (!OuterResourceManager->HasEnoughResource(EResourceType::Wood, BuyCostWood)) return false;
    if (!OuterResourceManager->HasEnoughResource(EResourceType::Stone, BuyCostStone)) return false;
    if (!OuterResourceManager->HasEnoughResource(EResourceType::Crop, BuyCostCrop)) return false;
    if (!OuterResourceManager->HasEnoughResource(EResourceType::Livestock, BuyCostLivestock)) return false;

    OuterResourceManager->RemoveResource(EResourceType::Wood, BuyCostWood);
    OuterResourceManager->RemoveResource(EResourceType::Stone, BuyCostStone);
    OuterResourceManager->RemoveResource(EResourceType::Crop, BuyCostCrop);
    OuterResourceManager->RemoveResource(EResourceType::Livestock, BuyCostLivestock);

    return true;
}


void UPotatoProductionComponent::Refund()
{
	// Actor 배치 전 호출될 수 있으므로 캐싱된 ResourceManager 대신 즉시 가져옴
	UPotatoResourceManager* RM = GetWorld() ? GetWorld()->GetSubsystem<UPotatoResourceManager>() : nullptr;
	if (!RM)
	{
        UE_LOG(LogTemp, Warning, TEXT("Refund failed: No ResourceManager found in world."));
		return;
	}

	if(RefundWood > 0) RM->AddResource(EResourceType::Wood, RefundWood);
	if(RefundStone > 0) RM->AddResource(EResourceType::Stone, RefundStone);
	if(RefundCrop > 0) RM->AddResource(EResourceType::Crop, RefundCrop);
	if(RefundLivestock > 0) RM->AddResource(EResourceType::Livestock, RefundLivestock);
}

// CDO 호출용 헬퍼함수
void UPotatoProductionComponent::RefundWithWorld(UPotatoResourceManager* OuterResourceManager)
{
    if (!OuterResourceManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("Refund failed: OuterResourceManager is null."));
        return;
    }

    if (RefundWood > 0) OuterResourceManager->AddResource(EResourceType::Wood, RefundWood);
    if (RefundStone > 0) OuterResourceManager->AddResource(EResourceType::Stone, RefundStone);
    if (RefundCrop > 0) OuterResourceManager->AddResource(EResourceType::Crop, RefundCrop);
    if (RefundLivestock > 0) OuterResourceManager->AddResource(EResourceType::Livestock, RefundLivestock);
}
