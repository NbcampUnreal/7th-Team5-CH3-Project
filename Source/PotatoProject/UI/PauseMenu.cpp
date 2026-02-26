// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PauseMenu.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PotatoPlayerController.h"

void UPauseMenu::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_Start)        Button_Start->OnClicked.AddDynamic(this, &UPauseMenu::OnResumeClicked);
    if (Button_RestartGame)   Button_RestartGame->OnClicked.AddDynamic(this, &UPauseMenu::OnRestartGameClicked);
    if (Button_GoToMainMenu) Button_GoToMainMenu->OnClicked.AddDynamic(this, &UPauseMenu::OnGoToMainMenuClicked);
    if (ButtonYes)           ButtonYes->OnClicked.AddDynamic(this, &UPauseMenu::OnDialogYesClicked);
    if (ButtonNo)            ButtonNo->OnClicked.AddDynamic(this, &UPauseMenu::OnDialogNoClicked);

    // 시작 시 다이얼로그 숨김
    ShowDialog(false);
    SetIsFocusable(true);
}

void UPauseMenu::NativeDestruct()
{
    if (Button_Start)        Button_Start->OnClicked.RemoveDynamic(this, &UPauseMenu::OnResumeClicked);
    if (Button_RestartGame)   Button_RestartGame->OnClicked.RemoveDynamic(this, &UPauseMenu::OnRestartGameClicked);
    if (Button_GoToMainMenu) Button_GoToMainMenu->OnClicked.RemoveDynamic(this, &UPauseMenu::OnGoToMainMenuClicked);
    if (ButtonYes)           ButtonYes->OnClicked.RemoveDynamic(this, &UPauseMenu::OnDialogYesClicked);
    if (ButtonNo)            ButtonNo->OnClicked.RemoveDynamic(this, &UPauseMenu::OnDialogNoClicked);

    Super::NativeDestruct();
}

FReply UPauseMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        // 다이얼로그 열려있으면 다이얼로그를 닫음
        if (DialogPanel && DialogPanel->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
        {
            OnDialogNoClicked();
            return FReply::Handled();
        }

        OnResumeClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// 게임 재개
void UPauseMenu::OnResumeClicked()
{
    APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetOwningPlayer());
    if (PlayerController)
    {
        PlayerController->SetPause(false);   // 게임 일시정지 해제
        PlayerController->SetUIMode(false);  // 게임 모드로 전환
    }

    RemoveFromParent();
}

// "게임 재시작" 버튼 -> 확인 다이얼로그 표시
void UPauseMenu::OnRestartGameClicked()
{
    PendingDialogAction = [this]()
    {
        APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetOwningPlayer());
        if (PlayerController)
        {
            PlayerController->SetUIMode(false);  // 게임 모드로 전환
        }
        UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this));
    };

    SetMessageText(FText::FromString(TEXT("Day 1부터 다시 시작하시겠습니까?")));
    ShowDialog(true);
}

// 메인 메뉴로
void UPauseMenu::OnGoToMainMenuClicked()
{
    PendingDialogAction = [this]()
    {
        APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetOwningPlayer());
        if (PlayerController)
        {
            PlayerController->SetUIMode(false);  // 게임 모드로 전환
        }
         UGameplayStatics::OpenLevel(this, TEXT("Potato_TitleLevel"));
    };

    SetMessageText(FText::FromString(TEXT("메인 메뉴로 돌아가시겠습니까?\n현재 진행 상황이 저장되지 않습니다.")));
    ShowDialog(true);
}

// 다이얼로그 "예" - 현재 레벨 재로드
void UPauseMenu::OnDialogYesClicked()
{
    if (PendingDialogAction)
    {
        PendingDialogAction();
        PendingDialogAction = nullptr;  // 실행 후 초기화
    }
}

// 다이얼로그 "아니오" - 다이얼로그 닫기
void UPauseMenu::OnDialogNoClicked()
{
    PendingDialogAction = nullptr;  // 취소 시 대기 중인 액션 초기화
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

void UPauseMenu::SetMessageText(const FText& InText)
{
    if (DialogText)
    {
        DialogText->SetText(InText);
    }
}
