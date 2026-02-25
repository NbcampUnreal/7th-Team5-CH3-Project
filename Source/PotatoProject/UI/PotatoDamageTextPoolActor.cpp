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
	Prewarm();
}

void APotatoDamageTextPoolActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 풀 액터들은 레벨/PIE 종료 시 자동으로 정리되긴 하지만,
	// 상태 꼬임 방지용으로 레퍼런스만 정리해 둠.
	Pool.Reset();
	Active.Reset();

	Super::EndPlay(EndPlayReason);
}

void APotatoDamageTextPoolActor::Prewarm()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!DamageTextActorClass) return;
	if (PrewarmCount <= 0) return;

	Pool.Reserve(PrewarmCount);

	for (int32 i = 0; i < PrewarmCount; ++i)
	{
		APotatoDamageTextActor* A = SpawnNewPooledActor();
		if (!A) continue;

		Pool.Add(A);
	}
}

APotatoDamageTextActor* APotatoDamageTextPoolActor::SpawnNewPooledActor()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	if (!DamageTextActorClass) return nullptr;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APotatoDamageTextActor* A =
		World->SpawnActor<APotatoDamageTextActor>(DamageTextActorClass, FTransform::Identity, Params);

	if (!A) return nullptr;

	// 풀에서 관리하는 기본 안전 상태
	A->SetActorEnableCollision(false);
	A->SetActorHiddenInGame(true);
	A->SetActorTickEnabled(false);

	return A;
}

APotatoDamageTextActor* APotatoDamageTextPoolActor::Acquire()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	if (!DamageTextActorClass) return nullptr;

	// Pool에서 꺼내기 (Invalid는 버리고 계속)
	while (Pool.Num() > 0)
	{
		APotatoDamageTextActor* A = Pool.Pop(false);
		if (!IsValid(A)) continue;

		// 이미 Active에 있으면(이상 케이스) 다음
		if (Active.Contains(A)) continue;

		// 혹시 상태 꼬여있으면 초기화
		A->SetActorHiddenInGame(true);
		A->SetActorTickEnabled(false);

		Active.Add(A);
		return A;
	}

	// 없으면 새로 생성
	APotatoDamageTextActor* A = SpawnNewPooledActor();
	if (!A) return nullptr;

	Active.Add(A);
	return A;
}

void APotatoDamageTextPoolActor::Release(APotatoDamageTextActor* Actor)
{
	if (!IsValid(Actor)) return;

	// Active에 없으면 중복 Release이거나 외부 호출 → 무시(안정)
	if (!Active.Contains(Actor))
	{
		return;
	}

	Active.Remove(Actor);

	// 반환 시 안전 상태로 정리 (Actor가 이미 숨김 처리했어도 한 번 더)
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);

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
		FPotatoDamageTextFinished::CreateUObject(this, &APotatoDamageTextPoolActor::Release)
	);
}