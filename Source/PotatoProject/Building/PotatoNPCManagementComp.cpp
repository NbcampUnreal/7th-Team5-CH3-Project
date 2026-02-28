// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/PotatoNPCManagementComp.h"
#include "NPC/PotatoNPC.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Core/PotatoProductionComponent.h"
#include "Core/PotatoResourceManager.h"
#include "Core/PotatoRewardGenerator.h"
#include "Core/PotatoGameMode.h"
#include "Core/PotatoNPCRegistrySubsystem.h"

UPotatoNPCManagementComp::UPotatoNPCManagementComp()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoNPCManagementComp::BeginPlay()
{
    Super::BeginPlay();

    // Owner Actor에서 BoxComponent 자동 탐색 & 캐싱
    if (AActor* Owner = GetOwner())
    {
        CachedBoxComp = Owner->FindComponentByClass<UBoxComponent>();
    }

    if (!CachedBoxComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("NPCManagement: No BoxComponent found on owner [%s]."),
            *GetNameSafe(GetOwner()));
    }

    // RewardGenerator에 등록 (GameMode 참조 경유)
    if (UPotatoNPCRegistrySubsystem* RegistrySystem = GetWorld()->GetSubsystem<UPotatoNPCRegistrySubsystem>())
    {
        RegistrySystem->RegisterNPCComp(this);
    }

    // 디버그용
    /*GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        HireNPC(ENPCType::Lumberjack);
    });*/
}

void UPotatoNPCManagementComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // RewardGenerator에서 등록 해제
    if (UPotatoNPCRegistrySubsystem* RegistrySystem = GetWorld()->GetSubsystem<UPotatoNPCRegistrySubsystem>())
    {
        RegistrySystem->UnregisterNPCComp(this);
    }

    Super::EndPlay(EndPlayReason);
}

bool UPotatoNPCManagementComp::HireNPC(ENPCType NPCType)
{
    // 0. Enum -> TSubclassOf 매핑 조회
    TSubclassOf<APotatoNPC>* Found = NPCClassMap.Find(NPCType);
    if (!Found || !*Found)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot hire NPC: No class mapped for NPCType %d."), (int32)NPCType);
        return false;
    }
    TSubclassOf<APotatoNPC> NPCClass = *Found;
        
	// 1. 슬롯 체크
    if (AssignedNPCs.Num() >= MaxNPCCount)
    { 
        UE_LOG(LogTemp, Warning, TEXT("Cannot hire NPC: Max capacity reached."));
        return false;
    }
    
    // 2. 캐싱된 BoxComponent 사용
    if (!CachedBoxComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot hire NPC: No BoxComp cached on owner."));
        return false;
    }

    // 3. 구매 비용 차감 - ProductionComponent -> TryPurchase
    APotatoNPC* DefaultNPC = NPCClass->GetDefaultObject<APotatoNPC>();
    if (DefaultNPC == nullptr || DefaultNPC->ProductionComp == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot hire NPC: Invalid NPCClass or missing ProductionComp on default object."));
        return false;
    }

    if (!DefaultNPC->ProductionComp->TryPurchaseWithWorld(GetWorld()->GetSubsystem<UPotatoResourceManager>()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot hire NPC: Not enough resources to purchase."));
        return false;
    }

    // 4. 영역 내 랜덤 위치에 스폰 (겹침 시 자동 위치 조정)
    FVector Origin = CachedBoxComp->GetComponentLocation();
    FVector Extent = CachedBoxComp->GetScaledBoxExtent();
    FVector SpawnLocation = UKismetMathLibrary::RandomPointInBoundingBox(Origin, Extent);
    FRotator SpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APotatoNPC* NewNPC = GetWorld()->SpawnActor<APotatoNPC>(
        NPCClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (!NewNPC)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn NPC actor."));
        return false;
    }

    // 5. 초기화 (건물 참조 저장 + 작업 시작)
    NewNPC->InitializeWithBuilding(GetOwner(), CachedBoxComp);
    // 6. 목록에 추가
    AssignedNPCs.Add(NewNPC);

    return true;
}

void UPotatoNPCManagementComp::FireNPC(APotatoNPC* NPC)
{
    // 0. NPC 유효성 체크
    if (!NPC)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot fire NPC: Invalid NPC reference."));
        return;
    }

    // 1. 리스트 유효성 체크
    if (AssignedNPCs.Num() == 0 || !AssignedNPCs.Contains(NPC))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot fire NPC: NPC not found in assigned list."));
        return;
    }

    // 2. 환급 처리 - ProductionComponent -> Refund
    if (!NPC->ProductionComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot fire NPC: missing ProductionComp."));
        return;
    }

    // 3. 리스트에서 제거 및 Destroy
    AssignedNPCs.Remove(NPC);
    NPC->Retire();
}

int32 UPotatoNPCManagementComp::ProcessNPCMaintenance(TMap<ENPCType, int32>& OutRetiredByType)
{
    int32 TotalPaid = 0;
    TArray<APotatoNPC*> NPCsToRetire;

    for (APotatoNPC* NPC : AssignedNPCs)
    {
        if (!NPC) 
        {
            continue;
        }

        if (NPC->TryPayMaintenance())
        {
            // 지급 성공 시
            TotalPaid += NPC->GetMaintenanceCost();
            UE_LOG(LogTemp, Log, TEXT("Paid maintenance for NPC [%s]: %d"), *GetNameSafe(NPC), NPC->GetMaintenanceCost());
        }
        else
        {
            // 지급 실패 시(퇴직)
            NPCsToRetire.Add(NPC);
            UE_LOG(LogTemp, Log, TEXT("Failed to pay maintenance for NPC [%s]. Marking for retirement."), *GetNameSafe(NPC));
        }
    }

    // 퇴직 처리 (리스트에서 제거 + Destroy)
    for (APotatoNPC* NPC : NPCsToRetire)
    {
        if (!NPC)
        {
            continue;
        }
        OutRetiredByType.FindOrAdd(NPC->Type)++;
        AssignedNPCs.Remove(NPC);
        NPC->Destroy();
    }

    return TotalPaid;
}
