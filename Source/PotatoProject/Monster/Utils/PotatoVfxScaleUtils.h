#pragma once

#include "CoreMinimal.h"

// Niagara/VFX가 "월드 스케일"에 반응한다는 가정
static constexpr float kVfxBaseRadiusUU = 200.f;
static constexpr float kVfxMinScale = 0.2f;
static constexpr float kVfxMaxScale = 20.f;

static FORCEINLINE FVector ExplodeRadiusToVfxScale(float ExplodeRadius)
{
	const float Base = FMath::Max(1.f, kVfxBaseRadiusUU);
	const float Raw = ExplodeRadius / Base;
	const float S = FMath::Clamp(Raw, kVfxMinScale, kVfxMaxScale);
	return FVector(S);
}