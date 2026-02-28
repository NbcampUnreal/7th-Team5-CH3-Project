#pragma once

#include "CoreMinimal.h"

class UObject;
class UWorld;
class AActor;
class USoundBase;
class USoundAttenuation;
class USoundConcurrency;
class USpecialSkillComponent;

struct FPotatoSFXSlot;
struct FPotatoVFXSlot;

// 네 프로젝트에 이미 존재하는 스킬 슬롯 타입
struct FPotatoSkillVFXSlot;
struct FPotatoSkillSFXSlot;

namespace PotatoFX
{
	enum class EGlobalBudgetChannel : uint8
	{
		HitSFX,
		DeathSFX,
		HitVFX,
		DeathVFX,
		CombatSFX
	};

	// ------------------------------
	// Basic helpers
	// ------------------------------
	float ComputeDistanceChance(float Dist, float NearDist, float FarDist, float FarChance);
	bool PassDistanceGate(const UObject* WorldContextObject, const FVector& Loc,
		float NearDist, float FarDist, float FarChance);

	bool PassGlobalBudget(float Now, float WindowSec, int32 MaxCount, EGlobalBudgetChannel Channel);

	// ------------------------------
	// Skill presentation helpers (NEW)
	// ------------------------------
	bool PassesFxDistanceGate(const UObject* WorldContextObject, const FVector& SpawnLoc, float MaxDist);

	// 기존 일반 슬롯(이미 쓰고 있는 것)
	void SpawnSFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoSFXSlot& Slot, const FVector& Location);
	void SpawnVFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoVFXSlot& Slot,
		const FVector& LocationWS, const FRotator& RotationWS, const FVector& FinalScale);

	float ComputeVFXSlotAutoScaleScalar(AActor* OwnerActor, const FPotatoVFXSlot& Slot);

	// 스킬 슬롯 (Presentation 전용, 중복 제거 핵심)
	void PlaySkillVFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillVFXSlot& Slot,
		const FVector& Origin, const FRotator& Facing);

	void PlaySkillSFXSlot(USpecialSkillComponent* Comp, const FPotatoSkillSFXSlot& Slot,
		const FVector& Origin, float MaxFxDist, bool bImportant);

	// 필요하면 Presentation에서 로그 찍을 때 쓰라고 노출(선택)
	const TCHAR* NetModeToCStr(const UWorld* World);

	// 런타임 sync load 정책 (필요하면 다른 곳에서도 재사용 가능)
	template<typename T>
	T* TryGetOrLoadSync(TSoftObjectPtr<T> SoftPtr)
	{
		T* Obj = SoftPtr.Get();
		if (Obj) return Obj;

#ifndef POTATOFX_ALLOW_SYNC_LOAD
#define POTATOFX_ALLOW_SYNC_LOAD 0
#endif

#if POTATOFX_ALLOW_SYNC_LOAD
		if (!SoftPtr.IsNull())
		{
			return SoftPtr.LoadSynchronous();
		}
#endif
		return nullptr;
	}
}