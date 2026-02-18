#include "PotatoAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Navigation/PathFollowingComponent.h"
#include "PotatoMonster.h"

const FName APotatoAIController::Key_WarehouseActor(TEXT("WarehouseActor"));
const FName APotatoAIController::Key_CurrentTarget(TEXT("CurrentTarget"));
const FName APotatoAIController::Key_bIsDead(TEXT("bIsDead"));
const FName APotatoAIController::Key_SpecialLogic(TEXT("SpecialLogic"));
const FName APotatoAIController::Key_MoveTarget(TEXT("MoveTarget"));
const FName APotatoAIController::Key_MoveGoal(TEXT("MoveGoal"));
const FName APotatoAIController::Key_bIsAttacking(TEXT("bIsAttacking"));
const FName APotatoAIController::Key_bInAttackRange(TEXT("bInAttackRange"));

APotatoAIController::APotatoAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

static FString ObjName(UObject* Obj)
{
    return Obj ? Obj->GetName() : FString(TEXT("None"));
}

void APotatoAIController::LogBBKeyObjects() const
{
    if (!BlackboardComp) return;

    auto DumpObj = [&](const FName& Key)
        {
            UObject* V = BlackboardComp->GetValueAsObject(Key);
            UE_LOG(LogTemp, Warning, TEXT("[AICon][BB] %s = %s"), *Key.ToString(), *ObjName(V));
        };

    DumpObj(Key_WarehouseActor);
    DumpObj(Key_MoveTarget);
    DumpObj(Key_MoveGoal);
    DumpObj(Key_CurrentTarget);
}

void APotatoAIController::LogBBKeyBools() const
{
    if (!BlackboardComp) return;

    auto DumpBool = [&](const FName& Key)
        {
            const bool b = BlackboardComp->GetValueAsBool(Key);
            UE_LOG(LogTemp, Warning, TEXT("[AICon][BB] %s = %s"), *Key.ToString(), b ? TEXT("true") : TEXT("false"));
        };

    DumpBool(Key_bIsDead);
    DumpBool(Key_bIsAttacking);
    DumpBool(Key_bInAttackRange);

    const int32 Special = BlackboardComp->GetValueAsInt(Key_SpecialLogic);
    UE_LOG(LogTemp, Warning, TEXT("[AICon][BB] %s = %d"), *Key_SpecialLogic.ToString(), Special);
}

void APotatoAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    APotatoMonster* Monster = Cast<APotatoMonster>(InPawn);
    if (!Monster) return;

    Monster->ApplyPresetsOnce();

    UBehaviorTree* BT = Monster->GetBehaviorTreeToRun();
    if (!BT || !BT->BlackboardAsset) return;

    UBlackboardComponent* BB = nullptr;

    // ✅ 이 한 줄이 “BT가 실제로 도는” 가장 큰 차이
    if (!UseBlackboard(BT->BlackboardAsset, BB))
    {
        UE_LOG(LogTemp, Warning, TEXT("[AICon] UseBlackboard=FAILED"));
        return;
    }

    const bool bBT = RunBehaviorTree(BT);
    UE_LOG(LogTemp, Warning, TEXT("[AICon] RunBehaviorTree=%s"), bBT ? TEXT("OK") : TEXT("FAILED"));

    if (!BB) BB = GetBlackboardComponent();
    if (!BB) return;

    // Warehouse
    if (IsValid(Monster->WarehouseActor))
        BB->SetValueAsObject(Key_WarehouseActor, Monster->WarehouseActor);
    else
        BB->ClearValue(Key_WarehouseActor);

    // MoveTarget init
    {
        AActor* Existing = Cast<AActor>(BB->GetValueAsObject(Key_MoveTarget));
        if (!IsValid(Existing))
        {
            AActor* FirstTarget = (Monster->LanePoints.Num() > 0) ? Monster->GetCurrentLaneTarget() : nullptr;
            if (!IsValid(FirstTarget)) FirstTarget = Monster->WarehouseActor;

            if (IsValid(FirstTarget)) BB->SetValueAsObject(Key_MoveTarget, FirstTarget);
            else BB->ClearValue(Key_MoveTarget);
        }
    }

    // ✅ MoveGoal init (필수)
    {
        AActor* ExistingGoal = Cast<AActor>(BB->GetValueAsObject(TEXT("MoveGoal")));
        if (!IsValid(ExistingGoal))
        {
            AActor* MT = Cast<AActor>(BB->GetValueAsObject(Key_MoveTarget));
            AActor* WH = Cast<AActor>(BB->GetValueAsObject(Key_WarehouseActor));
            AActor* InitGoal = MT ? MT : WH;

            if (IsValid(InitGoal)) BB->SetValueAsObject(TEXT("MoveGoal"), InitGoal);
            else BB->ClearValue(TEXT("MoveGoal"));
        }
    }

    BB->SetValueAsBool(Key_bIsDead, Monster->bIsDead);
    BB->SetValueAsInt(Key_SpecialLogic, (int32)Monster->FinalStats.SpecialLogic);

    BB->ClearValue(Key_CurrentTarget);
}
