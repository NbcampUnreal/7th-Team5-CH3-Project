// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "ResultScreen.generated.h"

class UTextBlock;
class UButton;
class UBorder;
class UHorizontalBox;
class UStackItem;

/**
 * WBP_ResultScreen
 * Day 생존 결과 화면 — 전투 통계 / 획득 보상 / 유지비 소모 / 계속 버튼
 *
 * StackItem들은 WBP 인스턴스(bIsVariable=False)이므로 C++ BindWidget 불가.
 * Border_0 / Border 전체 위젯 참조를 통해 동적 생성으로 채워넣을 수 있습니다.
 */
UCLASS()
class POTATOPROJECT_API UResultScreen : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/**
	 * 결과 화면을 초기화합니다.
	 * @param InCurrentDay       생존한 Day 번호
	 * @param InKilledMonster    처치한 일반 몬스터 수
	 * @param InKilledElite      처치한 정예 몬스터 수
	 * @param InKilledBoss       처치한 보스 몬스터 수
	 * @param InRewardItems      보상 배열 (EIconItemType, 수량)
	 * @param InCostItems        소모 배열 (EIconItemType, 수량)
	 */
	void InitScreen(
		int32 InCurrentDay,
		int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss,
		const TArray<TPair<EIconItemType, int32>>& InRewardItems,
		const TArray<TPair<EIconItemType, int32>>& InCostItems);

	// 화면 닫기 — 버튼 클릭 / Day 시작 자동 닫기 공통 경로
	UFUNCTION()
	void CloseScreen();

	// ---- BindWidgets ----

	/** "Day N 생존 성공!" 타이틀 텍스트 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> RemainingTime;

	/** 처치한 일반 몬스터 수 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> KilledMonster;

	/** 처치한 정예 몬스터 수 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> KilledElite;

	/** 처치한 보스 몬스터 수 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> KilledBoss;

	/** "계속" 버튼 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UButton> Button_Continue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UHorizontalBox> RewardBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UHorizontalBox> CostBox;

    // StackItemClass: WBP_StackItem의 UClass 참조 (에디터에서 설정)
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UStackItem> StackItemClass;

private:
	UFUNCTION()
	void OnContinueClicked();
};
