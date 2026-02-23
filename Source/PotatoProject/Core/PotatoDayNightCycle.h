#pragma once

#include "CoreMinimal.h"
#include "PotatoEnums.h"
#include "PotatoDayNightCycle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhaseChanged);

UCLASS()
class POTATOPROJECT_API UPotatoDayNightCycle : public UWorldSubsystem
{
    GENERATED_BODY()

private:
    EDayPhase CurrentPhase = EDayPhase::Day;
    float RemainingTime = 0.f;
    float TotalElapsedTime = 0.f;   // 현재 사이클 전체 기준 경과 시간(초)

    bool bIsStarted = false;

    // Day Phase 전환 핸들러
    FTimerHandle PhaseTimerHandle;

    float CachedDayDuration = 0.f;
    float CachedEveningDuration = 0.f;
    float CachedNightDuration = 0.f;
    float CachedDawnDuration = 0.f;

    // 게임 Timer Tick 핸들러
    FTimerHandle TickTimerHandle;

    UFUNCTION()
    void OnTimerTick();

#pragma region DayData
public:
    // Gamemode에서 Init용으로
    UFUNCTION(BlueprintCallable, Category = "DaySystem")
    void StartSystem(float InDayDuration, float InEveningDuration, float InNightDuration, float InDawnDuration);
    UFUNCTION(BlueprintCallable, Category = "DaySystem")
    void EndSystem();
    UFUNCTION(BlueprintPure, Category = "DaySystem")
    bool IsSystemStarted() const { return bIsStarted; }

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void EnterDay(float InDayDuration);

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void EnterEvening(float InEveningDuration);

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void EnterNight(float InNightDuration);

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void EnterDawn(float InDawnDuration);

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    EDayPhase GetCurrentPhase() const { return CurrentPhase; }

public:
    UPROPERTY(BlueprintAssignable, Category = "DayNight|Event")
    FOnPhaseChanged OnDayStarted;

    UPROPERTY(BlueprintAssignable, Category = "DayNight|Event")
    FOnPhaseChanged OnEveningStarted;

    UPROPERTY(BlueprintAssignable, Category = "DayNight|Event")
    FOnPhaseChanged OnNightStarted;

    UPROPERTY(BlueprintAssignable, Category = "DayNight|Event")
    FOnPhaseChanged OnDawnStarted;

#pragma endregion DayData

public:
    UFUNCTION(BlueprintCallable, Category = "DayNight")
    float GetRemainingDayTime() const { return RemainingTime; }

    // 전체 사이클 기준 경과 시간 (0 ~ TotalDuration)
    UFUNCTION(BlueprintPure, Category = "DayNight")
    float GetTotalElapsedTime() const { return TotalElapsedTime; }

protected:
    virtual void Deinitialize() override;

};