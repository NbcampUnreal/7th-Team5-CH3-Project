// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PotatoNPCRegistrySubsystem.h"
#include "Core/PotatoRewardGenerator.h"

void UPotatoNPCRegistrySubsystem::RegisterNPCComp(UPotatoNPCManagementComp* NPCManagementComp)
{
    if (NPCManagementComp && !RegisteredComps.Contains(NPCManagementComp))
    {
        RegisteredComps.Add(NPCManagementComp);
    }
}

void UPotatoNPCRegistrySubsystem::UnregisterNPCComp(UPotatoNPCManagementComp* NPCManagementComp)
{
    RegisteredComps.Remove(NPCManagementComp);

}

void UPotatoNPCRegistrySubsystem::Deinitialize()
{
    RegisteredComps.Empty();
    Super::Deinitialize();
}
