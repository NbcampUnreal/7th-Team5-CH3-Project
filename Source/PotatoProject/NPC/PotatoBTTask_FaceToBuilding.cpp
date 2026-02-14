// Fill out your copyright notice in the Description page of Project Settings.


#include "NPC/PotatoBTTask_FaceToBuilding.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UPotatoBTTask_FaceToBuilding::UPotatoBTTask_FaceToBuilding()
{
    NodeName = TEXT("Face To Building");
    bNotifyTick = true;

    BuildingKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UPotatoBTTask_FaceToBuilding, BuildingKey),
        AActor::StaticClass());
}

EBTNodeResult::Type UPotatoBTTask_FaceToBuilding::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp) return EBTNodeResult::Failed;

    AActor* TargetBuilding = Cast<AActor>(BlackboardComp->GetValueAsObject(BuildingKey.SelectedKeyName));
    if (!IsValid(TargetBuilding)) return EBTNodeResult::Failed;

    APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!Pawn) return EBTNodeResult::Failed;

    return EBTNodeResult::InProgress;
}

void UPotatoBTTask_FaceToBuilding::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

    if (!Pawn || !BlackboardComp) 
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AActor* Building = Cast<AActor>(BlackboardComp->GetValueAsObject(BuildingKey.SelectedKeyName));
    if (!IsValid(Building))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    FVector ToBuilding = Building->GetActorLocation() - Pawn->GetActorLocation();
    ToBuilding.Z = 0;

    if (ToBuilding.IsNearlyZero())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    FRotator TargetRotation = ToBuilding.Rotation();
    FRotator CurrentRotation = Pawn->GetActorRotation();

    FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaSeconds, RotationSpeed);
    Pawn->SetActorRotation(NewRotation);

    float AngleDiff = FMath::Abs(FRotator::NormalizeAxis(NewRotation.Yaw - TargetRotation.Yaw));
    if (AngleDiff <= AcceptanceAngle)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}