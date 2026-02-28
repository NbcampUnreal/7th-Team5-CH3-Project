// Source/PotatoProject/Monster/Utils/PotatoHitOnceUtils.h
#pragma once

#include "CoreMinimal.h"

class AActor;

// ------------------------------------------------------------
// HitOnce helpers (TArray<AActor*> 기반)
// ------------------------------------------------------------

static FORCEINLINE bool IsValidVictim(AActor* A)
{
	return IsValid(A) && !A->IsActorBeingDestroyed();
}

static FORCEINLINE void CompactHitOnceList(TArray<AActor*>& List)
{
	List.RemoveAll([](AActor* A)
	{
		return !IsValidVictim(A);
	});
}

static FORCEINLINE bool HasHitAlready(const TArray<AActor*>& List, AActor* Victim)
{
	if (!IsValidVictim(Victim)) return true;
	for (AActor* A : List)
	{
		if (A == Victim) return true;
	}
	return false;
}

static FORCEINLINE void MarkHit(TArray<AActor*>& List, AActor* Victim)
{
	if (!IsValidVictim(Victim)) return;
	if (!HasHitAlready(List, Victim))
	{
		List.Add(Victim);
	}
}