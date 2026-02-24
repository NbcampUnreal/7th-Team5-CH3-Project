// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultScreen.generated.h"

class UTextBlock;
class UButton;
class UBorder;

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

public:
	/**
	 * 결과 화면을 초기화합니다.
	 * @param InCurrentDay       생존한 Day 번호
	 * @param InKilledMonster    처치한 일반 몬스터 수
	 * @param InKilledElite      처치한 정예 몬스터 수
	 * @param InKilledBoss       처치한 보스 몬스터 수
	 */
	UFUNCTION(BlueprintCallable, Category = "ResultScreen")
	void InitScreen(int32 InCurrentDay, int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss);

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

	/**
	 * 획득 보상 영역 Border (내부에 WBP_StackItem x5 배치)
	 * TODO: StackItem 동적 생성이 필요한 경우 이 Border의 Content를 HorizontalBox로
	 *       교체하거나, WBP에서 직접 StackItem을 bIsVariable=True로 변경하세요.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> GainReward;

	/**
	 * 유지비 소모 영역 Border (내부에 WBP_StackItem x5 배치)
	 * TODO: 위와 동일
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> ConsumeCost;

private:
	UFUNCTION()
	void OnContinueClicked();   // Button_GoToMainMenu → 다음 Day로 계속
};
