#include "PotatoDayNightCycle.h"

void UPotatoDayNightCycle::StartSystem(float InDayDuration, float InEveningDuration, float InNightDuration, float InDawnDuration)
{   
    CachedDayDuration = InDayDuration;
    CachedEveningDuration = InEveningDuration;
    CachedNightDuration = InNightDuration;
    CachedDawnDuration = InDawnDuration;

    GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &UPotatoDayNightCycle::OnTimerTick, 1.f, true);

    EnterDay(CachedDayDuration);
}

void UPotatoDayNightCycle::EndSystem()
{

}

// UI 갱신용
void UPotatoDayNightCycle::OnTimerTick()
{
    RemainingTime = FMath::Max(0.f, RemainingTime - 1.f);

    // 여기 델리게이트 부착하면 될 것 같습니다
    UE_LOG(LogTemp, Log, TEXT("Remaining Time : %.1f"), RemainingTime);
}

void UPotatoDayNightCycle::EnterDay(float InDayDuration)
{
    CurrentPhase = EDayPhase::Day;
    RemainingTime = InDayDuration;
    OnDayStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterEvening(CachedEveningDuration); },
        InDayDuration, false);
}

void UPotatoDayNightCycle::EnterEvening(float InEveningDuration)
{
    CurrentPhase = EDayPhase::Evening;
    RemainingTime = InEveningDuration;
    OnEveningStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterNight(CachedNightDuration); },
        InEveningDuration, false);
}

void UPotatoDayNightCycle::EnterNight(float InNightDuration)
{
    CurrentPhase = EDayPhase::Night;
    RemainingTime = InNightDuration;
    OnNightStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterDawn(CachedDawnDuration); },
        InNightDuration, false);
}

void UPotatoDayNightCycle::EnterDawn(float InDawnDuration)
{
    CurrentPhase = EDayPhase::Dawn;
    RemainingTime = InDawnDuration;
    OnDawnStarted.Broadcast();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimerHandle, [this]() { EnterDay(CachedDayDuration); },
        InDawnDuration, false);
}

void UPotatoDayNightCycle::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }
    Super::Deinitialize();
}
