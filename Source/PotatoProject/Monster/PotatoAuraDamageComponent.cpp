// PotatoAuraDamageComponent.cpp (BUILDABLE - Shipping-safe teardown guards + overlap safety + tags policy)
//
// 정책:
// - RequiredTargetTags가 비어있으면(=Empty/None) Aura가 켜져 있어도 "아무도 때리지 않음" (StartAuraInternal/TickAura에서 즉시 StopAura)
// - RequiredTargetTags 중 하나라도 가진 Pawn만 데미지
// - 기본: 몬스터끼리 데미지 금지(bAllowDamageToMonsters=false면 APotatoMonster 제외)
//
// 패키징/Shipping 크래시 방지 포인트:
// - World teardown(EndPlay/맵 전환/종료) 중에는 Sphere의 UpdateOverlaps/Collision 토글을 가급적 하지 않음
// - Sphere가 Unregister/BeginDestroyed/IsBeingDestroyed 상태면 절대 건드리지 않음
// - EndPlay에서 delegate 먼저 해제 -> StopAura로 정리
//
// CVars:
// - potato.AuraDmgDebug 0/1
// - potato.AuraDmgDraw 0/1
// - potato.AuraUsePointDamage 0/1

#include "PotatoAuraDamageComponent.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"

#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"

#include "Monster/PotatoMonster.h"

const FName UPotatoAuraDamageComponent::AuraSphereName(TEXT("PotatoAuraSphere"));

// ------------------------------------------------------------
// CVars
// ------------------------------------------------------------
static TAutoConsoleVariable<int32> CVarAuraDmgDebug(
	TEXT("potato.AuraDmgDebug"), 1,
	TEXT("0=off, 1=log aura damage overlaps and apply"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarAuraDmgDraw(
	TEXT("potato.AuraDmgDraw"), 0,
	TEXT("0=off, 1=draw debug sphere radius"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarAuraUsePointDamage(
	TEXT("potato.AuraUsePointDamage"), 1,
	TEXT("0=Use ApplyDamage, 1=Prefer ApplyPointDamage"),
	ECVF_Default
);

static FORCEINLINE bool AuraDbgOn()
{
	return CVarAuraDmgDebug.GetValueOnGameThread() != 0;
}

// ------------------------------------------------------------
// Helpers (cpp-local)
// ------------------------------------------------------------
static FString TagsToString(const TArray<FName>& Tags)
{
	if (Tags.Num() == 0) return TEXT("Empty");

	FString S;
	for (int32 i = 0; i < Tags.Num(); ++i)
	{
		const FName T = Tags[i];
		S += T.IsNone() ? TEXT("None") : T.ToString();
		if (i != Tags.Num() - 1) S += TEXT(",");
	}
	return S;
}

static bool HasAnyRequiredTag(AActor* A, const TArray<FName>& Tags)
{
	if (!A) return false;

	for (const FName& T : Tags)
	{
		if (T.IsNone()) continue;
		if (A->ActorHasTag(T)) return true;
	}
	return false;
}

static bool HasAnyNonNoneTag(const TArray<FName>& Tags)
{
	for (const FName& T : Tags)
	{
		if (!T.IsNone()) return true;
	}
	return false;
}

static FORCEINLINE bool WorldSafeForOverlaps(const UWorld* W)
{
	// 월드 없으면 당연히 위험
	if (!W) return false;

	// 게임 월드가 아니거나(PIE 프리뷰/에디터 월드 등) 종료 흐름이면 보수적으로 막기
	if (!W->IsGameWorld()) return false;

	// 엔드플레이/맵 전환/종료 중 흔하게 들어오는 안전 체크들
	// (심볼 안정성이 높음)
	return (W->WorldType == EWorldType::Game
		 || W->WorldType == EWorldType::PIE
		 || W->WorldType == EWorldType::GamePreview);
}

// ------------------------------------------------------------
// Ctor
// ------------------------------------------------------------
UPotatoAuraDamageComponent::UPotatoAuraDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = false; // ✅ 기본 자동 활성화 방지
}

// ------------------------------------------------------------
// Safety
// ------------------------------------------------------------
bool UPotatoAuraDamageComponent::IsSphereSafeToTouch() const
{
	return IsValid(Sphere)
		&& Sphere->IsRegistered()
		&& !Sphere->IsBeingDestroyed()
		&& !Sphere->HasAnyFlags(RF_BeginDestroyed);
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void UPotatoAuraDamageComponent::Configure(float InRadius, float InDps, float InTickInterval)
{
	Radius = FMath::Max(0.f, InRadius);
	Dps = FMath::Max(0.f, InDps);
	TickInterval = FMath::Max(0.01f, InTickInterval);

	if (IsSphereSafeToTouch())
	{
		Sphere->SetSphereRadius(Radius, true);

		// UpdateOverlaps는 teardown 아닐 때만
		if (Sphere->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			if (WorldSafeForOverlaps(GetWorld()))
			{
				Sphere->UpdateOverlaps();
			}
		}
	}

	if (AuraDbgOn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] Configure Owner=%s R=%.1f Dps=%.2f Tick=%.3f Tags=[%s] AllowMonster=%d UsePoint=%d"),
			*GetNameSafe(GetOwner()),
			Radius, Dps, TickInterval,
			*TagsToString(RequiredTargetTags),
			bAllowDamageToMonsters ? 1 : 0,
			CVarAuraUsePointDamage.GetValueOnGameThread()
		);
	}
}

// ------------------------------------------------------------
// Lifecycle
// ------------------------------------------------------------
void UPotatoAuraDamageComponent::BeginPlay()
{
	Super::BeginPlay();

	bBegunPlay = true;

	CachedOwner = GetOwner();
	if (!CachedOwner) return;

	EnsureSphereCreated();
	if (!Sphere) return;

	// ✅ BeginPlay 전에 StartAura()가 들어온 경우 처리
	if (bPendingStart || IsActive())
	{
		bPendingStart = false;
		StartAuraInternal();
	}
	else
	{
		ApplySphereOffState();
	}
}

void UPotatoAuraDamageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AuraDbgOn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] EndPlay Owner=%s Targets=%d"),
			*GetNameSafe(GetOwner()),
			OverlappingTargets.Num()
		);
	}

	// ✅ 콜백 재진입/teardown 중 호출 방지: delegate 먼저 해제
	if (IsValid(Sphere))
	{
		Sphere->OnComponentBeginOverlap.RemoveAll(this);
		Sphere->OnComponentEndOverlap.RemoveAll(this);
	}

	StopAura();
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------
// Internals
// ------------------------------------------------------------
void UPotatoAuraDamageComponent::EnsureSphereCreated()
{
	if (!CachedOwner) return;

	// 기존에 같은 이름으로 붙어있는 Sphere가 있으면 재사용
	for (UActorComponent* C : CachedOwner->GetComponents())
	{
		if (USphereComponent* S = Cast<USphereComponent>(C))
		{
			if (S->GetFName() == AuraSphereName)
			{
				Sphere = S;
				break;
			}
		}
	}

	// 없으면 생성
	if (!Sphere)
	{
		Sphere = NewObject<USphereComponent>(CachedOwner, USphereComponent::StaticClass(), AuraSphereName);
		if (Sphere)
		{
			Sphere->SetCanEverAffectNavigation(false);
			Sphere->RegisterComponent();

			if (USceneComponent* Root = CachedOwner->GetRootComponent())
			{
				Sphere->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}
	}

	if (!Sphere) return;

	// 공통 세팅(ON/OFF는 별도)
	Sphere->SetSphereRadius(Radius);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 바인딩 초기화/재바인딩
	Sphere->OnComponentBeginOverlap.RemoveAll(this);
	Sphere->OnComponentEndOverlap.RemoveAll(this);

	// ✅ 주의: HandleBeginOverlap/HandleEndOverlap 는 반드시 UFUNCTION() 이어야 함
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &UPotatoAuraDamageComponent::HandleBeginOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &UPotatoAuraDamageComponent::HandleEndOverlap);
}

void UPotatoAuraDamageComponent::ApplySphereOffState()
{
	if (!IsSphereSafeToTouch()) return;

	UWorld* W = GetWorld();
	const bool bCanUpdate = WorldSafeForOverlaps(W);

	Sphere->SetGenerateOverlapEvents(false);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// OFF 전환 시 중복 오버랩 정리 (월드 안전할 때만)
	if (bCanUpdate)
	{
		Sphere->UpdateOverlaps();
	}
}

void UPotatoAuraDamageComponent::ApplySphereOnState()
{
	if (!IsSphereSafeToTouch()) return;

	UWorld* W = GetWorld();
	const bool bCanUpdate = WorldSafeForOverlaps(W);

	Sphere->SetSphereRadius(Radius, true);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetGenerateOverlapEvents(true);

	// ON 전환 즉시 주변 대상 반영 (월드 안전할 때만)
	if (bCanUpdate)
	{
		Sphere->UpdateOverlaps();
	}
}

void UPotatoAuraDamageComponent::StartAura()
{
	if (!CachedOwner) CachedOwner = GetOwner();
	if (!CachedOwner) return;

	// ✅ BeginPlay 전이면 실제 ON은 예약만
	if (!bBegunPlay)
	{
		bPendingStart = true;
		if (AuraDbgOn())
		{
			UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] StartAura (PENDING) Owner=%s"), *GetNameSafe(CachedOwner));
		}
		return;
	}

	StartAuraInternal();
}

void UPotatoAuraDamageComponent::StopAura()
{
	// Timer 정리
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AuraTickTH);
	}

	OverlappingTargets.Reset();
	LastSweepHitByTarget.Reset();

	// teardown/파괴 중엔 Sphere를 안전하게만 OFF
	if (IsSphereSafeToTouch())
	{
		UWorld* W = GetWorld();
		const bool bCanUpdate = WorldSafeForOverlaps(W);

		Sphere->SetGenerateOverlapEvents(false);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// ✅ teardown 중엔 UpdateOverlaps 금지
		if (bCanUpdate)
		{
			Sphere->UpdateOverlaps();
		}
	}

	if (AuraDbgOn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] StopAura Owner=%s"), *GetNameSafe(GetOwner()));
	}
}

bool UPotatoAuraDamageComponent::IsValidTarget(AActor* A) const
{
	if (!IsValid(A) || A->IsActorBeingDestroyed()) return false;
	if (A == CachedOwner) return false;

	// ✅ 정책: 태그 배열이 비어있으면 "아무도 때리지 않음"
	if (!HasAnyNonNoneTag(RequiredTargetTags)) return false;

	// ✅ 태그 중 하나라도 만족해야 타겟
	if (!HasAnyRequiredTag(A, RequiredTargetTags)) return false;

	// 기본: 몬스터끼리 데미지 금지
	if (!bAllowDamageToMonsters && A->IsA(APotatoMonster::StaticClass()))
	{
		return false;
	}

	return true;
}

// ------------------------------------------------------------
// Overlaps
// ------------------------------------------------------------
void UPotatoAuraDamageComponent::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// teardown 중엔 콜백 들어와도 무시
	if (!WorldSafeForOverlaps(GetWorld())) return;

	if (!IsValidTarget(OtherActor))
	{
		if (AuraDbgOn())
		{
			UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] BeginOverlap IGNORE Owner=%s Other=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(OtherActor)
			);
		}
		return;
	}

	OverlappingTargets.Add(OtherActor);

	if (bFromSweep)
	{
		LastSweepHitByTarget.Add(OtherActor, SweepResult);
	}
	else
	{
		FHitResult Dummy;
		Dummy.Location    = OtherActor->GetActorLocation();
		Dummy.ImpactPoint = Dummy.Location;
		Dummy.TraceStart  = CachedOwner ? CachedOwner->GetActorLocation() : Dummy.Location;
		Dummy.TraceEnd    = Dummy.Location;
		LastSweepHitByTarget.Add(OtherActor, Dummy);
	}
}

void UPotatoAuraDamageComponent::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!WorldSafeForOverlaps(GetWorld())) return;
	if (!OtherActor) return;

	OverlappingTargets.Remove(OtherActor);
	LastSweepHitByTarget.Remove(OtherActor);
}

// ------------------------------------------------------------
// Tick
// ------------------------------------------------------------
void UPotatoAuraDamageComponent::TickAura()
{
	UWorld* W = GetWorld();
	if (!CachedOwner || !WorldSafeForOverlaps(W))
	{
		StopAura();
		return;
	}

	if (CVarAuraDmgDraw.GetValueOnGameThread() != 0)
	{
		DrawDebugSphere(W, CachedOwner->GetActorLocation(), Radius, 20, FColor::Green, false, TickInterval, 0, 1.5f);
	}

	// ✅ 잘못된 설정이면 즉시 중지
	if (Radius <= 0.f || Dps <= 0.f || !HasAnyNonNoneTag(RequiredTargetTags))
	{
		if (AuraDbgOn())
		{
			UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] Tick STOP (invalid) Owner=%s R=%.1f Dps=%.2f Tags=[%s]"),
				*GetNameSafe(CachedOwner),
				Radius, Dps,
				*TagsToString(RequiredTargetTags)
			);
		}
		StopAura();
		return;
	}

	const float DamageThisTick = Dps * TickInterval;
	if (DamageThisTick <= 0.f) return;

	AController* InstigatorController = nullptr;
	if (APawn* P = Cast<APawn>(CachedOwner))
	{
		InstigatorController = P->GetController();
	}

	const bool bUsePoint = (CVarAuraUsePointDamage.GetValueOnGameThread() != 0);

	TArray<TWeakObjectPtr<AActor>> ToRemove;

	for (const TWeakObjectPtr<AActor>& WeakT : OverlappingTargets)
	{
		AActor* T = WeakT.Get();
		if (!IsValidTarget(T))
		{
			ToRemove.Add(WeakT);
			continue;
		}

		// ✅ UClass*로 정규화
		UClass* DTClass = DamageTypeClass ? DamageTypeClass.Get() : UDamageType::StaticClass();

		if (bUsePoint)
		{
			FHitResult Hit;
			if (const FHitResult* CachedHit = LastSweepHitByTarget.Find(T))
			{
				Hit = *CachedHit;
			}
			else
			{
				Hit.Location    = T->GetActorLocation();
				Hit.ImpactPoint = Hit.Location;
				Hit.TraceStart  = CachedOwner->GetActorLocation();
				Hit.TraceEnd    = Hit.Location;
			}

			// 라인트레이스로 보정(선택)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(AuraPointDamageTrace), false, CachedOwner);
				Params.AddIgnoredActor(CachedOwner);

				FHitResult TraceHit;
				const FVector Start = CachedOwner->GetActorLocation();
				const FVector End   = T->GetActorLocation();

				if (W->LineTraceSingleByChannel(TraceHit, Start, End, ECC_Visibility, Params))
				{
					if (TraceHit.GetActor() == T)
					{
						Hit = TraceHit;
					}
				}
			}

			const FVector ShotDir = (T->GetActorLocation() - CachedOwner->GetActorLocation()).GetSafeNormal();

			UGameplayStatics::ApplyPointDamage(
				T,
				DamageThisTick,
				ShotDir,
				Hit,
				InstigatorController,
				CachedOwner,
				DTClass
			);
		}
		else
		{
			UGameplayStatics::ApplyDamage(
				T,
				DamageThisTick,
				InstigatorController,
				CachedOwner,
				DTClass
			);
		}
	}

	for (const auto& R : ToRemove)
	{
		OverlappingTargets.Remove(R);
		LastSweepHitByTarget.Remove(R);
	}
}

void UPotatoAuraDamageComponent::StartAuraInternal()
{
	if (!CachedOwner) CachedOwner = GetOwner();
	if (!CachedOwner) return;

	// teardown 중에는 시작/토글 금지
	if (!WorldSafeForOverlaps(GetWorld()))
	{
		return;
	}

	EnsureSphereCreated();
	if (!Sphere) return;

	// None/빈배열이면 "아무도 안 때림" 정책
	if (!HasAnyNonNoneTag(RequiredTargetTags) || Radius <= 0.f || Dps <= 0.f)
	{
		StopAura();
		return;
	}

	ApplySphereOnState();

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AuraTickTH);
		W->GetTimerManager().SetTimer(AuraTickTH, this, &UPotatoAuraDamageComponent::TickAura, TickInterval, true);
	}

	if (AuraDbgOn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] StartAuraInternal Owner=%s R=%.1f Dps=%.2f Tick=%.3f Tags=[%s]"),
			*GetNameSafe(CachedOwner),
			Radius, Dps, TickInterval,
			*TagsToString(RequiredTargetTags)
		);
	}
}