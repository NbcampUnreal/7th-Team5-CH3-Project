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

	if (Button_GoToMainMenu)
	{
		Button_GoToMainMenu->OnClicked.AddDynamic(this, &UGameOverScreen::OnRestartClicked);
	}
	if (Button_GoToMainMenu_1)
	{
		Button_GoToMainMenu_1->OnClicked.AddDynamic(this, &UGameOverScreen::OnGoToTitleClicked);
	}
	if (Button_GoToMainMenu_3)
	{
		Button_GoToMainMenu_3->OnClicked.AddDynamic(this, &UGameOverScreen::OnVictoryTitleClicked);
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

		if (EliminatedDay_2)  EliminatedDay_2->SetText(DayText);
		if (EliminatedDay_3)  EliminatedDay_3->SetText(KillText);

		RefreshFinalAssets();
	}
	else
	{
		if (VictoryPanel)  VictoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (DefeatPanel)   DefeatPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		if (EliminatedDay)   EliminatedDay->SetText(DayText);
		if (EliminatedDay_1) EliminatedDay_1->SetText(KillText);
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
	// TODO: 실제 메인 메뉴 레벨 이름으로 교체하세요.
	UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
}

void UGameOverScreen::OnVictoryTitleClicked()
{
	// TODO: 실제 메인 메뉴 레벨 이름으로 교체하세요.
	UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
}
