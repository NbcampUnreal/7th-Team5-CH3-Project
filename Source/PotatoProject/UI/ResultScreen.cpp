// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ResultScreen.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Core/PotatoDayRewardRow.h"
#include "Player/PotatoPlayerController.h"
#include "Kismet/GameplayStatics.h"

void UResultScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UResultScreen::OnContinueClicked);
	}
}

void UResultScreen::InitScreen(
	int32 InCurrentDay,
	int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss,
	int32 InWoodReward, int32 InStoneReward, int32 InCropReward, int32 InLivestockReward,
	int32 InTotalMaintenance,
	int32 InRetiredNPCCount)
{
	// "Day N 생존 성공!" 타이틀
	if (RemainingTime)
	{
		RemainingTime->SetText(
			FText::Format(NSLOCTEXT("ResultScreen", "DayClear", "Day {0} 생존 성공!"), InCurrentDay)
		);
	}

	// 전투 통계
	if (KilledMonster) KilledMonster->SetText(FText::AsNumber(InKilledMonster));
	if (KilledElite)   KilledElite->SetText(FText::AsNumber(InKilledElite));
	if (KilledBoss)    KilledBoss->SetText(FText::AsNumber(InKilledBoss));

	// GainReward Border — 보상 항목을 텍스트로 표시
	//if (GainReward)
	//{
	//	UHorizontalBox* RewardBox = NewObject<UHorizontalBox>(this);
	//	auto AddRewardEntry = [&](const FString& Label, int32 Value)
	//	{
	//		if (Value <= 0) return;
	//		UTextBlock* Text = NewObject<UTextBlock>(this);
	//		Text->SetText(FText::FromString(FString::Printf(TEXT("%s: %d  "), *Label, Value)));
	//		UHorizontalBoxSlot* Slot = RewardBox->AddChildToHorizontalBox(Text);
	//		Slot->SetPadding(FMargin(4.f, 0.f));
	//	};
	//	AddRewardEntry(TEXT("나무"), InWoodReward);
	//	AddRewardEntry(TEXT("광석"), InStoneReward);
	//	AddRewardEntry(TEXT("농작물"), InCropReward);
	//	AddRewardEntry(TEXT("축산물"), InLivestockReward);
	//	GainReward->SetContent(RewardBox);
	//}

	//// ConsumeCost Border — 유지비 + 퇴직 NPC 수 표시
	//if (ConsumeCost)
	//{
	//	UHorizontalBox* CostBox = NewObject<UHorizontalBox>(this);

	//	UTextBlock* MaintenanceText = NewObject<UTextBlock>(this);
	//	MaintenanceText->SetText(FText::FromString(FString::Printf(TEXT("유지비: %d  "), InTotalMaintenance)));
	//	UHorizontalBoxSlot* MSlot = CostBox->AddChildToHorizontalBox(MaintenanceText);
	//	MSlot->SetPadding(FMargin(4.f, 0.f));

	//	if (InRetiredNPCCount > 0)
	//	{
	//		UTextBlock* RetiredText = NewObject<UTextBlock>(this);
	//		RetiredText->SetText(FText::FromString(FString::Printf(TEXT("퇴직 NPC: %d"), InRetiredNPCCount)));
	//		UHorizontalBoxSlot* RSlot = CostBox->AddChildToHorizontalBox(RetiredText);
	//		RSlot->SetPadding(FMargin(4.f, 0.f));
	//	}

	//	ConsumeCost->SetContent(CostBox);
	//}
}

void UResultScreen::OnContinueClicked()
{
	if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetOwningPlayer()))
	{
		PC->SetUIMode(false);
	}

	RemoveFromParent();
}
