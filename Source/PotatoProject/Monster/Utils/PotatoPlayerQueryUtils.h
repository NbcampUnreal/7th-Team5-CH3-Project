#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"

class APawn;
class UWorld;
class APotatoMonster;

// ------------------------------------------------------------
// Player Query helpers
// ------------------------------------------------------------

static FORCEINLINE APawn* GetPlayerPawnSafe(UWorld* World)
{
	if (!World) return nullptr;
	APawn* P = UGameplayStatics::GetPlayerPawn(World, 0);
	return IsValid(P) ? P : nullptr;
}

static FORCEINLINE bool IsValidAttackablePlayer(const APotatoMonster* M, const APawn* PlayerPawn)
{
	if (!M || !IsValid(PlayerPawn)) return false;
	if (!PlayerPawn->CanBeDamaged()) return false;
	const APawn* MonsterPawn = Cast<APawn>(M);
	if (PlayerPawn == MonsterPawn)
	{
		return false;
	}
	return true;
}