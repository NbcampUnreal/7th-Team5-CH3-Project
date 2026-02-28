#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PotatoDialogueData.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FPotatoDialogueRow : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UTexture2D> SpeakerIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = "true"))
	TArray<FText> DialogueLines;
};