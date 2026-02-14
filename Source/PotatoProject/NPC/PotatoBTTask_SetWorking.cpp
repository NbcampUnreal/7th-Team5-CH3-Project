// Fill out your copyright notice in the Description page of Project Settings.


#include "NPC/PotatoBTTask_SetWorking.h"
#include "PotatoNPC.h"
#include "AIController.h"

UPotatoBTTask_SetWorking::UPotatoBTTask_SetWorking()
{
    NodeName = TEXT("Set Working State");
}

EBTNodeResult::Type UPotatoBTTask_SetWorking::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!Pawn) return EBTNodeResult::Failed;

    APotatoNPC* NPC = Cast<APotatoNPC>(Pawn);
    if (!NPC) return EBTNodeResult::Failed;

    NPC->bIsWorking = bWorking;
    return EBTNodeResult::Succeeded;
}