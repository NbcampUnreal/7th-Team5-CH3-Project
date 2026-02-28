// Source/PotatoProject/Monster/Utils/PotatoActorSafety.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

class UWorld;
class AActor;
class AController;
class APawn;

// ------------------------------------------------------------
// Name helpers (log safe)
// ------------------------------------------------------------
static FORCEINLINE FString GetNameSafe(const UObject* Obj)
{
	return Obj ? Obj->GetName() : TEXT("None");
}

static FORCEINLINE FString GetNameSafe(const UClass* Cls)
{
	return Cls ? Cls->GetName() : TEXT("None");
}

// ------------------------------------------------------------
// World / Actor safety
// ------------------------------------------------------------
static FORCEINLINE UWorld* GetSafeWorld(const UObject* Obj)
{
	if (!Obj) return nullptr;
	UWorld* W = Obj->GetWorld();
	if (!W) return nullptr;
	if (W->bIsTearingDown) return nullptr;
	return W;
}

static FORCEINLINE bool IsActorSafeToTouch(AActor* A, const UWorld* World = nullptr)
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (A->HasAnyFlags(RF_BeginDestroyed)) return false;
	if (World && A->GetWorld() != World) return false;
	return true;
}

static FORCEINLINE AController* GetSafeInstigatorController(AActor* Owner)
{
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return nullptr;

	if (APawn* P = Cast<APawn>(Owner))
	{
		if (AController* C = P->GetController())
		{
			return C;
		}
	}
	return Owner->GetInstigatorController();
}

// ------------------------------------------------------------
// Damage apply (safe)
// ------------------------------------------------------------
static FORCEINLINE void ApplyDamage_Safe(AActor* Victim, float Damage, AActor* Owner)
{
	if (!IsValid(Victim) || Victim->IsActorBeingDestroyed()) return;
	if (!IsValid(Owner)  || Owner->IsActorBeingDestroyed()) return;
	if (Damage <= 0.f) return;

	// 데미지 수신 자체가 꺼져 있으면 컷
	if (!Victim->CanBeDamaged())
	{
		return;
	}

	AController* InstigatorCon = GetSafeInstigatorController(Owner);

	UGameplayStatics::ApplyDamage(
		Victim,
		Damage,
		InstigatorCon,
		Owner,
		UDamageType::StaticClass()
	);
}

// ------------------------------------------------------------
// Component helper (safe add/register)
// ------------------------------------------------------------
template<typename TComp>
static TComp* FindOrAddComponentSafe(AActor* Host, UWorld* World)
{
	if (!IsActorSafeToTouch(Host, World)) return nullptr;

	TComp* Existing = Host->FindComponentByClass<TComp>();
	if (Existing && IsValid(Existing) && Existing->IsRegistered())
	{
		return Existing;
	}

	if (!World || World->bIsTearingDown) return nullptr;

	TComp* NewC = NewObject<TComp>(Host, TComp::StaticClass(), NAME_None, RF_Transient);
	if (!NewC) return nullptr;

	NewC->RegisterComponentWithWorld(World);
	return NewC;
}

// ------------------------------------------------------------
// (선택) UFUNCTION이 존재할 때만 호출 (빌드 에러 방지용)
// ------------------------------------------------------------
static FORCEINLINE bool CallUFunctionIfExists(UObject* Obj, FName FuncName)
{
	if (!Obj) return false;
	UFunction* Fn = Obj->FindFunction(FuncName);
	if (!Fn) return false;
	Obj->ProcessEvent(Fn, nullptr);
	return true;
}