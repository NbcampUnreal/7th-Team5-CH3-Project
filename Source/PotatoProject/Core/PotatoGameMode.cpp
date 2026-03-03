// ============================================================================
// PotatoGameMode.cpp (FINAL) - Victory blocks ResultPhase + clean shutdown guard
// - VictoryCondition 충족 시 ResultPhase(보상/결과)로 진입하지 않음
// - RoundFinished(웨이브 종료 스킵)에서도 Victory 먼저 체크
// - Game 종료 후 페이즈/타이머 재진입 방지(bGameEnded)
// - ResultPanel 보험 타이머/메시지 콜백 중복 방지 유지
// ============================================================================

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

#include "Components/AudioComponent.h"

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
	// DayNight 델리게이트 정리
	if (DayNightSystem)
	{
		DayNightSystem->OnDayStarted.Clear();
		DayNightSystem->OnNightStarted.Clear();
		DayNightSystem->OnEveningStarted.Clear();
		DayNightSystem->OnDawnStarted.Clear();
		DayNightSystem = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TH_ResultPanel);
	}

	// Spawner 델리게이트 정리(안전)
	if (MonsterSpawner)
	{
		MonsterSpawner->OnRoundFinished.RemoveDynamic(this, &APotatoGameMode::HandleRoundFinished);
		MonsterSpawner = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APotatoGameMode::StartGame()
{
	bGameEnded = false;

	// DayNight System 접근 및 초기화
	DayNightSystem = GetWorld()->GetSubsystem<UPotatoDayNightCycle>();
	if (!DayNightSystem) return;

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

	// Spawner 찾기
	if (!MonsterSpawner)
	{
		for (TActorIterator<APotatoMonsterSpawner> It(GetWorld()); It; ++It)
		{
			MonsterSpawner = *It;
			break;
		}
	}

	//  Spawner RoundFinished 바인딩
	if (MonsterSpawner)
	{
		MonsterSpawner->OnRoundFinished.RemoveDynamic(this, &APotatoGameMode::HandleRoundFinished);
		MonsterSpawner->OnRoundFinished.AddDynamic(this, &APotatoGameMode::HandleRoundFinished);
	}
}

void APotatoGameMode::StartDayPhase()
{
	if (bGameEnded) return;

	//  새 Day로 들어오면 ResultPhase 중복 가드 해제
	bResultPhaseTriggered = false;
	bResultPanelOpened = false;

	if (bIsFirstDay)
	{
		bIsFirstDay = false;
	}
	else
	{
		//  Day 진입 시점 승리 체크(기존 유지)
		if (CheckVictoryCondition())
		{
			EndGame(true);
			return;
		}

		CurrentDay++;
	}

	UE_LOG(LogTemp, Log, TEXT("DayPhase %d"), CurrentDay);

	OnDayPhase.Broadcast();

	if (PlayerCharacter)
	{
		PlayerCharacter->SetIsBuildingMode(true);
	}

	if (AnimalController)
	{
		AnimalController->SetIsAnimalPosses(true);
	}

	// 오늘 재생할 스토리 다이얼로그가 있는지 확인하고, 있다면 재생
	if (const FName* ScheduledDialogue = DayPhaseDialogues.Find(CurrentDay))
	{
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

	// 낮 BGM 재생
	if (DayBGM && IsBGM)
	{
		DayAudioComponent = UGameplayStatics::SpawnSound2D(this, DayBGM);
	}
}

void APotatoGameMode::StartWarningPhase()
{
	if (bGameEnded) return;

	OnWarningPhase.Broadcast();

	if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
	{
		if (PC->PlayerHUDWidget)
		{
			PC->PlayerHUDWidget->ShowMessageWithDuration(
				NSLOCTEXT("HUD", "EveningWarning", "밤이 찾아옵니다..."),
				3.0f,
				true
			);
		}
	}

	// 낮 음악 fadeout
	if (DayAudioComponent && DayAudioComponent->IsPlaying())
	{
		DayAudioComponent->FadeOut(EveningDuration, 0.0f);
	}
}

void APotatoGameMode::StartNightPhase()
{
	if (bGameEnded) return;

	OnNightPhase.Broadcast();

	if (PlayerCharacter)
	{
		PlayerCharacter->SetIsBuildingMode(false);
		PlayerCharacter->CloseAllPopups();
	}

	if (MonsterSpawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] Spawner=%s Class=%s"),
			*GetNameSafe(MonsterSpawner),
			*GetNameSafe(MonsterSpawner ? MonsterSpawner->GetClass() : nullptr));

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("start Wave!")));

		FString WaveString = FString::FromInt(CurrentDay);
		FName WaveId(*WaveString);

		MonsterSpawner->StartWave(WaveId);

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("start Wave: %s"), *WaveId.ToString()));
	}

	if (AnimalController)
	{
		AnimalController->SetIsAnimalPosses(false);
	}

	if (const FName* ScheduledDialogue = NightPhaseDialogues.Find(CurrentDay))
	{
		TriggerStoryDialogue(*ScheduledDialogue);
	}

	// 밤 BGM 재생
	if (NightBGM && IsBGM)
	{
		NightAudioComponent = UGameplayStatics::SpawnSound2D(this, NightBGM);
	}
}

void APotatoGameMode::StartResultPhase()
{
	//  게임이 이미 끝났으면 어떤 이유로든 ResultPhase 진입 금지
	if (bGameEnded) return;

	//  VictoryCondition이면 ResultPhase(보상/결과)로 넘어가지 않음
	//    - 특히 RoundFinished에서 ForceToDawn으로 들어오는 케이스를 여기서도 차단
	if (CheckVictoryCondition())
	{
		EndGame(true);
		return;
	}

	if (bResultPhaseTriggered) return;
	bResultPhaseTriggered = true;

	OnResultPhase.Broadcast();

	auto OpenResultPanelOnce = [this]()
	{
		if (!GetWorld()) return;
		if (bGameEnded) return;
		if (bResultPanelOpened) return;   //  중복 방지
		bResultPanelOpened = true;

		// 보험 타이머 살아있으면 여기서도 제거(추가 안전)
		GetWorld()->GetTimerManager().ClearTimer(TH_ResultPanel);

		OnShowResultPanel.Broadcast();
	};

	//  보험 타이머(3초)
	GetWorld()->GetTimerManager().ClearTimer(TH_ResultPanel);
	GetWorld()->GetTimerManager().SetTimer(
		TH_ResultPanel,
		FTimerDelegate::CreateLambda([OpenResultPanelOnce]() { OpenResultPanelOnce(); }),
		3.0f,
		false
	);

	//  메시지 콜백에서도 “열기”는 동일 함수로만
	if (APotatoPlayerController* PC = GetWorld()->GetFirstPlayerController<APotatoPlayerController>())
	{
		if (PC->PlayerHUDWidget)
		{
			PC->PlayerHUDWidget->ShowMessageWithDuration(
				NSLOCTEXT("HUD", "DawnMessage", "날이 밝아옵니다!"),
				3.0f,
				true,
				FSimpleDelegate::CreateLambda([this, OpenResultPanelOnce]()
				{
					if (!GetWorld()) return;
					if (bGameEnded) return;

					if (MonsterSpawner) MonsterSpawner->NotifyTimeExpired();
					OpenResultPanelOnce();
				})
			);
		}
	}

	if (NightAudioComponent && NightAudioComponent->IsPlaying())
	{
		NightAudioComponent->FadeOut(DawnDuration, 0.0f);
	}
}

//  Spawner에서 Round(=CurrentDay) 웨이브 묶음이 끝났을 때 호출됨
void APotatoGameMode::HandleRoundFinished(int32 Round)
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleRoundFinished Round=%d CurrentDay=%d"), Round, CurrentDay);

	if (bGameEnded) return;

	if (Round != CurrentDay)
	{
		return;
	}

	//  웨이브가 끝났을 때도 Victory 먼저 체크해서 Dawn/ResultPhase 스킵 방지
	if (CheckVictoryCondition())
	{
		EndGame(true);
		return;
	}

	//  "진짜로" Night 타이머 끊고 Dawn(=ResultPhase)으로 즉시 스킵
	if (DayNightSystem)
	{
		DayNightSystem->ForceToDawn(true); // 또는 DayNightSystem->SkipToPhase(EDayPhase::Dawn, true);
	}
	else
	{
		// fallback (혹시 DayNightSystem이 아직 null이면)
		StartResultPhase();
	}
}

void APotatoGameMode::EndGame(bool IsGameClear, FText Message)
{
	if (bGameEnded) return;
	bGameEnded = true;

	// ResultPanel 보험 타이머가 남아있으면 즉시 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TH_ResultPanel);
	}

	if (MonsterSpawner)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Stop Wave!")));
		MonsterSpawner->NotifyGameOver();
	}

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

	// (선택) DayNightSystem이 계속 tick/타이머를 돌린다면 여기서 Stop 함수가 있으면 호출 추천
	// if (DayNightSystem) DayNightSystem->StopSystem();
}

bool APotatoGameMode::CheckVictoryCondition()
{
	return (CurrentDay >= 10);
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