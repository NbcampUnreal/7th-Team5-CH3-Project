#include "PotatoGameMode.h"
#include "PotatoDayNightCycle.h"
#include "PotatoResourceManager.h"
#include "PotatoRewardGenerator.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" 
#include "../Player/PotatoPlayerCharacter.h" 
#include "../Monster/PotatoMonsterSpawner.h" 
#include "../Animal/PotatoAnimalController.h"
#include "../Core/PotatoEnums.h"
#include "Core/PotatoGameStateBase.h"

APotatoGameMode::APotatoGameMode() 
{
    GameStateClass = APotatoGameStateBase::StaticClass();
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
    if (bIsFirstDay)
    {
        bIsFirstDay = false;
    }
    else
    {
        if (CheckVictoryCondition())
        {
            EndGame(true);
            return;
        }
        CurrentDay++;
    }
    UE_LOG(LogTemp, Log, TEXT("DayPhase %d"), CurrentDay);
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