#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PotatoDialogueData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EDialogueSpeaker : uint8
{
	None,
	Player,
	Rat,
	Snake,
	Skeleton,
	Cactus,
	Mushroom,
	Stingray,
	Slime,
	TurtleShell,
	Boss
};

/** 대화 시퀀스에서의 하나의 대사 라인 */
USTRUCT(BlueprintType)
struct FPotatoDialogueLine
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EDialogueSpeaker Speaker = EDialogueSpeaker::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = "true"))
	FText DialogueText;
};

/** Data Table에 사용되는 Row Struct */
USTRUCT(BlueprintType)
struct FPotatoDialogueRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FPotatoDialogueLine> DialogueLines;
};