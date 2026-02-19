#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PotatoGameMode.generated.h"

class UPotatoDayNightCycle;
class UPotatoResourceManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDayPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNightPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarningPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultPhase);

UCLASS()
class POTATOPROJECT_API APotatoGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
    APotatoGameMode();

    void StartGame();
    void EndGame();
    void CheckVictoryCondition();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region DayNightSystem
private:
    UPROPERTY()
    UPotatoDayNightCycle* DayNightSystem;
    
    int32 CurrentDay = 1;

public:
    // -- Day-night cycle BP 설정용입니다. --
    UPROPERTY(EditDefaultsOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DayDuration = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float EveningDuration = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float NightDuration = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DawnDuration = 30.0f;

public:
    UPROPERTY(BlueprintAssignable)
    FOnDayPhase OnDayPhase;

    UPROPERTY(BlueprintAssignable)
    FOnNightPhase OnNightPhase;

    UPROPERTY(BlueprintAssignable)
    FOnWarningPhase OnWarningPhase;

    UPROPERTY(BlueprintAssignable)
    FOnResultPhase OnResultPhase;

public:
    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void StartDayPhase();

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void StartWarningPhase();

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void StartNightPhase();

    UFUNCTION(BlueprintCallable, Category = "DayNight")
    void StartResultPhase();

    UFUNCTION(BlueprintPure, Category = "DayNight")
    int32 GetCurrentDay() const { return CurrentDay; }

#pragma endregion DayNightSystem

#pragma region ResourceSystem
private:
    UPROPERTY()
    UPotatoResourceManager* ResourceManager;

    // -- Resource Manager BP 설정용입니다. --
    UPROPERTY(EditDefaultsOnly, Category = "ResourceSystem|Initial")
    int32 InitialWood = 100;
    UPROPERTY(EditDefaultsOnly, Category = "ResourceSystem|Initial")
    int32 InitialStone = 100;
    UPROPERTY(EditDefaultsOnly, Category = "ResourceSystem|Initial")
    int32 InitialCrop = 50;
    UPROPERTY(EditDefaultsOnly, Category = "ResourceSystem|Initial")
    int32 InitialLivestock = 50;

#pragma endregion ResourceSystem
};
