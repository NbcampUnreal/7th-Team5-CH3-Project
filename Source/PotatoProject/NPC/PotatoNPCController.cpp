// Fill out your copyright notice in the Description page of Project Settings.

#include "PotatoNPCController.h"
#include "PotatoNPC.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"

const FName APotatoNPCController::Key_AssignedBuilding(TEXT("AssignedBuilding"));
const FName APotatoNPCController::Key_MovingArea(TEXT("MovingArea"));

APotatoNPCController::APotatoNPCController()
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

void APotatoNPCController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);

    APotatoNPC* NPC = Cast<APotatoNPC>(InPawn);
    if (!NPC) return;

    // BT/ BB 유효성 검사
    if (!BehaviorTreeAsset || !BehaviorTreeAsset->BlackboardAsset) return;

    // BB 초기화 이후 BT 실행
    BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
    RunBehaviorTree(BehaviorTreeAsset);

    if (IsValid(NPC->AssignedBuilding))
    {
        BlackboardComp->SetValueAsObject(Key_AssignedBuilding, NPC->AssignedBuilding);
    }

    if (UBoxComponent* MovingArea = NPC->MovingArea.Get())
    {
        BlackboardComp->SetValueAsObject(Key_MovingArea, Cast<UObject>(MovingArea));
    }
}

void APotatoNPCController::OnUnPossess()
{
    Super::OnUnPossess();

    if (BehaviorComp)
    {
        BehaviorComp->StopTree();
    }

}
