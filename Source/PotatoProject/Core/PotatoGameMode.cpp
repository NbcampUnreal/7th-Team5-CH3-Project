#include "PotatoGameMode.h"
#include "PotatoDayNightCycle.h"
#include "PotatoResourceManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "../Player/PotatoPlayerCharacter.h" 
#include "../Monster/PotatoMonsterSpawner.h" 
#include "../Animal/PotatoAnimalController.h"

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
    
    /*if (MonsterSpawnerClass)
    {
        FActorSpawnParameters SpawnParams;
        MonsterSpawner = GetWorld()->SpawnActor<APotatoMonsterSpawner>(
            MonsterSpawnerClass,
            FVector(-40.f, -4340.f, 690.f),
            FRotator::ZeroRotator,
            SpawnParams
        );
    }*/
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
        MonsterSpawner->StopWave();
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
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("start Wave!")));
        FString WaveString = FString::FromInt(CurrentDay) + TEXT("-1");

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
    CheckVictoryCondition();
    // 보상 지급 및 결과 UI 표시?
}

void APotatoGameMode::EndGame()
{
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("게임 끝!")));
}

void  APotatoGameMode::CheckVictoryCondition()
{
    if (CurrentDay > 10)
    {
        EndGame();
    }
}