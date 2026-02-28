#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Story/PotatoDialogueData.h"
#include "PotatoDialogueWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class POTATOPROJECT_API UPotatoDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartDialogue(const FPotatoDialogueRow* DialogueRow);
	bool AdvanceDialogue();
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> SpeakerIcon;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> DialogueText;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopAnimation;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Settings")
	float AutoAdvanceTime = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Settings")
	TMap<EDialogueSpeaker, TObjectPtr<UTexture2D>> SpeakerPortraits;
	
private:
	void UpdateUI();
	void TriggerAutoAdvance();
	
	TArray<FPotatoDialogueLine> CurrentLines;
	int32 CurrentLineIndex = 0;
	
	FTimerHandle AutoAdvanceTimerHandle;
};
