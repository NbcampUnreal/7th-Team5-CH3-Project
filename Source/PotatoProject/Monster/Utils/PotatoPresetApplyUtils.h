#pragma once

#include "CoreMinimal.h"

struct FPotatoMonsterSpecialSkillPresetRow;
struct FPotatoMonsterFinalStats;

// ------------------------------------------------------------
// Preset Apply helpers
// ------------------------------------------------------------
void ApplySplitFromSkillRow(
	const FPotatoMonsterSpecialSkillPresetRow* SkillRow,
	FPotatoMonsterFinalStats& OutStats,
	FName TypeDefaultSkillId
);