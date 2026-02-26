#include "SpecialSkillPresentation.h"
#include "SpecialSkillComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"

// VFX/SFX
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

#include "UObject/UnrealType.h" // UEnum

// ======================================================
// Debug toggle
// ======================================================
#ifndef SKILL_PRESENT_DEBUG
#define SKILL_PRESENT_DEBUG 1
#endif

#if SKILL_PRESENT_DEBUG
#define SKP_LOG(Verbosity, Fmt, ...) UE_LOG(LogTemp, Verbosity, TEXT("[SkillPresent] " Fmt), ##__VA_ARGS__)
#else
#define SKP_LOG(Verbosity, Fmt, ...)
#endif

// ======================================================
// Burst gate (SFX)
// ======================================================
static double GSkillSfxGateWindowSec = 0.08;        // 80ms window
static int32  GSkillSfxGateMaxPlaysPerWindow = 3;   // allow up to 3 per window per sound

struct FSfxGateBucket
{
	double WindowStart = 0.0;
	int32 PlaysInWindow = 0;
};

static TMap<TWeakObjectPtr<USoundBase>, FSfxGateBucket> GSfxGate;

static bool PassesSfxBurstGate(const UWorld* World, USoundBase* Sound)
{
	if (!World || !Sound) return true;

	const double Now = World->GetTimeSeconds();
	FSfxGateBucket& B = GSfxGate.FindOrAdd(Sound);

	if (Now - B.WindowStart > GSkillSfxGateWindowSec)
	{
		B.WindowStart = Now;
		B.PlaysInWindow = 0;
	}

	if (B.PlaysInWindow >= GSkillSfxGateMaxPlaysPerWindow)
	{
		SKP_LOG(Verbose, "SFXGate DROP Sound=%s Plays=%d Window=%.3f",
			*GetNameSafe(Sound), B.PlaysInWindow, (float)GSkillSfxGateWindowSec);
		return false;
	}

	B.PlaysInWindow++;
	return true;
}

// ======================================================
// Safe finite checks (UE5.x compatible)
// ======================================================
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

// ======================================================
// Debug string helpers
// ======================================================
static const TCHAR* NetModeToCStr(const UWorld* World)
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

static FString EnumToString_Safe(const TCHAR* EnumPathName, int64 Value)
{
	const UEnum* EnumObj = FindObject<UEnum>(nullptr, EnumPathName);
	if (EnumObj)
	{
		return EnumObj->GetNameStringByValue(Value);
	}
	return FString::Printf(TEXT("%lld"), Value);
}

static bool PassesFxDistanceGate(USpecialSkillComponent* Comp, const FVector& SpawnLoc, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Comp) return true;
	if (Row.MaxFxDistance <= 0.f) return true;

	UWorld* World = Comp->GetWorld();
	if (!World) return true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return true;

	const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc);
	const bool bPass = (Dist <= Row.MaxFxDistance);

	if (!bPass)
	{
		SKP_LOG(Log, "DistGate DROP Owner=%s Dist=%.1f > Max=%.1f SpawnLoc=%s",
			*GetNameSafe(Comp->GetOwner()), Dist, Row.MaxFxDistance, *SpawnLoc.ToString());
	}

	return bPass;
}

static FVector ResolveSkillOrigin(AActor* Owner, AActor* Target, const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (!Owner) return FVector::ZeroVector;

	if (Row.TargetType == EMonsterSpecialTargetType::Self)
	{
		return Owner->GetActorLocation();
	}

	if (Row.TargetType == EMonsterSpecialTargetType::Location)
	{
		if (IsValid(Target))
		{
			return Target->GetActorLocation();
		}
		return Owner->GetActorLocation() + Owner->GetActorForwardVector() * FMath::Max(0.f, Row.Range);
	}

	return IsValid(Target) ? Target->GetActorLocation() : Owner->GetActorLocation();
}

static void PlayVFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillVFXSlot& Slot, const FVector& Origin, const FRotator& Facing)
{
	if (!Comp) return;

	UWorld* World = Comp->GetWorld();
	if (!World) return;

	if (!Slot.HasAny())
	{
		SKP_LOG(VeryVerbose, "VFX Slot empty. Owner=%s", *GetNameSafe(Comp->GetOwner()));
		return;
	}

	const FVector Loc = Origin + Slot.LocationOffset;
	const FRotator Rot = (Facing + Slot.RotationOffset);

	// NaN/Inf 방어
	if (!IsFiniteVector3(Loc) || !IsFiniteRotator(Rot) || !IsFiniteScale3(Slot.Scale))
	{
		SKP_LOG(Error, "VFX INVALID TRANSFORM Owner=%s Origin=%s Loc=%s Rot=%s Scale=%s",
			*GetNameSafe(Comp->GetOwner()),
			*Origin.ToString(),
			*Loc.ToString(),
			*Rot.ToString(),
			*Slot.Scale.ToString());
		return;
	}

	SKP_LOG(VeryVerbose, "VFX Begin Owner=%s Net=%s Loc=%s Rot=%s Scale=%s Socket=%s",
		*GetNameSafe(Comp->GetOwner()),
		NetModeToCStr(World),
		*Loc.ToString(),
		*Rot.ToString(),
		*Slot.Scale.ToString(),
		*Slot.AttachSocket.ToString());

	// Attach
	if (!Slot.AttachSocket.IsNone())
	{
		AActor* Owner = Comp->GetOwner();
		USkeletalMeshComponent* Skel = nullptr;
		if (ACharacter* C = Cast<ACharacter>(Owner)) Skel = C->GetMesh();
		if (!Skel) Skel = Owner ? Owner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

		if (Skel && Skel->DoesSocketExist(Slot.AttachSocket))
		{
			if (!Slot.Niagara.IsNull())
			{
				UNiagaraSystem* NS = Slot.Niagara.Get();
				if (!NS) NS = Slot.Niagara.LoadSynchronous();

				if (NS)
				{
					UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
						NS, Skel, Slot.AttachSocket,
						Slot.LocationOffset, Slot.RotationOffset,
						EAttachLocation::KeepRelativeOffset,
						true, true);

					SKP_LOG(Log, "VFX SpawnAttached Niagara OK Owner=%s NS=%s Comp=%s",
						*GetNameSafe(Owner), *GetNameSafe(NS), *GetNameSafe(NC));
				}
				else
				{
					SKP_LOG(Warning, "VFX Niagara load failed Owner=%s Asset=%s", *GetNameSafe(Owner), *Slot.Niagara.ToString());
				}
				return;
			}

			if (!Slot.Cascade.IsNull())
			{
				UParticleSystem* PS = Slot.Cascade.Get();
				if (!PS) PS = Slot.Cascade.LoadSynchronous();

				if (PS)
				{
					UParticleSystemComponent* PC = UGameplayStatics::SpawnEmitterAttached(
						PS, Skel, Slot.AttachSocket,
						Slot.LocationOffset, Slot.RotationOffset,
						EAttachLocation::KeepRelativeOffset,
						true);

					SKP_LOG(Log, "VFX SpawnAttached Cascade OK Owner=%s PS=%s Comp=%s",
						*GetNameSafe(Owner), *GetNameSafe(PS), *GetNameSafe(PC));
				}
				else
				{
					SKP_LOG(Warning, "VFX Cascade load failed Owner=%s Asset=%s", *GetNameSafe(Owner), *Slot.Cascade.ToString());
				}
				return;
			}
		}
		else
		{
			SKP_LOG(Warning, "VFX Attach failed Owner=%s Mesh=%s Socket=%s",
				*GetNameSafe(Owner),
				*GetNameSafe(Skel),
				*Slot.AttachSocket.ToString());
		}
	}

	// Location spawn
	if (!Slot.Niagara.IsNull())
	{
		UNiagaraSystem* NS = Slot.Niagara.Get();
		if (!NS) NS = Slot.Niagara.LoadSynchronous();
		if (NS)
		{
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, NS, Loc, Rot, Slot.Scale, true, true, ENCPoolMethod::AutoRelease);

			SKP_LOG(Log, "VFX SpawnAtLocation Niagara OK Owner=%s NS=%s Comp=%s Pool=AutoRelease",
				*GetNameSafe(Comp->GetOwner()), *GetNameSafe(NS), *GetNameSafe(NC));
		}
		else
		{
			SKP_LOG(Warning, "VFX Niagara load failed Owner=%s Asset=%s", *GetNameSafe(Comp->GetOwner()), *Slot.Niagara.ToString());
		}
		return;
	}

	if (!Slot.Cascade.IsNull())
	{
		UParticleSystem* PS = Slot.Cascade.Get();
		if (!PS) PS = Slot.Cascade.LoadSynchronous();
		if (PS)
		{
			UParticleSystemComponent* PC = UGameplayStatics::SpawnEmitterAtLocation(
				World, PS, FTransform(Rot, Loc, Slot.Scale), true);

			SKP_LOG(Log, "VFX SpawnAtLocation Cascade OK Owner=%s PS=%s Comp=%s",
				*GetNameSafe(Comp->GetOwner()), *GetNameSafe(PS), *GetNameSafe(PC));
		}
		else
		{
			SKP_LOG(Warning, "VFX Cascade load failed Owner=%s Asset=%s", *GetNameSafe(Comp->GetOwner()), *Slot.Cascade.ToString());
		}
		return;
	}
}

static void PlaySFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillSFXSlot& Slot, const FVector& Origin)
{
	if (!Comp) return;

	UWorld* World = Comp->GetWorld();
	if (!World) return;

	const bool bHasSoundRef = (!Slot.Sound.IsNull()) || (Slot.Sound.Get() != nullptr);
	if (!bHasSoundRef)
	{
		SKP_LOG(Warning, "SFX Slot considered empty (no sound ref). Owner=%s", *GetNameSafe(Comp->GetOwner()));
		return;
	}

	// DS에서는 오디오 없음 (멀티에서 원인 분리)
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		SKP_LOG(Verbose, "SFX Skip DedicatedServer Owner=%s", *GetNameSafe(Comp->GetOwner()));
		return;
	}

	USoundBase* S = Slot.Sound.Get();
	if (!S && !Slot.Sound.IsNull()) S = Slot.Sound.LoadSynchronous();

	if (!S)
	{
		SKP_LOG(Warning, "SFX Sound null/load failed Owner=%s Asset=%s",
			*GetNameSafe(Comp->GetOwner()),
			Slot.Sound.IsNull() ? TEXT("Null") : *Slot.Sound.ToString());
		return;
	}

	if (!PassesSfxBurstGate(World, S))
	{
		SKP_LOG(Verbose, "SFX Dropped by BurstGate Owner=%s Sound=%s", *GetNameSafe(Comp->GetOwner()), *GetNameSafe(S));
		return;
	}

	USoundAttenuation* Atten = Slot.Attenuation.Get();
	if (!Atten && !Slot.Attenuation.IsNull()) Atten = Slot.Attenuation.LoadSynchronous();

	USoundConcurrency* Con = Slot.Concurrency.Get();
	if (!Con && !Slot.Concurrency.IsNull()) Con = Slot.Concurrency.LoadSynchronous();

	const float Pitch = FMath::Clamp(Slot.PitchMultiplier + FMath::FRandRange(-0.03f, 0.03f), 0.5f, 2.0f);
	const float Vol = FMath::Max(0.f, Slot.VolumeMultiplier);

	SKP_LOG(Log, "SFX Play Owner=%s Net=%s Sound=%s Vol=%.2f Pitch=%.2f Origin=%s Atten=%s Con=%s",
		*GetNameSafe(Comp->GetOwner()),
		NetModeToCStr(World),
		*GetNameSafe(S),
		Vol,
		Pitch,
		*Origin.ToString(),
		*GetNameSafe(Atten),
		*GetNameSafe(Con));

	UGameplayStatics::PlaySoundAtLocation(World, S, Origin, Vol, Pitch, 0.f, Atten, Con);
}

static void PlayPresentationCommon(
	USpecialSkillComponent* Comp,
	const FPotatoMonsterSpecialSkillPresetRow& Row,
	AActor* Target,
	const FPotatoSkillVFXSlot& Vfx,
	const FPotatoSkillSFXSlot& Sfx,
	const TCHAR* PhaseName)
{
	if (!Comp)
	{
		SKP_LOG(Warning, "Present Comp null Phase=%s", PhaseName);
		return;
	}

	AActor* Owner = Comp->GetOwner();
	if (!Owner)
	{
		SKP_LOG(Warning, "Present Owner null Phase=%s", PhaseName);
		return;
	}

	UWorld* World = Comp->GetWorld();

	const FVector Origin = ResolveSkillOrigin(Owner, Target, Row);

	// enum 문자열은 가장 안전하게 UEnum으로 (EnumPathName은 너 enum의 실제 경로로 바꿔도 됨)
	// 못 찾으면 숫자로 출력됨.
	const FString TargetTypeStr = EnumToString_Safe(TEXT("/Script/PotatoProject.EMonsterSpecialTargetType"), (int64)Row.TargetType);

	SKP_LOG(VeryVerbose, "Present Phase=%s Owner=%s Net=%s Target=%s TargetType=%s Range=%.1f MaxFxDist=%.1f Origin=%s",
		PhaseName,
		*GetNameSafe(Owner),
		NetModeToCStr(World),
		*GetNameSafe(Target),
		*TargetTypeStr,
		Row.Range,
		Row.MaxFxDistance,
		*Origin.ToString());

	if (!PassesFxDistanceGate(Comp, Origin, Row))
	{
		SKP_LOG(Log, "Present DROP DistanceGate Phase=%s Owner=%s", PhaseName, *GetNameSafe(Owner));
		return;
	}

	const FRotator Facing = Owner->GetActorRotation();
	PlayVFXSlot(Comp, Vfx, Origin, Facing);
	PlaySFXSlot(Comp, Sfx, Origin);
}

void FSpecialSkillPresentation::PlayTelegraph(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.TelegraphVFX, Row.Presentation.TelegraphSFX, TEXT("Telegraph"));
}

void FSpecialSkillPresentation::PlayCast(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.CastVFX, Row.Presentation.CastSFX, TEXT("Cast"));
}

void FSpecialSkillPresentation::PlayExecute(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.ExecuteVFX, Row.Presentation.ExecuteSFX, TEXT("Execute"));
}

void FSpecialSkillPresentation::PlayEnd(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.EndVFX, Row.Presentation.EndSFX, TEXT("End"));
}