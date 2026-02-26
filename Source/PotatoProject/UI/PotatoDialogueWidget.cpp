#include "PotatoDialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPotatoDialogueWidget::StarDialogue(const FPotatoDialogueRow* DialogueRow)
{
	if (!DialogueRow || DialogueRow->DialogueLines.IsEmpty())
	{
		return;
	}
	
	CurrentLines = DialogueRow->DialogueLines;
	CurrentLineIndex = 0;
	
	if (DialogueRow->SpeakerIcon)
	{
		SpeakerIcon->SetBrushFromTexture(DialogueRow->SpeakerIcon);
		SpeakerIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		SpeakerIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	UpdateUI();
	
	if (PopAnimation)
	{
		PlayAnimation(PopAnimation);
	}
	
	SetVisibility(ESlateVisibility::Visible);	
}

bool UPotatoDialogueWidget::AdvanceDialogue()
{
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
	
	return false;
}

void UPotatoDialogueWidget::UpdateUI()
{
	if (DialogueText && CurrentLines.IsValidIndex(CurrentLineIndex))
	{
		DialogueText->SetText(CurrentLines[CurrentLineIndex]);
	}
}
