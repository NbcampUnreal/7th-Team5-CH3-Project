// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverScreen.generated.h"

class UCanvasPanel;
class UTextBlock;
class UButton;

/**
 * WBP_GameOverScreen
 * 
 * VictoryPanel  - THE END  : 최종 생존일, 총 처치 수, 최종 자산(Wood/Stone/Crop/Livestock)
 * DefeatPanel   - GAME OVER: 최종 생존일, 총 처치 수, 다시 시작 / 타이틀로 버튼
 */
UCLASS()
class POTATOPROJECT_API UGameOverScreen : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	/** 결과 화면을 초기화합니다.
	 *  @param bVictory      클리어 여부
	 *  @param InCurrentDay  최종 생존 Day
	 *  @param InKillCount   총 처치 수
	 */
	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void InitScreen(bool bVictory, int32 InCurrentDay, int32 InKillCount);

	// ---- DefeatPanel BindWidgets ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCanvasPanel> DefeatPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> GameOverMessage;       // "창고가 몬스터들에게 남김없이 약탈당했습니다..."

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> EliminatedDay;         // 최종 생존일 (DefeatPanel)

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> TotalKilledEnemyCount;       // 총 처치 수 (DefeatPanel)

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UButton> Button_Restart;      // 다시 시작

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UButton> Button_GoToTitle;    // 타이틀로

	// ---- VictoryPanel BindWidgets ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCanvasPanel> VictoryPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> VictoryMessage;     // "농부의 노력으로 마침내 마을은 평화를 얻었습니다!"

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> LastDay;       // 최종 생존일 (VictoryPanel)

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> TotalKilledEnemyVictory;       // 총 처치 수 (VictoryPanel)

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> WoodAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> StoneAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> CropAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> LivestockAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UButton> Button_VictoryBackTitle;    // 타이틀로 (VictoryPanel)

private:
	UFUNCTION()
	void OnRestartClicked();       // 현재 레벨 재시작

	UFUNCTION()
	void OnGoToTitleClicked();     // 타이틀 이동 (DefeatPanel)

	UFUNCTION()
	void OnVictoryTitleClicked();  // 타이틀 이동 (VictoryPanel)

	void RefreshFinalAssets();     // VictoryPanel 자산 텍스트 갱신
};
