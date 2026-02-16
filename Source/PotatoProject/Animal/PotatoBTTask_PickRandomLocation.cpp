// Fill out your copyright notice in the Description page of Project Settings.


#include "Animal/PotatoBTTask_PickRandomLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"

UPotatoBTTask_PickRandomLocation::UPotatoBTTask_PickRandomLocation()
{
    NodeName = TEXT("Pick Random Location");
    
    // Moving Area 필터링 - BoxComponent만 허용
    MovingAreaKey.AddObjectFilter(this, 
        GET_MEMBER_NAME_CHECKED(UPotatoBTTask_PickRandomLocation, MovingAreaKey), UBoxComponent::StaticClass());

    // Target Location - Vector
    TargetLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UPotatoBTTask_PickRandomLocation, TargetLocationKey));
}

EBTNodeResult::Type UPotatoBTTask_PickRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("PickRandomLocation: No BlackboardComponent found."));
        return EBTNodeResult::Failed;
    }

    UObject* Building = BlackboardComp->GetValueAsObject(MovingAreaKey.SelectedKeyName);
    UBoxComponent* TargetArea = Cast<UBoxComponent>(Building);

    if (!TargetArea)
    {
        UE_LOG(LogTemp, Warning, TEXT("PickRandomLocation: Invalid BoxComponent in MovingAreaKey."));
        return EBTNodeResult::Failed;
    }

    // 랜덤 위치 계산
    FVector Origin = TargetArea->GetComponentLocation();
    FVector BoxExtent = TargetArea->GetScaledBoxExtent();
    FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(Origin, BoxExtent);

    BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, RandomPoint);

    return EBTNodeResult::Succeeded;
}
