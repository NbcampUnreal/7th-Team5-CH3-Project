// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PauseMenu.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UPauseMenu::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_Start)        Button_Start->OnClicked.AddDynamic(this, &UPauseMenu::OnResumeClicked);
    if (Button_DayRestart)   Button_DayRestart->OnClicked.AddDynamic(this, &UPauseMenu::OnDayRestartClicked);
    if (Button_GoToMainMenu) Button_GoToMainMenu->OnClicked.AddDynamic(this, &UPauseMenu::OnGoToMainMenuClicked);
    if (ButtonYes)           ButtonYes->OnClicked.AddDynamic(this, &UPauseMenu::OnDialogYesClicked);
    if (ButtonNo)            ButtonNo->OnClicked.AddDynamic(this, &UPauseMenu::OnDialogNoClicked);

    // 시작 시 다이얼로그 숨김
    ShowDialog(false);
}

void UPauseMenu::NativeDestruct()
{
    if (Button_Start)        Button_Start->OnClicked.RemoveDynamic(this, &UPauseMenu::OnResumeClicked);
    if (Button_DayRestart)   Button_DayRestart->OnClicked.RemoveDynamic(this, &UPauseMenu::OnDayRestartClicked);
    if (Button_GoToMainMenu) Button_GoToMainMenu->OnClicked.RemoveDynamic(this, &UPauseMenu::OnGoToMainMenuClicked);
    if (ButtonYes)           ButtonYes->OnClicked.RemoveDynamic(this, &UPauseMenu::OnDialogYesClicked);
    if (ButtonNo)            ButtonNo->OnClicked.RemoveDynamic(this, &UPauseMenu::OnDialogNoClicked);

    Super::NativeDestruct();
}

// 게임 재개
void UPauseMenu::OnResumeClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    PC->SetPause(false);
    PC->SetInputMode(FInputModeGameOnly());
    PC->SetShowMouseCursor(false);
    RemoveFromParent();
}

// "게임 재시작" 버튼 -> 확인 다이얼로그 표시
void UPauseMenu::OnDayRestartClicked()
{
    ShowDialog(true);
}

// 메인 메뉴로
void UPauseMenu::OnGoToMainMenuClicked()
{
    // 첫 번째 레벨(메인 메뉴 레벨 이름)로 이동 — 필요 시 레벨 이름 수정
    //UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
}

// 다이얼로그 "예" - 현재 레벨 재로드 (Day 재시작)
void UPauseMenu::OnDialogYesClicked()
{
    //UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this));
}

// 다이얼로그 "아니오" - 다이얼로그 닫기
void UPauseMenu::OnDialogNoClicked()
{
    ShowDialog(false);
}

// 다이얼로그 패널 표시/숨김 토글
void UPauseMenu::ShowDialog(bool bShow)
{
    if (DialogPanel)
    {
        DialogPanel->SetVisibility(
            bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }
}
