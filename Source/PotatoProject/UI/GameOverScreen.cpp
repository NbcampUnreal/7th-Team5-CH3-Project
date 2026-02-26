// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GameOverScreen.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

#include "Core/PotatoResourceManager.h"
#include "Core/PotatoEnums.h"

void UGameOverScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Restart)
	{
		Button_Restart->OnClicked.AddDynamic(this, &UGameOverScreen::OnRestartClicked);
	}
	if (Button_GoToTitle)
	{
		Button_GoToTitle->OnClicked.AddDynamic(this, &UGameOverScreen::OnGoToTitleClicked);
	}
	if (Button_VictoryBackTitle)
	{
		Button_VictoryBackTitle->OnClicked.AddDynamic(this, &UGameOverScreen::OnVictoryTitleClicked);
	}

	// 초기에는 둘 다 숨김 (InitScreen 호출 전)
	if (VictoryPanel)  VictoryPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (DefeatPanel)   DefeatPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UGameOverScreen::InitScreen(bool bVictory, int32 InCurrentDay, int32 InKillCount)
{
	const FText DayText  = FText::Format(NSLOCTEXT("GameOver", "Day",  "Day {0}"), InCurrentDay);
	const FText KillText = FText::AsNumber(InKillCount);

	if (bVictory)
	{
		if (DefeatPanel)   DefeatPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (VictoryPanel)  VictoryPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (LastDay)  LastDay->SetText(DayText);
		if (TotalKilledEnemyVictory)  TotalKilledEnemyVictory->SetText(KillText);

		RefreshFinalAssets();
	}
	else
	{
		if (VictoryPanel)  VictoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (DefeatPanel)   DefeatPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (EliminatedDay)   EliminatedDay->SetText(DayText);
		if (TotalKilledEnemyCount) TotalKilledEnemyCount->SetText(KillText);
	}
}

void UGameOverScreen::RefreshFinalAssets()
{
	// TODO: ResourceManager에서 현재 자산을 읽어와 표시합니다.
	UPotatoResourceManager* RM = GetWorld()->GetSubsystem<UPotatoResourceManager>();
	if (!RM) return;

	auto SetAmount = [](UTextBlock* TB, int32 Val)
	{
		if (TB) TB->SetText(FText::AsNumber(Val));
	};

	SetAmount(WoodAmount,      RM->GetResource(EResourceType::Wood));
	SetAmount(StoneAmount,     RM->GetResource(EResourceType::Stone));
	SetAmount(CropAmount,      RM->GetResource(EResourceType::Crop));
	SetAmount(LivestockAmount, RM->GetResource(EResourceType::Livestock));
}

void UGameOverScreen::OnRestartClicked()
{
	// 현재 레벨을 다시 로드합니다.
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this));
	}
}

void UGameOverScreen::OnGoToTitleClicked()
{
	UGameplayStatics::OpenLevel(this, TEXT("Potato_TitleLevel"));
}

void UGameOverScreen::OnVictoryTitleClicked()
{
    UGameplayStatics::OpenLevel(this, TEXT("Potato_TitleLevel"));
}
