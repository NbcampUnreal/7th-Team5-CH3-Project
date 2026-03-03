#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/PotatoEnums.h"
#include "TimerManager.h"
#include "PotatoGameMode.generated.h"

class UPotatoDayNightCycle;
class UPotatoResourceManager;
class APotatoPlayerCharacter;
class APotatoMonsterSpawner;
class APotatoAnimalController;
class UGameOverScreen;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDayPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNightPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarningPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowResultPanel);

UCLASS()
class POTATOPROJECT_API APotatoGameMode : public AGameMode
{
	GENERATED_BODY()
	
private:
    bool bGameEnded = false;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UGameOverScreen> GameOverScreenClass;

public:
    APotatoGameMode();

    void StartGame();
    void EndGame(bool IsGameClear, FText Message = FText::GetEmpty());
    bool CheckVictoryCondition();

    bool IsGameEnded() const { return bGameEnded; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    UFUNCTION()
    void OnHouseDestroyed(AActor* DestroyedActor);
    
    UFUNCTION(BlueprintCallable, Category = "GameOver")
    void RegisterWarehouse(AActor* Warehouse);

#pragma region DayNightSystem
private:
    UPROPERTY()
    UPotatoDayNightCycle* DayNightSystem;
    
    int32 CurrentDay = 1;

    // 게임 시작 직 후 첫 번째 StartDayPhase이 호출 될 때 CurrentDay 증가 시키지 않음
    bool bIsFirstDay = true;
    
private:
    FTimerHandle TH_ResultPanel;
    bool bResultPanelOpened = false;

public:
    // -- Day-night cycle BP 설정용입니다. --
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DayDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float EveningDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float NightDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DayNight|Duration", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DawnDuration = 3.0f;

public:
    UPROPERTY(BlueprintAssignable)
    FOnDayPhase OnDayPhase;

    UPROPERTY(BlueprintAssignable)
    FOnNightPhase OnNightPhase;

    UPROPERTY(BlueprintAssignable)
    FOnWarningPhase OnWarningPhase;

    UPROPERTY(BlueprintAssignable)
    FOnResultPhase OnResultPhase;

    UPROPERTY(BlueprintAssignable)
    FOnShowResultPanel OnShowResultPanel;
    
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

    /**
     * 하루(Day -> Evening -> Night -> Dawn) 전체 길이(초)를 반환합니다.
     * DayClockNeedle 진행도 계산 등에 사용됩니다.
     */
    UFUNCTION(BlueprintPure, Category = "DayNight")
    float GetTotalCycleDuration() const { return DayDuration + EveningDuration + NightDuration + DawnDuration; }

    /**
     * 주어진 페이즈가 사이클 전체에서 시작되는 시점(초)을 반환합니다.
     * Day=0, Evening=DayDuration, Night=Day+Evening, Dawn=Day+Evening+Night
     */
    float GetPhaseStartTime(EDayPhase Phase) const;

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

    APotatoPlayerCharacter* PlayerCharacter;

    UFUNCTION()
    void HandleRoundFinished(int32 Round);

    // ResultPhase 중복 호출 가드
    bool bResultPhaseTriggered = false;
    
    UPROPERTY(EditAnywhere, Category = "Spawner")
    TSubclassOf<APotatoMonsterSpawner> MonsterSpawnerClass;

    APotatoMonsterSpawner* MonsterSpawner = nullptr;
    
    UPROPERTY(EditAnywhere, Category = "Spawner")
    TSubclassOf<APotatoAnimalController> AnimalControllerClass;

    APotatoAnimalController* AnimalController;

    UPROPERTY(EditAnywhere, Category = "Spawner")
    TObjectPtr<AActor> WarehouseActor;

    // 재생할 사운드 에셋 (에디터에서 할당)
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* DayBGM;
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* NightBGM;

    // 현재 재생 중인 오디오 컴포넌트 관리용
    UPROPERTY()
    UAudioComponent* DayAudioComponent;
    UPROPERTY()
    UAudioComponent* NightAudioComponent;

#pragma endregion ResourceSystem
    
#pragma region DialogueSystem
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Story|Dialogues")
    TMap<int32, FName> DayPhaseDialogues;
    
    UPROPERTY(EditDefaultsOnly, Category = "Story|Dialogues")
    TMap<int32, FName> NightPhaseDialogues;
    
    void TriggerStoryDialogue(FName RowName);
    
#pragma endregion DialogueSystem
};
