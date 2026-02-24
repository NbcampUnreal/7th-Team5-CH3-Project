// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ResultScreen.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"

void UResultScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UResultScreen::OnContinueClicked);
	}
}

void UResultScreen::InitScreen(int32 InCurrentDay, int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss)
{
	// "Day N 생존 성공!" 타이틀
	if (RemainingTime)
	{
		RemainingTime->SetText(
			FText::Format(NSLOCTEXT("ResultScreen", "DayClear", "Day {0} 생존 성공!"), InCurrentDay)
		);
	}

	// 전투 통계
	if (KilledMonster)
	{
		KilledMonster->SetText(FText::AsNumber(InKilledMonster));
	}
	if (KilledElite)
	{
		KilledElite->SetText(FText::AsNumber(InKilledElite));
	}
	if (KilledBoss)
	{
		KilledBoss->SetText(FText::AsNumber(InKilledBoss));
	}

	// TODO: Border_0 / Border 내부 StackItem들에 보상/유지비 데이터 설정
	// WBP에서 StackItem 인스턴스들이 bIsVariable=False이므로 현재 C++에서 직접 접근 불가.
	// 방법 1) WBP 에디터에서 각 WBP_StackItem의 bIsVariable=True로 변경 후 BindWidget 추가
	// 방법 2) 이 함수에서 동적으로 UStackItem 위젯을 생성하여 HorizontalBox에 AddChild
}

void UResultScreen::OnContinueClicked()
{
	// TODO: GameMode에 다음 Day 진행 요청 (예: StartDayPhase 호출)
	// APotatoGameMode* GM = Cast<APotatoGameMode>(UGameplayStatics::GetGameMode(this));
	// if (GM) GM->StartDayPhase();
	RemoveFromParent();
}
