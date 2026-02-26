#pragma once

#include "CoreMinimal.h"

struct FPotatoMonsterSpecialSkillPresetRow;
class USpecialSkillComponent;
class AActor;

struct POTATOPROJECT_API FSpecialSkillPresentation
{
	static void PlayTelegraph(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	static void PlayCast(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	static void PlayExecute(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	static void PlayEnd(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
};