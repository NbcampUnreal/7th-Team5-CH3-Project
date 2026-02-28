#include "Monster/Utils/PotatoPresetApplyUtils.h"
#include "Monster/PotatoMonsterFinalStats.h"
#include "Monster/PotatoMonsterSpecialSkillPresetRow.h"
#include "Core/PotatoEnums.h" 
void ApplySplitFromSkillRow(
	const FPotatoMonsterSpecialSkillPresetRow* SkillRow,
	FPotatoMonsterFinalStats& OutStats,
	FName TypeDefaultSkillId
)
{
	if (!SkillRow)
	{
		OutStats.bEnableSplit = false;
		OutStats.SplitSpec = FPotatoSplitSpec();
		return;
	}

	const bool bIsSplitLogic = (SkillRow->Logic == EMonsterSpecialLogic::Split);
	const bool bIsSplitExec  = (SkillRow->Execution == EMonsterSpecialExecution::SummonSplit);
	const bool bEnableSplit  = (SkillRow->bEnableSplit || bIsSplitLogic || bIsSplitExec);

	OutStats.bEnableSplit = bEnableSplit;

	if (bEnableSplit)
	{
		OutStats.SplitSpec.ThresholdPercents     = SkillRow->SplitThresholdPercents;
		OutStats.SplitSpec.MinMaxHpToAllowSplit  = SkillRow->SplitMinMaxHpToAllow;
		OutStats.SplitSpec.MaxDepth              = SkillRow->SplitMaxDepth;
		OutStats.SplitSpec.SpawnCount            = SkillRow->SplitSpawnCount;
		OutStats.SplitSpec.OwnerScaleMultiplier  = SkillRow->SplitOwnerScaleMultiplier;
		OutStats.SplitSpec.ChildMaxHpRatio       = SkillRow->SplitChildMaxHpRatio;
		OutStats.SplitSpec.SpawnJitterRadius     = SkillRow->SplitSpawnJitterRadius;
		OutStats.SplitSpec.SpawnZOffset          = SkillRow->SplitSpawnZOffset;

		// 안전: 퍼센트 비어있으면 최소 1개
		if (OutStats.SplitSpec.ThresholdPercents.Num() == 0)
		{
			OutStats.SplitSpec.ThresholdPercents = { 0.5f };
		}

		// 중요: Split은 Skill 시스템(타겟/쿨타임)으로 돌리지 않도록 차단
		OutStats.DefaultSpecialSkillId = NAME_None;
	}
	else
	{
		OutStats.SplitSpec = FPotatoSplitSpec();
		OutStats.DefaultSpecialSkillId = TypeDefaultSkillId;
	}
}