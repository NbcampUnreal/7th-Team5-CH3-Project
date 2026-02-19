// Fill out your copyright notice in the Description page of Project Settings.

#include "Animal/PotatoAnimalController.h"
#include "Animal/PotatoAnimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"

const FName APotatoAnimalController::Key_AssignedBuilding(TEXT("AssignedBuilding"));
const FName APotatoAnimalController::Key_MovingArea(TEXT("MovingArea"));

APotatoAnimalController::APotatoAnimalController()
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
    IsAnimalPosses = true;
}

void APotatoAnimalController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    APotatoAnimal* Animal = Cast<APotatoAnimal>(InPawn);
    if (!Animal && !IsAnimalPosses) return;

    if (!BehaviorTreeAsset || !BehaviorTreeAsset->BlackboardAsset) return;

    BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
    RunBehaviorTree(BehaviorTreeAsset);

    if (IsValid(Animal->AssignedBarn))
    {
        BlackboardComp->SetValueAsObject(Key_AssignedBuilding, Animal->AssignedBarn);
    }
    
    if (UBoxComponent* MovingArea = Animal->MovingArea.Get())
    {
        BlackboardComp->SetValueAsObject(Key_MovingArea, Cast<UObject>(MovingArea));
    }
}

void APotatoAnimalController::OnUnPossess()
{
    Super::OnUnPossess();

    if (BehaviorComp && IsAnimalPosses)
    {
        BehaviorComp->StopTree();
    }
}

void APotatoAnimalController::SetIsAnimalPosses(bool IsPossess)
{
    IsAnimalPosses = IsPossess;
}
