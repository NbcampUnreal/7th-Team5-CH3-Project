#include "PotatoGameMode.h"
#include "PotatoDayNightCycle.h"
#include "Subsystems/WorldSubsystem.h"

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
}

void APotatoGameMode::StartDayPhase()
{
    // 게임 로직 관련 넣고싶은 기능들을
    OnDayPhase.Broadcast();
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

}

void  APotatoGameMode::CheckVictoryCondition()
{
    if (CurrentDay > 10)
    {
        EndGame();
    }
}