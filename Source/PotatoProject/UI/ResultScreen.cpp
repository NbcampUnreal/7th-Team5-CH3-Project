// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ResultScreen.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Core/PotatoDayRewardRow.h"
#include "Core/PotatoGameMode.h"
#include "Player/PotatoPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/StackItem.h"

void UResultScreen::NativeConstruct()
{
	Super::NativeConstruct();

    SetIsFocusable(true);

	if (Button_Continue)
	{
		Button_Continue->OnClicked.AddDynamic(this, &UResultScreen::OnContinueClicked);
	}
}

void UResultScreen::InitScreen(
    int32 InCurrentDay,
    int32 InKilledMonster, int32 InKilledElite, int32 InKilledBoss,
    const TArray<TPair<EIconItemType, int32>>& InRewardItems,
    const TArray<TPair<EIconItemType, int32>>& InCostItems)
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
    
    auto DrawStackItems = [this](UHorizontalBox* Box, const TArray<TPair<EIconItemType, int32>>& Items, EStackItemType PlusMinus)
    {
        if (!Box || !StackItemClass)
        {
            return;
        }

        Box->ClearChildren();
        for (const TPair<EIconItemType, int32>& Item : Items)
        {
            UStackItem* ItemWidget = CreateWidget<UStackItem>(this, StackItemClass);
            if (ItemWidget)
            {
                ItemWidget->SetStackData(Item.Key, Item.Value, PlusMinus);
                Box->AddChildToHorizontalBox(ItemWidget);
            }
        }
    }; 

    DrawStackItems(RewardBox, InRewardItems, EStackItemType::Plus);
    DrawStackItems(CostBox, InCostItems, EStackItemType::Minus);
}

void UResultScreen::CloseScreen()
{
	if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetOwningPlayer()))
	{
		PC->SetUIMode(false);
	}
	RemoveFromParent();
}

void UResultScreen::OnContinueClicked()
{
	CloseScreen();
}

void UResultScreen::NativeDestruct()
{
    // OnDayPhase에 바인딩된 CloseScreen 델리게이트 해제
    if (APotatoGameMode* GM = GetWorld()->GetAuthGameMode<APotatoGameMode>())
    {
        GM->OnDayPhase.RemoveDynamic(this, &UResultScreen::CloseScreen);
    }

    Super::NativeDestruct();
}
