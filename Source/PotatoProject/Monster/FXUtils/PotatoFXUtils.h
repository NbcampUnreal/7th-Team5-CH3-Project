#pragma once

#include "CoreMinimal.h"

// forward declarations
class APotatoMonster;

struct FPotatoSFXSlot;
struct FPotatoVFXSlot;

namespace PotatoFX
{
	// ------------------------------------------------------------
	// Distance gate
	// ------------------------------------------------------------
	float ComputeDistanceChance(float Dist, float NearDist, float FarDist, float FarChance);

	bool PassDistanceGate(const UObject* WorldContextObject, const FVector& Loc,
		float NearDist, float FarDist, float FarChance);

	// ------------------------------------------------------------
	// Global budget channels (SFX/VFX separated)
	// ------------------------------------------------------------
	enum class EGlobalBudgetChannel : uint8
	{
		HitSFX,
		DeathSFX,
		HitVFX,
		DeathVFX,
		CombatSFX
	};

	bool PassGlobalBudget(float Now, float WindowSec, int32 MaxCount, EGlobalBudgetChannel Channel);

	// ------------------------------------------------------------
	// SFX spawn
	// ------------------------------------------------------------
	void SpawnSFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoSFXSlot& Slot, const FVector& Location);

	// ------------------------------------------------------------
	// VFX auto scale (per-slot)
	// ------------------------------------------------------------
	float ComputeVFXSlotAutoScaleScalar(APotatoMonster* Monster, const FPotatoVFXSlot& Slot);

	// ------------------------------------------------------------
	// VFX spawn (AtLocation only)
	// FinalScale = Slot.Scale * Scalar
	// ------------------------------------------------------------
	void SpawnVFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoVFXSlot& Slot,
		const FVector& LocationWS, const FRotator& RotationWS, const FVector& FinalScale);
}