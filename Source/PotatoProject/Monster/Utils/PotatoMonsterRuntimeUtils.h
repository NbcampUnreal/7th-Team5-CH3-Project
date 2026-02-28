#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"

class APotatoMonster;
class UAnimInstance;
class USkeletalMeshComponent;
class UCapsuleComponent;
class UCharacterMovementComponent;

// ------------------------------------------------------------
// Monster Runtime helpers
// - Capsule radius, AnimInstance, movement disable
// - Safe timer scheduling for APotatoMonster member functions
// - Bounds top location
// ------------------------------------------------------------

float GetMonsterCapsuleRadiusSafe(const APotatoMonster* M, float Fallback = 34.f);

UAnimInstance* GetAnimInstanceSafe(APotatoMonster* M);

void DisableMovementSafe(APotatoMonster* M);

bool ScheduleTimerSafe(
	APotatoMonster* Obj,
	FTimerHandle& Handle,
	void (APotatoMonster::*Func)(),
	float DelaySec
);

FVector ComputeBoundsTopLocation(APotatoMonster* M, float ZMul, float ZAdd);