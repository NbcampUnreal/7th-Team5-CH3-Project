#include "PotatoDamageTextPoolActor.h"
#include "PotatoDamageTextActor.h"
#include "Engine/World.h"

APotatoDamageTextPoolActor::APotatoDamageTextPoolActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APotatoDamageTextPoolActor::BeginPlay()
{
	Super::BeginPlay();

	if (!DamageTextActorClass) return;

	for (int32 i = 0; i < PrewarmCount; ++i)
	{
		APotatoDamageTextActor* A =
			GetWorld()->SpawnActor<APotatoDamageTextActor>(DamageTextActorClass);

		A->SetActorHiddenInGame(true);
		Pool.Add(A);
	}
}

APotatoDamageTextActor* APotatoDamageTextPoolActor::Acquire()
{
	if (!DamageTextActorClass) return nullptr;

	if (Pool.Num() > 0)
	{
		APotatoDamageTextActor* A = Pool.Pop(false);
		Active.Add(A);
		return A;
	}

	APotatoDamageTextActor* A =
		GetWorld()->SpawnActor<APotatoDamageTextActor>(DamageTextActorClass);

	Active.Add(A);
	return A;
}

void APotatoDamageTextPoolActor::Release(APotatoDamageTextActor* Actor)
{
	if (!Actor) return;

	Active.RemoveSingleSwap(Actor);
	Pool.Add(Actor);
}

void APotatoDamageTextPoolActor::SpawnDamageText(
	int32 Damage,
	const FVector& WorldLocation,
	int32 StackIndex
)
{
	APotatoDamageTextActor* A = Acquire();
	if (!A) return;

	A->ShowDamage(
		Damage,
		WorldLocation,
		StackIndex,
		FPotatoDamageTextFinished::CreateUObject(
			this,
			&APotatoDamageTextPoolActor::Release
		)
	);
}