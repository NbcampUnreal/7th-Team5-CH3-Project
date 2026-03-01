// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/PotatoAnimalManagementComp.h"
#include "Components/BoxComponent.h"
#include "Core/PotatoProductionComponent.h"
#include "Animal/PotatoAnimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "Core/PotatoResourceManager.h"
#include "Player/PotatoPlayerController.h"

UPotatoAnimalManagementComp::UPotatoAnimalManagementComp()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoAnimalManagementComp::BeginPlay()
{
    Super::BeginPlay();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("animalComp!"));
    // Owner Actor에서 BoxComponent 자동 탐색 & 캐싱
    if (AActor* Owner = GetOwner())
    {
        CachedBoxComp = Owner->FindComponentByClass<UBoxComponent>();
    }

    if (!CachedBoxComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("AnimalManagement: No BoxComponent found on owner [%s]."),
            *GetNameSafe(GetOwner()));
    }

    // 디버그용
    //GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    //{
    //     SpawnAnimal(EAnimalType::Cow);
    //     SpawnAnimal(EAnimalType::Pig);
    //     SpawnAnimal(EAnimalType::Chicken);
    //});
}

bool UPotatoAnimalManagementComp::SpawnAnimal(EAnimalType AnimalType)
{
    // 0. Enum -> BP 클래스 조회
    TSubclassOf<APotatoAnimal>* Found = AnimalClassMap.Find(AnimalType);
    if (!Found || !*Found)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spawn animal: No class mapped for this AnimalType."));
        return false;
    }
    TSubclassOf<APotatoAnimal> AnimalClass = *Found;

    // 1. 슬롯 체크
    if (AssignedAnimals.Num() >= MaxAnimalCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spawn animal: Max capacity reached."));
        return false;
    }

    // 2. 캐싱된 BoxComponent 사용
    if (!CachedBoxComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spawn animal: No BoxComp cached on owner."));
        return false;
    }

    // 3. 구매 비용 차감 - ProductionComponent -> TryPurchase
    APotatoAnimal* DefaultAnimal = AnimalClass->GetDefaultObject<APotatoAnimal>();
    if (!DefaultAnimal || !DefaultAnimal->ProductionComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spawn animal: Invalid AnimalClass or missing ProductionComp on default object."));
        return false;
    }

    if (!DefaultAnimal->ProductionComp->TryPurchaseWithWorld(GetWorld()->GetSubsystem<UPotatoResourceManager>()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot spawn animal: Not enough resources to purchase."));
        return false;
    }

    // 4. 영역 내 랜덤 위치에 스폰 (겹침 시 자동 위치 조정)
    FVector Origin = CachedBoxComp->GetComponentLocation();
    FVector Extent = CachedBoxComp->GetScaledBoxExtent();
    FVector SpawnLocation = UKismetMathLibrary::RandomPointInBoundingBox(Origin, Extent);
    FRotator SpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APotatoAnimal* NewAnimal = GetWorld()->SpawnActor<APotatoAnimal>(
        AnimalClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (!NewAnimal)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn animal actor."));
        return false;
    }

    // 5. 동물 초기화 (할당된 헛간, 이동 영역)
    NewAnimal->InitializeWithBarn(GetOwner(), CachedBoxComp);
    AssignedAnimals.Add(NewAnimal);

    return true;
}

void UPotatoAnimalManagementComp::RemoveAnimal(APotatoAnimal* Animal)
{
    // 0. Animal 유효성 체크
    if (!Animal)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot remove animal: Invalid animal reference."));
        return;
    }

    // 1. 리스트 유효성 체크
    if (AssignedAnimals.Num() == 0 || !AssignedAnimals.Contains(Animal))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot remove animal: Animal not found in assigned list."));
        return;
    }

    // 2. 환급 처리 - ProductionComponent -> Refund
    if (!Animal->ProductionComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot remove animal: missing ProductionComp."));
        return;
    }

    Animal->ProductionComp->Refund();
    
    // 3. 리스트에서 제거 및 Destory
    AssignedAnimals.Remove(Animal);
    Animal->Destroy();
}

void UPotatoAnimalManagementComp::OnPlayerEnter(APotatoPlayerController* PC)
{
	if (!PC) return;
	PC->ShowInterGuide();
}

void UPotatoAnimalManagementComp::OnPlayerExit(APotatoPlayerController* PC)
{
	if (!PC) return;
	PC->HideAnimalPopup();
	PC->HideInterGuide();
}

void UPotatoAnimalManagementComp::Interact(APotatoPlayerController* PC)
{
	if (!PC) return;
	PC->ToggleAnimalPopup(this);
}
