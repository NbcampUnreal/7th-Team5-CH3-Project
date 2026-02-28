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
#include "UI/GameOverScreen.h"
#include "Player/PotatoPlayerController.h"
#include "UI/PotatoPlayerHUD.h"

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

    OnDayPhase.Broadcast();
    if (PlayerCharacter) {
        PlayerCharacter->SetIsBuildingMode(true);
    }
    
    if (AnimalController)
    {
        AnimalController->SetIsAnimalPosses(true);
    }
    
    // 오늘 재생할 스토리 다이얼로그가 있는지 확인하고, 있다면 재생
    if (const FName* ScheduledDialogue = DayPhaseDialogues.Find(CurrentDay))
    {
        // DAY 1의 경우, 1.5초를 기다림
        if (CurrentDay == 1)
        {
            FTimerHandle IntroTimerHandle;
            FTimerDelegate IntroDelegate = FTimerDelegate::CreateUObject(
                this,
                &APotatoGameMode::TriggerStoryDialogue,
                *ScheduledDialogue);
            GetWorld()->GetTimerManager().SetTimer(IntroTimerHandle, IntroDelegate, 1.5f, false);
        }
        TriggerStoryDialogue(*ScheduledDialogue);
    }
}

void APotatoGameMode::StartWarningPhase()
{
    OnWarningPhase.Broadcast();

    if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
    {
        if (PC->PlayerHUDWidget)
        {
            PC->PlayerHUDWidget->ShowMessageWithDuration(
                NSLOCTEXT("HUD", "EveningWarning", "밤이 찾아옵니다..."), 3.0f, true);
        }
    }
}

void APotatoGameMode::StartNightPhase()
{
    OnNightPhase.Broadcast();
    if (PlayerCharacter)
    {
        PlayerCharacter->SetIsBuildingMode(false);
        PlayerCharacter->CloseAllPopups(); // 밤 진입 시 모든 팝업 강제 닫기
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
    
    // 오늘 재생할 스토리 다이얼로그가 있는지 확인하고, 있다면 재생
    if (const FName* ScheduledDialogue = NightPhaseDialogues.Find(CurrentDay))
    {
        TriggerStoryDialogue(*ScheduledDialogue);
    }
}

void APotatoGameMode::StartResultPhase()
{
    OnResultPhase.Broadcast();
    
    if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
    {
        if (PC->PlayerHUDWidget)
        {
            PC->PlayerHUDWidget->ShowMessageWithDuration(
                NSLOCTEXT("HUD", "DawnMessage", "날이 밝아옵니다!"),
                3.0f,
                true,
                FSimpleDelegate::CreateLambda([this]()
                {
                    if (MonsterSpawner)
                    {
                        MonsterSpawner->NotifyTimeExpired();
                    }

                    OnShowResultPanel.Broadcast();
                })
            );
        }
    }
}

void APotatoGameMode::EndGame(bool IsGameClear, FText Message)
{
    
    if (MonsterSpawner)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Stop Wave!")));
        MonsterSpawner->NotifyGameOver();
    }

    // 총 처치수 조회
    APotatoGameStateBase* PotatoGameState = GetGameState<APotatoGameStateBase>();
    int32 TotalKills = PotatoGameState ? PotatoGameState->GetTotalKilledEnemy() : 0;

    APotatoPlayerController* PlayerController = GetWorld()->GetFirstPlayerController<APotatoPlayerController>();
    if (!PlayerController || !GameOverScreenClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController or GameOverScreenClass is null!"));
        return;
    }

    UGameOverScreen* GameOverScreen = CreateWidget<UGameOverScreen>(PlayerController, GameOverScreenClass);
    if (!GameOverScreen)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create GameOverScreen widget!"));
        return;
    }

    GameOverScreen->InitScreen(IsGameClear, CurrentDay, TotalKills, Message);
    GameOverScreen->AddToViewport();

    PlayerController->SetPause(true);
    PlayerController->SetUIMode(true, GameOverScreen);
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
    EndGame(false, NSLOCTEXT("GameOver", "Warehouse", "창고가 몬스터들에게 남김없이 약탈당했습니다..."));
}

void APotatoGameMode::RegisterWarehouse(AActor* Warehouse)
{
    if (!Warehouse)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to register a null Warehouse!"));
        return;
    }
 
    WarehouseActor = Warehouse;
    WarehouseActor->OnDestroyed.AddDynamic(this, &APotatoGameMode::OnHouseDestroyed);
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

void APotatoGameMode::TriggerStoryDialogue(FName RowName)
{
    if (APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        if (PlayerController->PlayerHUDWidget)
        {
            PlayerController->PlayerHUDWidget->PlayDialogue(RowName);
        }
    }
}
