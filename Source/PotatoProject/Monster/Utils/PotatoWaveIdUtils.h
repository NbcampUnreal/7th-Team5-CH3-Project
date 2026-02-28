// Source/PotatoProject/Spawner/Utils/PotatoWaveIdUtils.h
#pragma once

#include "CoreMinimal.h"

// ------------------------------------------------------------
// Stage / WaveId helpers
// - "3"   -> StageOnly (Stage=3)
// - "3-2" -> StageWaveId (Stage=3, Sub=2)
// ------------------------------------------------------------

static FORCEINLINE bool IsStageOnlyName(const FName& InName, int32& OutStage)
{
	const FString S = InName.ToString().TrimStartAndEnd();
	if (S.Contains(TEXT("-"))) return false;
	if (!S.IsNumeric()) return false;

	OutStage = FCString::Atoi(*S);
	return OutStage > 0;
}

static FORCEINLINE FName MakeStageWaveId(int32 Stage, int32 Sub)
{
	return FName(*FString::Printf(TEXT("%d-%d"), Stage, Sub));
}