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
#include "SkillTransformResolver.h"
#include "UObject/ObjectKey.h"
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
// Safer "no streaming load during combat" switch
//  - 권장: 프리셋 적용 시점에 미리 로드(스폰 때)
//  - 여기서는 런타임 LoadSynchronous를 기본적으로 꺼서 스톨/불안정 방지
// ======================================================
#ifndef SKILL_PRESENT_ALLOW_SYNC_LOAD
#define SKILL_PRESENT_ALLOW_SYNC_LOAD 0
#endif

template<typename T>
static T* TryGetOrLoadSync(TSoftObjectPtr<T> SoftPtr)
{
	T* Obj = SoftPtr.Get();
	if (Obj) return Obj;

#if SKILL_PRESENT_ALLOW_SYNC_LOAD
	if (!SoftPtr.IsNull())
	{
		return SoftPtr.LoadSynchronous();
	}
#endif
	return nullptr;
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
	if (EnumObj) return EnumObj->GetNameStringByValue(Value);
	return FString::Printf(TEXT("%lld"), Value);
}

// ======================================================
// Distance gate (VFX+SFX 공통으로 써도 됨)
// ======================================================
static bool PassesFxDistanceGate(USpecialSkillComponent* Comp, const FVector& SpawnLoc, float MaxDist)
{
	if (!Comp) return true;
	if (MaxDist <= 0.f) return true;

	UWorld* World = Comp->GetWorld();
	if (!World || World->bIsTearingDown) return false;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return true;

	const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc);
	return (Dist <= MaxDist);
}

// ======================================================
// Safer SFX burst gate
//  - 기존: "사운드별 전역" 드랍 → 다수 몬스터면 소리 씹힘 매우 흔함
//  - 개선: (Sound + Owner) 단위로 드랍. + 보스/중요스킬은 완화 가능
// ======================================================
static double GSkillSfxGateWindowSec = 0.06;  // 조금 더 짧게
static int32  GSkillSfxGateMaxPlaysPerWindowPerOwner = 2; // Owner당 2회

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
	if (bImportant) return true; // 보스/중요 스킬은 Gate 면제(원하면 정책 바꿔도 됨)

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
		SKP_LOG(VeryVerbose, "SFXGate DROP Owner=%s Sound=%s Plays=%d Window=%.3f",
			*GetNameSafe(Owner), *GetNameSafe(Sound), B.PlaysInWindow, (float)GSkillSfxGateWindowSec);
		return false;
	}

	B.PlaysInWindow++;
	return true;
}


// ======================================================
// VFX slot
// ======================================================
static void PlayVFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillVFXSlot& Slot, const FVector& Origin, const FRotator& Facing)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

	UWorld* World = Comp->GetWorld();
	if (!World || World->bIsTearingDown) return;

	if (!Slot.HasAny())
	{
		SKP_LOG(VeryVerbose, "VFX Slot empty Owner=%s", *GetNameSafe(Owner));
		return;
	}

	const FVector Loc = Origin + Slot.LocationOffset;
	const FRotator Rot = (Facing + Slot.RotationOffset);

	if (!IsFiniteVector3(Loc) || !IsFiniteRotator(Rot) || !IsFiniteScale3(Slot.Scale))
	{
		SKP_LOG(Error, "VFX INVALID TRANSFORM Owner=%s Loc=%s Rot=%s Scale=%s",
			*GetNameSafe(Owner), *Loc.ToString(), *Rot.ToString(), *Slot.Scale.ToString());
		return;
	}

	// Attach
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
					SKP_LOG(VeryVerbose, "VFX Niagara not loaded (skip) Owner=%s Asset=%s",
						*GetNameSafe(Owner), *Slot.Niagara.ToString());
				}
				return;
			}

			if (!Slot.Cascade.IsNull())
			{
				UParticleSystem* PS = TryGetOrLoadSync(Slot.Cascade);
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
					SKP_LOG(VeryVerbose, "VFX Cascade not loaded (skip) Owner=%s Asset=%s",
						*GetNameSafe(Owner), *Slot.Cascade.ToString());
				}
				return;
			}
		}
	}

	// AtLocation
	if (!Slot.Niagara.IsNull())
	{
		UNiagaraSystem* NS = TryGetOrLoadSync(Slot.Niagara);
		if (NS)
		{
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, NS, Loc, Rot, Slot.Scale, true, true, ENCPoolMethod::AutoRelease);

			SKP_LOG(Log, "VFX SpawnAtLocation Niagara OK Owner=%s NS=%s Comp=%s",
				*GetNameSafe(Owner), *GetNameSafe(NS), *GetNameSafe(NC));
		}
		else
		{
			SKP_LOG(VeryVerbose, "VFX Niagara not loaded (skip) Owner=%s Asset=%s",
				*GetNameSafe(Owner), *Slot.Niagara.ToString());
		}
		return;
	}

	if (!Slot.Cascade.IsNull())
	{
		UParticleSystem* PS = TryGetOrLoadSync(Slot.Cascade);
		if (PS)
		{
			UParticleSystemComponent* PC = UGameplayStatics::SpawnEmitterAtLocation(
				World, PS, FTransform(Rot, Loc, Slot.Scale), true);

			SKP_LOG(Log, "VFX SpawnAtLocation Cascade OK Owner=%s PS=%s Comp=%s",
				*GetNameSafe(Owner), *GetNameSafe(PS), *GetNameSafe(PC));
		}
		else
		{
			SKP_LOG(VeryVerbose, "VFX Cascade not loaded (skip) Owner=%s Asset=%s",
				*GetNameSafe(Owner), *Slot.Cascade.ToString());
		}
		return;
	}
}

// ======================================================
// SFX slot (드랍 원인 추적 강화)
// ======================================================
static void PlaySFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillSFXSlot& Slot, const FVector& Origin, float MaxFxDist, bool bImportant)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

	UWorld* World = Comp->GetWorld();
	if (!World || World->bIsTearingDown) return;

	// DS no audio
	if (World->GetNetMode() == NM_DedicatedServer) return;

	// 거리 게이트: "VFX는 보이는데 SFX는 안 들림"을 줄이려면
	// SFX도 같은 거리 정책을 타는 게 보통 안전함(원하면 분리)
	if (!PassesFxDistanceGate(Comp, Origin, MaxFxDist))
	{
		SKP_LOG(VeryVerbose, "SFX dropped by distance gate Owner=%s", *GetNameSafe(Owner));
		return;
	}

	USoundBase* S = TryGetOrLoadSync(Slot.Sound);
	if (!S)
	{
		SKP_LOG(VeryVerbose, "SFX not loaded (skip) Owner=%s Asset=%s",
			*GetNameSafe(Owner), Slot.Sound.IsNull() ? TEXT("Null") : *Slot.Sound.ToString());
		return;
	}

	if (!PassesSfxBurstGate(World, Owner, S, bImportant))
	{
		return;
	}

	USoundAttenuation* Atten = TryGetOrLoadSync(Slot.Attenuation);
	USoundConcurrency* Con = TryGetOrLoadSync(Slot.Concurrency);

	const float Pitch = FMath::Clamp(Slot.PitchMultiplier + FMath::FRandRange(-0.03f, 0.03f), 0.5f, 2.0f);
	const float Vol   = FMath::Max(0.f, Slot.VolumeMultiplier);

	// Concurrency에 막히는 경우가 많아서 "Con 이름" 로그가 필요
	SKP_LOG(Log, "SFX Play Owner=%s Net=%s Sound=%s Vol=%.2f Pitch=%.2f Origin=%s Atten=%s Con=%s",
		*GetNameSafe(Owner),
		NetModeToCStr(World),
		*GetNameSafe(S),
		Vol,
		Pitch,
		*Origin.ToString(),
		*GetNameSafe(Atten),
		*GetNameSafe(Con));

	UGameplayStatics::PlaySoundAtLocation(World, S, Origin, Vol, Pitch, 0.f, Atten, Con);
}

// ======================================================
// Presentation common
// ======================================================
static void PlayPresentationCommon(
	USpecialSkillComponent* Comp,
	const FPotatoMonsterSpecialSkillPresetRow& Row,
	AActor* Target,
	const FPotatoSkillVFXSlot& Vfx,
	const FPotatoSkillSFXSlot& Sfx,
	const TCHAR* PhaseName)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

	UWorld* World = Comp->GetWorld();
	if (!World || World->bIsTearingDown) return;

	const FVector Origin =FSkillTransformResolver::ResolveOrigin(Owner, Target, Row);

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

	// VFX/SFX 공통 거리 정책
	if (!PassesFxDistanceGate(Comp, Origin, Row.MaxFxDistance))
	{
		SKP_LOG(VeryVerbose, "Present DROP DistanceGate Phase=%s Owner=%s", PhaseName, *GetNameSafe(Owner));
		return;
	}

	const FRotator Facing = Owner->GetActorRotation();
	PlayVFXSlot(Comp, Vfx, Origin, Facing);

	// "중요 스킬" 판정 정책(예: 보스/엘리트 Execute는 중요)
	// Row에 bImportantSfx 같은 필드가 있으면 그걸 쓰는 게 정석인데,
	// 여기선 Phase=Execute를 중요로 간주(원하면 바꿔)
	const bool bImportant = (FCString::Stricmp(PhaseName, TEXT("Execute")) == 0);

	PlaySFXSlot(Comp, Sfx, Origin, Row.MaxFxDistance, bImportant);
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