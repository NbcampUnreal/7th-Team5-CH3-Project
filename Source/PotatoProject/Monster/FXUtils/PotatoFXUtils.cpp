#include "PotatoFXUtils.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

#include "UObject/ObjectKey.h"
#include "Engine/World.h"

// Niagara / Cascade
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystem.h"

#include "Monster/PotatoMonster.h"
// 네 프로젝트 슬롯 정의 헤더(경로는 네 프로젝트에 맞춰 조정)
#include "Components/CapsuleComponent.h"
#include "Monster/PotatoMonsterSpecialSkillPresetRow.h" // FPotatoSkillVFXSlot / FPotatoSkillSFXSlot 가 여기 있다고 가정
#include "Monster/SpecialSkillComponent.h"              // USpecialSkillComponent

// ------------------------------
// Global history per channel (기존 유지)
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
	case PotatoFX::EGlobalBudgetChannel::HitSFX:    return G_HitSFXTimes;
	case PotatoFX::EGlobalBudgetChannel::DeathSFX:  return G_DeathSFXTimes;
	case PotatoFX::EGlobalBudgetChannel::HitVFX:    return G_HitVFXTimes;
	case PotatoFX::EGlobalBudgetChannel::CombatSFX: return G_CombatSFXTimes;
	default:                                       return G_DeathVFXTimes;
	}
}

static bool PassGlobalBudgetImpl(float Now, float WindowSec, int32 MaxCount, TArray<float>& Times)
{
	if (WindowSec <= 0.f || MaxCount <= 0) return true;

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

	if (Times.Num() >= MaxCount) return false;

	Times.Add(Now);
	return true;
}

// ------------------------------
// Safe finite checks (cpp-local)
// ------------------------------
static bool IsFiniteVector3(const FVector& V)
{
	return FMath::IsFinite((double)V.X) && FMath::IsFinite((double)V.Y) && FMath::IsFinite((double)V.Z);
}
static bool IsFiniteRotator(const FRotator& R)
{
	return FMath::IsFinite((double)R.Pitch) && FMath::IsFinite((double)R.Yaw) && FMath::IsFinite((double)R.Roll);
}
static bool IsFiniteScale3(const FVector& S)
{
	return FMath::IsFinite((double)S.X) && FMath::IsFinite((double)S.Y) && FMath::IsFinite((double)S.Z);
}

// ------------------------------
// Skill SFX Burst Gate (Owner+Sound)
// ------------------------------
static double GSkillSfxGateWindowSec = 0.06;
static int32  GSkillSfxGateMaxPlaysPerWindowPerOwner = 2;

struct FSfxGateKey
{
	FObjectKey SoundKey;
	TWeakObjectPtr<AActor> Owner;

	bool operator==(const FSfxGateKey& Other) const
	{
		return SoundKey == Other.SoundKey && Owner == Other.Owner;
	}
};
FORCEINLINE uint32 GetTypeHash(const FSfxGateKey& K)
{
	return HashCombine(GetTypeHash(K.SoundKey), GetTypeHash(K.Owner));
}
struct FSfxGateBucket
{
	double WindowStart = 0.0;
	int32 PlaysInWindow = 0;
};
static TMap<FSfxGateKey, FSfxGateBucket> GSfxGate;

static bool PassesSfxBurstGate(const UWorld* World, AActor* Owner, USoundBase* Sound, bool bImportant)
{
	if (!World || !Owner || !Sound) return true;
	if (bImportant) return true;

	const double Now = World->GetTimeSeconds();

	FSfxGateKey Key;
	Key.SoundKey = FObjectKey(Sound);
	Key.Owner = Owner;

	FSfxGateBucket& B = GSfxGate.FindOrAdd(Key);

	if (Now - B.WindowStart > GSkillSfxGateWindowSec)
	{
		B.WindowStart = Now;
		B.PlaysInWindow = 0;
	}

	if (B.PlaysInWindow >= GSkillSfxGateMaxPlaysPerWindowPerOwner)
	{
		return false;
	}

	B.PlaysInWindow++;
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

	bool PassesFxDistanceGate(const UObject* WorldContextObject, const FVector& SpawnLoc, float MaxDist)
	{
		if (!WorldContextObject) return false;
		if (MaxDist <= 0.f) return true;

		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
		if (!PlayerPawn) return true;

		const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc);
		return (Dist <= MaxDist);
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
	// Net mode string
	// ------------------------------------------------------------
	const TCHAR* NetModeToCStr(const UWorld* World)
	{
		if (!World) return TEXT("NoWorld");
		switch (World->GetNetMode())
		{
		case NM_Standalone:      return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		case NM_ListenServer:    return TEXT("ListenServer");
		case NM_Client:          return TEXT("Client");
		default:                 return TEXT("Unknown");
		}
	}

	// ------------------------------------------------------------
	// SFX spawn (기존)
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
	// VFX auto scale (기존)
	// ------------------------------------------------------------
	float PotatoFX::ComputeVFXSlotAutoScaleScalar(AActor* OwnerActor, const FPotatoVFXSlot& Slot)
	{
		if (!OwnerActor) return 1.f;
		if (!Slot.bAutoScale) return 1.f;

		float Ext = 0.f;

		if (USkeletalMeshComponent* Mesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			const FVector E = Mesh->Bounds.BoxExtent;
			Ext = Slot.bUseMaxExtentXYZ ? FMath::Max3(E.X, E.Y, E.Z) : E.Z;
		}
		else if (UCapsuleComponent* Cap = OwnerActor->FindComponentByClass<UCapsuleComponent>())
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
	// VFX spawn (기존)
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

			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				Slot.Cascade,
				LocationWS,
				RotationWS,
				FinalScale,
				true,
				EPSCPoolMethod::AutoRelease
			);
			return;
		}
	}

	// ------------------------------------------------------------
	// Skill VFX (NEW) : Attached 우선, 아니면 AtLocation
	// ------------------------------------------------------------
	void PlaySkillVFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillVFXSlot& Slot,
		const FVector& Origin, const FRotator& Facing)
	{
		if (!Comp) return;

		AActor* Owner = Comp->GetOwner();
		if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

		UWorld* World = Comp->GetWorld();
		if (!World || World->bIsTearingDown) return;

		if (!Slot.HasAny()) return;

		const FVector Loc = Origin + Slot.LocationOffset;
		const FRotator Rot = (Facing + Slot.RotationOffset);

		if (!IsFiniteVector3(Loc) || !IsFiniteRotator(Rot) || !IsFiniteScale3(Slot.Scale))
		{
			return;
		}

		// Attach path
		if (!Slot.AttachSocket.IsNone())
		{
			USkeletalMeshComponent* Skel = nullptr;
			if (ACharacter* C = Cast<ACharacter>(Owner)) Skel = C->GetMesh();
			if (!Skel) Skel = Owner->FindComponentByClass<USkeletalMeshComponent>();

			if (Skel && Skel->IsRegistered() && Skel->DoesSocketExist(Slot.AttachSocket))
			{
				if (!Slot.Niagara.IsNull())
				{
					UNiagaraSystem* NS = TryGetOrLoadSync(Slot.Niagara);
					if (NS)
					{
						UNiagaraFunctionLibrary::SpawnSystemAttached(
							NS, Skel, Slot.AttachSocket,
							Slot.LocationOffset, Slot.RotationOffset,
							EAttachLocation::KeepRelativeOffset,
							true, true);
					}
					return;
				}

				if (!Slot.Cascade.IsNull())
				{
					UParticleSystem* PS = TryGetOrLoadSync(Slot.Cascade);
					if (PS)
					{
						UGameplayStatics::SpawnEmitterAttached(
							PS, Skel, Slot.AttachSocket,
							Slot.LocationOffset, Slot.RotationOffset,
							EAttachLocation::KeepRelativeOffset,
							true);
					}
					return;
				}
			}
		}

		// AtLocation path
		if (!Slot.Niagara.IsNull())
		{
			UNiagaraSystem* NS = TryGetOrLoadSync(Slot.Niagara);
			if (NS)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World, NS, Loc, Rot, Slot.Scale, true, true, ENCPoolMethod::AutoRelease);
			}
			return;
		}

		if (!Slot.Cascade.IsNull())
		{
			UParticleSystem* PS = TryGetOrLoadSync(Slot.Cascade);
			if (PS)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					World, PS, FTransform(Rot, Loc, Slot.Scale), true);
			}
			return;
		}
	}

	// ------------------------------------------------------------
	// Skill SFX (NEW) : 거리 + burst + atten/concurrency
	// ------------------------------------------------------------
	void PlaySkillSFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillSFXSlot& Slot,
		const FVector& Origin, float MaxFxDist, bool bImportant)
	{
		if (!Comp) return;

		AActor* Owner = Comp->GetOwner();
		if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

		UWorld* World = Comp->GetWorld();
		if (!World || World->bIsTearingDown) return;

		if (World->GetNetMode() == NM_DedicatedServer) return;

		if (!PassesFxDistanceGate(World, Origin, MaxFxDist))
		{
			return;
		}

		USoundBase* S = TryGetOrLoadSync(Slot.Sound);
		if (!S) return;

		if (!PassesSfxBurstGate(World, Owner, S, bImportant))
		{
			return;
		}

		USoundAttenuation* Atten = TryGetOrLoadSync(Slot.Attenuation);
		USoundConcurrency* Con   = TryGetOrLoadSync(Slot.Concurrency);

		const float Pitch = FMath::Clamp(Slot.PitchMultiplier + FMath::FRandRange(-0.03f, 0.03f), 0.5f, 2.0f);
		const float Vol   = FMath::Max(0.f, Slot.VolumeMultiplier);

		UGameplayStatics::PlaySoundAtLocation(World, S, Origin, Vol, Pitch, 0.f, Atten, Con);
	}
}