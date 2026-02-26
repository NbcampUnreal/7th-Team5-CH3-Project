#pragma once

#include "CoreMinimal.h"

struct FPotatoMonsterSpecialSkillPresetRow;
class USpecialSkillComponent;
class AActor;

struct POTATOPROJECT_API FSpecialSkillExecution
{
	static void Execute(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& RowCopy, AActor* Target);
};