#pragma once

#include "CoreMinimal.h"
#include "PotatoDayNightCycle.generated.h"

UENUM()
enum class EDayPhase : uint8
{
    Day UMETA(DisplayName = "Day"),
    Evening UMETA(DisplayName = "Evening"),
    Night UMETA(DisplayName = "Night"),
    Dawn UMETA(DisplayName = "Dawn")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhaseChanged);

UCLASS()
class POTATOPROJECT_API UPotatoDayNightCycle : public UWorldSubsystem
{
    GENERATED_BODY()

private:
    EDayPhase CurrentPhase = EDayPhase::Day;
    float RemainingTime = 0.f;

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
    void StartSystem(float InDayDuration, float InEveningDuration, float InNightDuration, float InDawnDuration);
    void EndSystem();

    void EnterDay(float InDayDuration);
    void EnterEvening(float InEveningDuration);
    void EnterNight(float InNightDuration);
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

protected:
    virtual void Deinitialize() override;

};