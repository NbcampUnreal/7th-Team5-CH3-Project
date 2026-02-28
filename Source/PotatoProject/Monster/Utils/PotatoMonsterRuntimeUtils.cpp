#include "Monster/Utils/PotatoMonsterRuntimeUtils.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

#include "../PotatoMonster.h"

float GetMonsterCapsuleRadiusSafe(const APotatoMonster* M, float Fallback)
{
	if (!M) return Fallback;

	if (const UCapsuleComponent* Cap = M->FindComponentByClass<UCapsuleComponent>())
	{
		return Cap->GetScaledCapsuleRadius();
	}
	return Fallback;
}

UAnimInstance* GetAnimInstanceSafe(APotatoMonster* M)
{
	if (!M) return nullptr;
	USkeletalMeshComponent* Skel = M->GetMesh();
	return Skel ? Skel->GetAnimInstance() : nullptr;
}

void DisableMovementSafe(APotatoMonster* M)
{
	if (!M) return;

	if (UCharacterMovementComponent* MoveComp = M->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

bool ScheduleTimerSafe(
	APotatoMonster* Obj,
	FTimerHandle& Handle,
	void (APotatoMonster::*Func)(),
	float DelaySec
)
{
	if (!IsValid(Obj) || Obj->IsActorBeingDestroyed())
		return false;

	// NaN/Inf/음수 방어
	if (!FMath::IsFinite(DelaySec) || DelaySec < 0.f)
	{
		DelaySec = 0.f;
	}

	UWorld* W = Obj->GetWorld();
	if (!W || W->bIsTearingDown)
		return false;

	FTimerManager& TM = W->GetTimerManager();
	TM.ClearTimer(Handle);

	// 0초는 즉시 호출(가장 안전)
	if (DelaySec <= 0.f)
	{
		(Obj->*Func)();
		return true;
	}

	// UObject + 멤버함수 오버로드(안정)
	TM.SetTimer(Handle, Obj, Func, DelaySec, false);
	return true;
}

FVector ComputeBoundsTopLocation(APotatoMonster* M, float ZMul, float ZAdd)
{
	if (!M) return FVector::ZeroVector;

	if (USkeletalMeshComponent* Mesh = M->GetMesh())
	{
		const FVector Origin = Mesh->Bounds.Origin;
		const float Z = Mesh->Bounds.BoxExtent.Z * ZMul + ZAdd;
		return Origin + FVector(0.f, 0.f, Z);
	}

	if (UCapsuleComponent* Cap = M->GetCapsuleComponent())
	{
		return M->GetActorLocation() + FVector(0.f, 0.f, Cap->GetScaledCapsuleHalfHeight() * ZMul + ZAdd);
	}

	return M->GetActorLocation();
}