// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenu.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;

UCLASS()
class POTATOPROJECT_API UPauseMenu : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    
    // 키 입력 가로채기
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // == PausePanel 위젯 ==================================
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCanvasPanel> PausePanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> Button_Start;           // 게임 재개

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> Button_RestartGame;      // Day 다시 시작 (다이얼로그 열기)

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> Button_GoToMainMenu;    // 메인 메뉴로

    // == DialogPanel 위젯 =================================
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCanvasPanel> DialogPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> DialogText;   // 확인 메시지

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> ButtonYes;              // 예 — Day 재시작 실행

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> ButtonNo;               // 아니오 — 다이얼로그 닫기

private:
    TFunction<void()> PendingDialogAction;  // "예" 클릭 시 실행할 함수

private:
    UFUNCTION()
    void OnResumeClicked();

    UFUNCTION()
    void OnRestartGameClicked();     // DialogPanel 열기

    UFUNCTION()
    void OnGoToMainMenuClicked();

    UFUNCTION()
    void OnDialogYesClicked();      // 실제 Day 재시작

    UFUNCTION()
    void OnDialogNoClicked();       // DialogPanel 닫기

    void ShowDialog(bool bShow);

    void SetMessageText(const FText& InText);
};
