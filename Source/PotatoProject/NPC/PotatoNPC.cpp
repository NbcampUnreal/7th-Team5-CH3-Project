#include "PotatoNPC.h"
#include "PotatoNPCController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/PotatoProductionComponent.h"
#include "Core/PotatoResourceManager.h"

APotatoNPC::APotatoNPC()
{
 	PrimaryActorTick.bCanEverTick = true;

	// 건물 설정 후 수동으로 AIController 부착 (작업지시 후 SpawnDefaultController)
	AutoPossessAI = EAutoPossessAI::Disabled;

	// AI Controller 기본 클래스 설정
	AIControllerClass = APotatoNPCController::StaticClass();

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Capsule->SetCollisionResponseToAllChannels(ECR_Block);
    }

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 100.0f;
    }

	// 생산 컴포넌트
	ProductionComp = CreateDefaultSubobject<UPotatoProductionComponent>(TEXT("ProductionComp"));
}

void APotatoNPC::BeginPlay()
{
	Super::BeginPlay();
	
}

void APotatoNPC::InitializeWithBuilding(AActor* InBuilding, UBoxComponent* InMovingArea)
{
	AssignedBuilding = InBuilding;
	MovingArea = InMovingArea;

	// 데이터 주입 완료 후 수동으로 AIController 부착
	SpawnDefaultController();
}

bool APotatoNPC::TryPayMaintenance()
{
	if (!ProductionComp) return false;

	UPotatoResourceManager* ResourceManager = GetWorld()->GetSubsystem<UPotatoResourceManager>();
	if (!ResourceManager) return false;

	if (!ResourceManager->HasEnoughResource(EResourceType::Livestock, MaintenanceCostLivestock))
	{
		// 축산물 부족시 false 반환 (퇴직 처리는 위임)
		return false;
	}

    ResourceManager->RemoveResource(EResourceType::Livestock, MaintenanceCostLivestock);
	return true;
}

void APotatoNPC::Retire()
{
	if (ProductionComp)
	{
		ProductionComp->Refund();
	}

	// Destroy시 Component UnregisterProduction 자동 호출
	Destroy();
}