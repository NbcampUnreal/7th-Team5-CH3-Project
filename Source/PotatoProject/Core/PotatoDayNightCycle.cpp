#include "PotatoDayNightCycle.h"
#include "PotatoResourceManager.h"

void UPotatoDayNightCycle::StartSystem(float InDayDuration, float InEveningDuration, float InNightDuration, float InDawnDuration)
{   
    if (bIsStarted) return;

    bIsStarted = true;

    CachedDayDuration = InDayDuration;
    CachedEveningDuration = InEveningDuration;
    CachedNightDuration = InNightDuration;
    CachedDawnDuration = InDawnDuration;

    TotalElapsedTime = 0.f;

    GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &UPotatoDayNightCycle::OnTimerTick, 1.f, true);

    EnterDay(CachedDayDuration);
}

void UPotatoDayNightCycle::EndSystem()
{
    if (!bIsStarted) return;

    bIsStarted = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }
}

// UI 갱신용
void UPotatoDayNightCycle::OnTimerTick()
{
    RemainingTime = FMath::Max(0.f, RemainingTime - 1.f);
    TotalElapsedTime += 1.f;

    UE_LOG(LogTemp, Log, TEXT("Remaining Time : %.1f"), RemainingTime);
}

void UPotatoDayNightCycle::EnterDay(float InDayDuration)
{
    CurrentPhase = EDayPhase::Day;
    RemainingTime = InDayDuration;
    TotalElapsedTime = 0.f;   // 새 사이클 시작 시 리셋
    OnDayStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterEvening(CachedEveningDuration); },
        InDayDuration, false);
}

void UPotatoDayNightCycle::EnterEvening(float InEveningDuration)
{
    CurrentPhase = EDayPhase::Evening;
    RemainingTime = InEveningDuration;
    TotalElapsedTime = CachedDayDuration;   // Day가 끝난 시점
    OnEveningStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterNight(CachedNightDuration); },
        InEveningDuration, false);
}

void UPotatoDayNightCycle::EnterNight(float InNightDuration)
{
    CurrentPhase = EDayPhase::Night;
    RemainingTime = InNightDuration;
    TotalElapsedTime = CachedDayDuration + CachedEveningDuration;   // Day+Evening이 끝난 시점
    OnNightStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterDawn(CachedDawnDuration); },
        InNightDuration, false);
}

void UPotatoDayNightCycle::EnterDawn(float InDawnDuration)
{
    CurrentPhase = EDayPhase::Dawn;
    RemainingTime = InDawnDuration;
    TotalElapsedTime = CachedDayDuration + CachedEveningDuration + CachedNightDuration;   // Day+Evening+Night이 끝난 시점
    OnDawnStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterDay(CachedDayDuration); },
        InDawnDuration, false);
}

void UPotatoDayNightCycle::Deinitialize()
{
    EndSystem();
    Super::Deinitialize();
}
