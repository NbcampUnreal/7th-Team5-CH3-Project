#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PotatoDialogueData.h"
#include "PotatoDialogueWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class POTATOPROJECT_API UPotatoDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StarDialogue(const FPotatoDialogueRow* DialogueRow);
	bool AdvanceDialogue();
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> SpeakerIcon;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> DialogueText;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopAnimation;
	
private:
	void UpdateUI();
	
	TArray<FText> CurrentLines;
	int32 CurrentLineIndex = 0;
};
