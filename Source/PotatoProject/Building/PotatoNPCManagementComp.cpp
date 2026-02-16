// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/PotatoNPCManagementComp.h"
#include "NPC/PotatoNPC.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Core/PotatoProductionComponent.h"
#include "Core/PotatoResourceManager.h"

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

    NPC->ProductionComp->Refund();

    // 3. 리스트에서 제거 및 Destroy
    AssignedNPCs.Remove(NPC);
    NPC->Destroy();
}
