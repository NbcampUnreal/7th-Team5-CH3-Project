#include "PotatoResourceManager.h"
#include "PotatoDayNightCycle.h"
#include "PotatoEnums.h"

void UPotatoResourceManager::StartSystem(int32 InitWood, int32 InitStone, int32 InitCrop, int32 InitLivestock)
{
    if (bIsStarted) return;

    bIsStarted = true;

    Wood = InitWood;
    Stone = InitStone;
    Crop = InitCrop;
    Livestock = InitLivestock;

    if (UWorld* World = GetWorld())
    {
        DayNightCycle = World->GetSubsystem<UPotatoDayNightCycle>();

        // 1초 주기 생산 틱 시작
        World->GetTimerManager().SetTimer(
            ProductionTimerHandle,
            this,
            &UPotatoResourceManager::OnProductionTick,
            ProductionTickInterval, // 생산 간격
            true // 반복
        );
    }

    BroadcastAll();
}

void UPotatoResourceManager::EndSystem()
{
    if (!bIsStarted) return;

    bIsStarted = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ProductionTimerHandle);
    }

    Wood = 0;
    Stone = 0;
    Crop = 0;
    Livestock = 0;
    
    // 생산 레지스트리 초기화
    TotalProductionPerMinuteWood = 0;
    TotalProductionPerMinuteStone = 0;
    TotalProductionPerMinuteCrop = 0;
    TotalProductionPerMinuteLivestock = 0;
    AccumulatedWood = 0.f;
    AccumulatedStone = 0.f;
    AccumulatedCrop = 0.f;
    AccumulatedLivestock = 0.f;

    BroadcastAll();
}

void UPotatoResourceManager::BroadcastAll()
{
    BroadcastOne(EResourceType::Wood);
    BroadcastOne(EResourceType::Stone);
    BroadcastOne(EResourceType::Crop);
    BroadcastOne(EResourceType::Livestock);
}

void UPotatoResourceManager::BroadcastOne(EResourceType Type)
{
    OnResourceChanged.Broadcast(Type, GetResource(Type));
}

void UPotatoResourceManager::AddResource(EResourceType Type, int32 Amount)
{
    if (!bIsStarted || Amount < 0) return;

    switch (Type)
    {
        case EResourceType::Wood:      Wood += Amount;      break;
        case EResourceType::Stone:     Stone += Amount;     break;
        case EResourceType::Crop:      Crop += Amount;      break;
        case EResourceType::Livestock: Livestock += Amount;  break;
        default: return;
    }

    BroadcastOne(Type);
}

bool UPotatoResourceManager::RemoveResource(EResourceType Type, int32 Amount)
{
    if (!bIsStarted || Amount < 0 || !HasEnoughResource(Type, Amount)) return false;

    switch (Type)
    {
        case EResourceType::Wood:      Wood -= Amount;      break;
        case EResourceType::Stone:     Stone -= Amount;     break;
        case EResourceType::Crop:      Crop -= Amount;      break;
        case EResourceType::Livestock: Livestock -= Amount;  break;
        default: return false;
    }

    BroadcastOne(Type);
    return true;
}

int32 UPotatoResourceManager::GetResource(EResourceType Type) const
{
    switch (Type)
    {
        case EResourceType::Wood:            return Wood;
        case EResourceType::Stone:           return Stone;
        case EResourceType::Crop:            return Crop;
        case EResourceType::Livestock:       return Livestock;
        default:                             return -1;
    }
}

bool UPotatoResourceManager::HasEnoughResource(EResourceType Type, int32 Amount) const
{
    if (!bIsStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("Resource check failed: System not started."));
        return false;
    }

    if (Amount < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Resource check failed: Invalid amount %d."), Amount);
        return false;
    }

    return GetResource(Type) >= Amount;
}

// ----- 생산 등록 -----

void UPotatoResourceManager::RegisterProduction(int32 InWood, int32 InStone, int32 InCrop, int32 InLivestock)
{
    TotalProductionPerMinuteWood += InWood;
    TotalProductionPerMinuteStone += InStone;
    TotalProductionPerMinuteCrop += InCrop;
    TotalProductionPerMinuteLivestock += InLivestock;
    // 필요시 이곳에 델리게이트 추가
}

void UPotatoResourceManager::UnregisterProduction(int32 InWood, int32 InStone, int32 InCrop, int32 InLivestock)
{
    TotalProductionPerMinuteWood -= InWood;
    TotalProductionPerMinuteStone -= InStone;
    TotalProductionPerMinuteCrop -= InCrop;
    TotalProductionPerMinuteLivestock -= InLivestock;
    // 필요시 이곳에 델리게이트 추가
}

int32 UPotatoResourceManager::GetTotalProductionPerMinute(EResourceType Type) const
{
    switch (Type)
    {
        case EResourceType::Wood:      return TotalProductionPerMinuteWood;
        case EResourceType::Stone:     return TotalProductionPerMinuteStone;
        case EResourceType::Crop:      return TotalProductionPerMinuteCrop;
        case EResourceType::Livestock: return TotalProductionPerMinuteLivestock;
        default:                       return 0;
    }
}

void UPotatoResourceManager::OnProductionTick()
{
    if (!bIsStarted) return;

    // 낮에만 생산
    if (DayNightCycle && DayNightCycle->GetCurrentPhase() != EDayPhase::Day) return;

    AccumulateAndFlush(TotalProductionPerMinuteWood, AccumulatedWood, ProductionTickInterval, EResourceType::Wood);
    AccumulateAndFlush(TotalProductionPerMinuteStone, AccumulatedStone, ProductionTickInterval, EResourceType::Stone);
    AccumulateAndFlush(TotalProductionPerMinuteCrop, AccumulatedCrop, ProductionTickInterval, EResourceType::Crop);
    AccumulateAndFlush(TotalProductionPerMinuteLivestock, AccumulatedLivestock, ProductionTickInterval, EResourceType::Livestock);
}

// 초당 생산량 누적 후 정수 단위로 자원 추가
void UPotatoResourceManager::AccumulateAndFlush(int32 TotalPerMinute, float& Accumulated, float Interval, EResourceType Type)
{
    if (TotalPerMinute <= 0) return;

    const float PerSecond = static_cast<float>(TotalPerMinute) / 60.f;
    Accumulated += PerSecond * Interval;

    const int32 WholeAmount = FMath::FloorToInt32(Accumulated);
    if (WholeAmount < 0) return;

    Accumulated -= static_cast<float>(WholeAmount);
    AddResource(Type, WholeAmount);
}