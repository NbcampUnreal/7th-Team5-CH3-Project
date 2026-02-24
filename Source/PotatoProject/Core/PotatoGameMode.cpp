#include "PotatoGameMode.h"
#include "PotatoDayNightCycle.h"
#include "PotatoResourceManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" 
#include "../Player/PotatoPlayerCharacter.h" 
#include "../Monster/PotatoMonsterSpawner.h" 
#include "../Animal/PotatoAnimalController.h"
#include "../NPC/PotatoNPC.h"
#include "../Core/PotatoEnums.h"

APotatoGameMode::APotatoGameMode() 
{
    
}

void APotatoGameMode::BeginPlay()
{
    Super::BeginPlay();
    StartGame();
}

void APotatoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (DayNightSystem)
    {
        // 델리게이트에 붙은 모든 바인딩도 제거됨
        DayNightSystem->OnDayStarted.Clear();
        DayNightSystem->OnNightStarted.Clear();
        DayNightSystem->OnEveningStarted.Clear();
        DayNightSystem->OnDawnStarted.Clear();

        DayNightSystem = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void APotatoGameMode::StartGame()
{
    // DayNight System 접근 및 초기화
    DayNightSystem = GetWorld()->GetSubsystem<UPotatoDayNightCycle>();
    if (!DayNightSystem) return;

    // 델리게이트 바인딩
    DayNightSystem->OnDayStarted.AddDynamic(this, &APotatoGameMode::StartDayPhase);
    DayNightSystem->OnEveningStarted.AddDynamic(this, &APotatoGameMode::StartWarningPhase);
    DayNightSystem->OnNightStarted.AddDynamic(this, &APotatoGameMode::StartNightPhase);
    DayNightSystem->OnDawnStarted.AddDynamic(this, &APotatoGameMode::StartResultPhase);

    DayNightSystem->StartSystem(DayDuration, EveningDuration, NightDuration, DawnDuration);

    // Resource System 접근 및 초기화
    ResourceManager = GetWorld()->GetSubsystem<UPotatoResourceManager>();
    if (!ResourceManager) return;

    ResourceManager->StartSystem(InitialWood, InitialStone, InitialCrop, InitialLivestock);
    PlayerCharacter = Cast<APotatoPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    if (!MonsterSpawner)
    {
        for (TActorIterator<APotatoMonsterSpawner> It(GetWorld()); It; ++It)
        {
            MonsterSpawner = *It;
            break;
        }
    }
    
    UClass* WarehouseClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/LHW/BluePrints/BP_WareHouse.BP_WareHouse"));
    if (!WarehouseActor)
    {
        for (TActorIterator<AActor> It(GetWorld(), WarehouseClass); It; ++It)
        {
            WarehouseActor = *It;
            if (WarehouseActor)break;
        }
    }
    if (WarehouseActor)
    {
        WarehouseActor->OnDestroyed.AddDynamic(this, &APotatoGameMode::OnHouseDestroyed);
    }
    
}

void APotatoGameMode::StartDayPhase()
{
   
    // 게임 로직 관련 넣고싶은 기능들을
    OnDayPhase.Broadcast();
    if (PlayerCharacter) {
        PlayerCharacter->SetIsBuildingMode(true);
    }
    if (MonsterSpawner)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Stop Wave!")));
        MonsterSpawner->NotifyTimeExpired();
    }
    if (AnimalController)
    {
        AnimalController->SetIsAnimalPosses(true);
    }
}

void APotatoGameMode::StartWarningPhase()
{
    // 이 함수들 아래에
    OnWarningPhase.Broadcast();
}

void APotatoGameMode::StartNightPhase()
{
    // 취향껏 추가하십쇼
    OnNightPhase.Broadcast();
    if (PlayerCharacter)
    {
        PlayerCharacter->SetIsBuildingMode(false);
    }
    if (MonsterSpawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GM] Spawner=%s Class=%s"),
            *GetNameSafe(MonsterSpawner),
            *GetNameSafe(MonsterSpawner ? MonsterSpawner->GetClass() : nullptr));
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("start Wave!")));
        FString WaveString = FString::FromInt(CurrentDay);

        // 2. FName으로 변환 (필요한 경우)
        FName s = FName(*WaveString);
       MonsterSpawner->StartWave(s);
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("start Wave: %s"), *s.ToString()));
    }
    if (AnimalController)
    {
        AnimalController->SetIsAnimalPosses(false);
    }
}

void APotatoGameMode::StartResultPhase()
{
    OnResultPhase.Broadcast();

    CurrentDay++;
    if (CheckVictoryCondition()) 
    {
        EndGame(true);
        return;
    }

    //보상 어떤식으로 줘야 할지 논의 필요
    ResourceManager->AddResource(EResourceType::Wood, 1);
    ResourceManager->AddResource(EResourceType::Stone, 1);
    ResourceManager->AddResource(EResourceType::Crop, 1);
    ResourceManager->AddResource(EResourceType::Livestock, 1);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("나무: %d"), ResourceManager->GetResource(EResourceType::Wood)));
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("광석: %d"), ResourceManager->GetResource(EResourceType::Stone )));
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("농작물: %d"), ResourceManager->GetResource(EResourceType::Crop)));
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("축산물: %d"), ResourceManager->GetResource(EResourceType::Livestock)));


    ///NPC가 추가될 때 마다 관리하는 array에 추가하는 코드로 바꿔야 할 듯.
    if (NPCs.IsEmpty())
    {
        for (TActorIterator<APotatoNPC> It(GetWorld()); It; ++It)
        {
            auto tmp = *It;
            if (tmp)
            {
                NPCs.Add(tmp);
            }
            //break;
        }
    }
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,FString::Printf(TEXT("npc 수: %d"), NPCs.Num()));
    for (int i = 0; i < NPCs.Num(); i++)
    {
        bool npcTryPay = NPCs[i]->TryPayMaintenance();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("유지비용지불시도: %s"), *LexToString(npcTryPay)));
    }
    // 보상 지급 및 결과 UI 표시?

}

void APotatoGameMode::EndGame(bool IsGameClear)
{
    
    if (MonsterSpawner)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Stop Wave!")));
        MonsterSpawner->NotifyGameOver();
    }
    if (IsGameClear)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("게임 클리어!")));
    }
    else {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("실패해서 게임 끝!")));
    }
}

bool APotatoGameMode::CheckVictoryCondition()
{
    if (CurrentDay > 10)
    {
        return true;
    }

    return false;
}

void APotatoGameMode::OnHouseDestroyed(AActor* DestroyedActor)
{
    EndGame(false);
}

float APotatoGameMode::GetPhaseStartTime(EDayPhase Phase) const
{
    switch (Phase)
    {
        case EDayPhase::Day:     return 0.f;
        case EDayPhase::Evening: return DayDuration;
        case EDayPhase::Night:   return DayDuration + EveningDuration;
        case EDayPhase::Dawn:    return DayDuration + EveningDuration + NightDuration;
        default:                 return 0.f;
    }
}