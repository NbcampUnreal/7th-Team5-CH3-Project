#include "PotatoFXUtils.h"

#include "../PotatoMonsterAnimSet.h" 

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

// VFX
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"

// bounds access
#include "../PotatoMonster.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

// ------------------------------
// Global history per channel
// ------------------------------
static TArray<float> G_HitSFXTimes;
static TArray<float> G_DeathSFXTimes;
static TArray<float> G_HitVFXTimes;
static TArray<float> G_DeathVFXTimes;
static TArray<float> G_CombatSFXTimes;

static TArray<float>& GetTimesByChannel(PotatoFX::EGlobalBudgetChannel Channel)
{
	switch (Channel)
	{
	case PotatoFX::EGlobalBudgetChannel::HitSFX:   return G_HitSFXTimes;
	case PotatoFX::EGlobalBudgetChannel::DeathSFX: return G_DeathSFXTimes;
	case PotatoFX::EGlobalBudgetChannel::HitVFX:   return G_HitVFXTimes;
	case PotatoFX::EGlobalBudgetChannel::CombatSFX: return G_CombatSFXTimes;
	default:                                      return G_DeathVFXTimes;
	}
}

// ✅ 간섭 제거 버전: Now 역전(PIE 리스타트) 보호를 "Times 배열" 기준으로만 처리
static bool PassGlobalBudgetImpl(float Now, float WindowSec, int32 MaxCount, TArray<float>& Times)
{
	if (WindowSec <= 0.f || MaxCount <= 0)
	{
		return true;
	}

	// PIE/월드 재시작 등으로 시간이 역전되면 해당 히스토리만 리셋
	if (Times.Num() > 0 && Now + 0.01f < Times.Last())
	{
		Times.Reset();
	}

	for (int32 i = Times.Num() - 1; i >= 0; --i)
	{
		if (Now - Times[i] > WindowSec)
		{
			Times.RemoveAtSwap(i, 1, false);
		}
	}

	if (Times.Num() >= MaxCount)
	{
		return false;
	}

	Times.Add(Now);
	return true;
}

namespace PotatoFX
{
	// ------------------------------------------------------------
	// Distance gate
	// ------------------------------------------------------------
	float ComputeDistanceChance(float Dist, float NearDist, float FarDist, float FarChance)
	{
		NearDist = FMath::Max(0.f, NearDist);
		FarDist = FMath::Max(NearDist + 1.f, FarDist);
		FarChance = FMath::Clamp(FarChance, 0.f, 1.f);

		if (Dist <= NearDist) return 1.0f;
		if (Dist >= FarDist)  return FarChance;

		const float Alpha = (Dist - NearDist) / (FarDist - NearDist);
		return FMath::Lerp(1.0f, FarChance, Alpha);
	}

	bool PassDistanceGate(const UObject* WorldContextObject, const FVector& Loc,
		float NearDist, float FarDist, float FarChance)
	{
		if (!WorldContextObject) return false;

		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
		if (!PlayerPawn) return true;

		const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), Loc);
		const float Chance = ComputeDistanceChance(Dist, NearDist, FarDist, FarChance);
		return FMath::FRand() <= Chance;
	}

	// ------------------------------------------------------------
	// Global budget
	// ------------------------------------------------------------
	bool PassGlobalBudget(float Now, float WindowSec, int32 MaxCount, EGlobalBudgetChannel Channel)
	{
		TArray<float>& Times = GetTimesByChannel(Channel);
		return PassGlobalBudgetImpl(Now, WindowSec, MaxCount, Times);
	}

	// ------------------------------------------------------------
	// SFX spawn
	// ------------------------------------------------------------
	void SpawnSFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoSFXSlot& Slot, const FVector& Location)
	{
		if (!WorldContextObject) return;
		if (!Slot.Sound) return;

		const float Volume = FMath::Max(0.f, Slot.VolumeMultiplier);
		const float Pitch = FMath::Max(0.01f, Slot.PitchMultiplier);

		UGameplayStatics::SpawnSoundAtLocation(
			WorldContextObject,
			Slot.Sound,
			Location,
			FRotator::ZeroRotator,
			Volume,
			Pitch,
			0.0f,
			Slot.Attenuation,
			Slot.Concurrency,
			true
		);
	}

	// ------------------------------------------------------------
	// VFX auto scale (per-slot)
	// ------------------------------------------------------------
	float ComputeVFXSlotAutoScaleScalar(APotatoMonster* Monster, const FPotatoVFXSlot& Slot)
	{
		if (!Monster) return 1.f;
		if (!Slot.bAutoScale) return 1.f;

		float Ext = 0.f;

		if (USkeletalMeshComponent* Mesh = Monster->GetMesh())
		{
			const FVector E = Mesh->Bounds.BoxExtent;
			Ext = Slot.bUseMaxExtentXYZ ? FMath::Max3(E.X, E.Y, E.Z) : E.Z;
		}
		else if (UCapsuleComponent* Cap = Monster->GetCapsuleComponent())
		{
			Ext = Cap->GetScaledCapsuleHalfHeight();
		}
		else
		{
			return 1.f;
		}

		const float Ref = FMath::Max(1.f, Slot.RefExtent);
		const float S = Ext / Ref;

		const float MinS = FMath::Max(0.01f, Slot.AutoScaleMin);
		const float MaxS = FMath::Max(MinS, Slot.AutoScaleMax);

		return FMath::Clamp(S, MinS, MaxS);
	}

	// ------------------------------------------------------------
	// VFX spawn (AtLocation only) - UE5.6 safe overload fixed
	// ------------------------------------------------------------
	void SpawnVFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoVFXSlot& Slot,
		const FVector& LocationWS, const FRotator& RotationWS, const FVector& FinalScale)
	{
		if (!WorldContextObject) return;

		if (Slot.Niagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				WorldContextObject,
				Slot.Niagara,
				LocationWS,
				RotationWS,
				FinalScale,
				true, true,
				ENCPoolMethod::AutoRelease
			);
			return;
		}

		if (Slot.Cascade)
		{
			UWorld* World = WorldContextObject->GetWorld();
			if (!World) return;

			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				Slot.Cascade,
				LocationWS,
				RotationWS,
				FinalScale,
				true,
				EPSCPoolMethod::AutoRelease
			);
			(void)PSC;
			return;
		}
	}
}