#include "PotatoResourceManager.h"

void UPotatoResourceManager::StartSystem(int32 InitWood, int32 InitStone, int32 InitCrop, int32 InitLivestock)
{
    if (bIsStarted) return;

    bIsStarted = true;

    Wood = InitWood;
    Stone = InitStone;
    Crop = InitCrop;
    Livestock = InitLivestock;
    BroadcastAll();
}

void UPotatoResourceManager::EndSystem()
{
    if (!bIsStarted) return;

    bIsStarted = false;

    Wood = 0;
    Stone = 0;
    Crop = 0;
    Livestock = 0;
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
    if (!bIsStarted) return;

    if (Amount <= 0) return;
    switch (Type)
    {
        case EResourceType::Wood:
            Wood += Amount;
        break;
        case EResourceType::Stone:
            Stone += Amount;
            break;
        case EResourceType::Crop:
            Crop += Amount;
            break;
        case EResourceType::Livestock:
            Livestock += Amount;
            break;
        default:
            return;
    }

    BroadcastOne(Type);
}

bool UPotatoResourceManager::RemoveResource(EResourceType Type, int32 Amount)
{
    if (!bIsStarted) return false;

    if (Amount <= 0 || !HasEnoughResource(Type, Amount)) return false;
    
    switch (Type)
    {
        case EResourceType::Wood:
            Wood -= Amount;
            break;
        case EResourceType::Stone:
            Stone -= Amount;
            break;
        case EResourceType::Crop:
            Crop -= Amount;
            break;
        case EResourceType::Livestock:
            Livestock -= Amount;
            break;
        default:
            return false;
    }
    
    BroadcastOne(Type);
    return true;
}

int32 UPotatoResourceManager::GetResource(EResourceType Type) const
{
    switch (Type)
    {
        case EResourceType::Wood:
            return Wood;
        case EResourceType::Stone:
            return Stone;
        case EResourceType::Crop:
            return Crop;
        case EResourceType::Livestock:
            return Livestock;
        default:
            return -1;
    }
}

bool UPotatoResourceManager::HasEnoughResource(EResourceType Type, int32 Amount) const
{
    if (!bIsStarted) return false;
    
    if (Amount <= 0) return false;

    return GetResource(Type) >= Amount;
}