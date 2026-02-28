#include "PotatoDialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPotatoDialogueWidget::StartDialogue(const FPotatoDialogueRow* DialogueRow)
{
	UE_LOG(LogTemp, Warning, TEXT("StartDialogue Called!"));
	
	if (!DialogueRow || DialogueRow->DialogueLines.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartDialogue Failed: Lines are empty!"));
		return;
	}
	
	CurrentLines = DialogueRow->DialogueLines;
	CurrentLineIndex = 0;
	
	UpdateUI();
	
	if (PopAnimation)
	{
		PlayAnimation(PopAnimation);
	}
	
	SetVisibility(ESlateVisibility::Visible);
	
	// 자동 진행 타이머 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AutoAdvanceTimerHandle,
			this,
			&UPotatoDialogueWidget::TriggerAutoAdvance,
			AutoAdvanceTime,
			false);
	}
}

bool UPotatoDialogueWidget::AdvanceDialogue()
{
	UWorld* World = GetWorld();
	// 중복 스킵 방지: 기존 타이머 클리어
	if (World)
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}
	
	
	CurrentLineIndex++;
	if (CurrentLineIndex >= CurrentLines.Num())
	{
		// TODO: 역방향 애니메이션 추가
		SetVisibility(ESlateVisibility::Hidden);
		return true;
	}
	
	UpdateUI();
	
	// 텍스트 변경 느낌?
	if (PopAnimation)
	{
		PlayAnimation(PopAnimation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.5f);
	}
	
	if (World)
	{
		World->GetTimerManager().SetTimer(
			AutoAdvanceTimerHandle,
			this,
			&UPotatoDialogueWidget::TriggerAutoAdvance,
			AutoAdvanceTime,
			false);
	}
	
	return false;
}

void UPotatoDialogueWidget::UpdateUI()
{
	if (CurrentLines.IsValidIndex(CurrentLineIndex))
	{
		const FPotatoDialogueLine& CurrentLine = CurrentLines[CurrentLineIndex];
		
		if (DialogueText)
		{
			DialogueText->SetText(CurrentLine.DialogueText);
		}
		
		if (SpeakerIcon)
		{
			if (TObjectPtr<UTexture2D>* FoundTexture = SpeakerPortraits.Find(CurrentLine.Speaker))
			{
				SpeakerIcon->SetBrushFromTexture(*FoundTexture);
				SpeakerIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				SpeakerIcon->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UPotatoDialogueWidget::TriggerAutoAdvance()
{
	AdvanceDialogue();
}
