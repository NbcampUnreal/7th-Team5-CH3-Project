// Combat/PotatoDotComponent.cpp

#include "PotatoDotComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"

UPotatoDotComponent::UPotatoDotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoDotComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPotatoDotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDot();
	Super::EndPlay(EndPlayReason);
}

float UPotatoDotComponent::GetRemainingTime() const
{
	if (!bActive) return 0.f;

	const UWorld* W = GetWorld();
	if (!W) return 0.f;

	const double Now = W->GetTimeSeconds();
	return (float)FMath::Max(0.0, EndTime - Now);
}

void UPotatoDotComponent::ApplyDot(AActor* Source, float Dps, float Duration, float TickInterval, EMonsterDotStackPolicy Policy)
{
	AActor* OwnerActor = GetOwner();
	UWorld* W = GetWorld();

	if (!OwnerActor || OwnerActor->IsActorBeingDestroyed()) return;
	if (!W) return;

	Dps = FMath::Max(0.f, Dps);
	Duration = FMath::Max(0.f, Duration);

	// TickInterval 0이면 기본값(0.5s) 사용
	if (TickInterval <= 0.f) TickInterval = 0.5f;
	TickInterval = FMath::Max(0.05f, TickInterval);

	if (Dps <= 0.f || Duration <= 0.f)
	{
		return;
	}

	const double Now = W->GetTimeSeconds();
	const double NewEnd = Now + (double)Duration;

	// 이미 활성인 경우 정책 처리
	if (bActive)
	{
		switch (Policy)
		{
		case EMonsterDotStackPolicy::RefreshDuration:
			// DPS는 유지, Duration만 갱신(연장)
			EndTime = FMath::Max(EndTime, NewEnd);
			SourceActor = Source;
			break;

		case EMonsterDotStackPolicy::OverrideValues:
			// 신규 값으로 덮어쓰기 (Dps/TickInterval/Duration 모두 신규 기준)
			CurrentDps = Dps;
			CurrentTickInterval = TickInterval;
			EndTime = NewEnd;
			SourceActor = Source;
			break;

		case EMonsterDotStackPolicy::Stack:
		default:
			// 누적(기본): DPS += 신규DPS, Duration은 더 긴 값으로
			CurrentDps += Dps;
			EndTime = FMath::Max(EndTime, NewEnd);
			SourceActor = Source;
			break;
		}
	}
	else
	{
		// 신규 활성화
		bActive = true;
		SourceActor = Source;
		CurrentDps = Dps;
		CurrentTickInterval = TickInterval;
		EndTime = NewEnd;

		// 타이머 시작
		W->GetTimerManager().ClearTimer(DotTickTH);
		W->GetTimerManager().SetTimer(
			DotTickTH,
			this,
			&UPotatoDotComponent::TickDot,
			CurrentTickInterval,
			true
		);
		return;
	}

	// 정책에 따라 TickInterval이 바뀐 경우(OverrideValues 등) 타이머 재설정
	if (bActive)
	{
		FTimerManager& TM = W->GetTimerManager();

		// 타이머가 없으면 생성, 있으면 rate 비교 후 재설정
		if (!TM.TimerExists(DotTickTH))
		{
			TM.SetTimer(
				DotTickTH,
				this,
				&UPotatoDotComponent::TickDot,
				CurrentTickInterval,
				true
			);
		}
		else
		{
			const float CurrentRate = TM.GetTimerRate(DotTickTH);
			if (FMath::Abs(CurrentRate - CurrentTickInterval) > KINDA_SMALL_NUMBER)
			{
				TM.ClearTimer(DotTickTH);
				TM.SetTimer(
					DotTickTH,
					this,
					&UPotatoDotComponent::TickDot,
					CurrentTickInterval,
					true
				);
			}
		}
	}
}

void UPotatoDotComponent::TickDot()
{
	AActor* OwnerActor = GetOwner();
	UWorld* W = GetWorld();
	if (!OwnerActor || OwnerActor->IsActorBeingDestroyed() || !W)
	{
		StopDot();
		return;
	}

	const double Now = W->GetTimeSeconds();
	if (!bActive || Now >= EndTime)
	{
		StopDot();
		return;
	}

	// Tick 데미지 = DPS * TickInterval
	const float DamageThisTick = FMath::Max(0.f, CurrentDps * CurrentTickInterval);
	if (DamageThisTick <= 0.f) return;

	AActor* Source = SourceActor.Get();

	AController* InstigatorController = nullptr;
	if (APawn* P = Cast<APawn>(Source))
	{
		InstigatorController = P->GetController();
	}

	UGameplayStatics::ApplyDamage(
		OwnerActor,
		DamageThisTick,
		InstigatorController,
		Source ? Source : nullptr,
		UDamageType::StaticClass()
	);
}

void UPotatoDotComponent::StopDot()
{
	bActive = false;
	SourceActor.Reset();
	CurrentDps = 0.f;
	CurrentTickInterval = 0.5f;
	EndTime = 0.0;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DotTickTH);
	}
}