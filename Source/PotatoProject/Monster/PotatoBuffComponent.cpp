// Combat/PotatoBuffComponent.cpp
#include "PotatoBuffComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

UPotatoBuffComponent::UPotatoBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	// 버프 만료 정리용(가볍게 0.25s)
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			TickTH,
			this,
			&UPotatoBuffComponent::TickBuffs,
			0.25f,
			true
		);
	}
}

void UPotatoBuffComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(TickTH);
	}
	Super::EndPlay(EndPlayReason);
}

double UPotatoBuffComponent::Now() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetTimeSeconds() : 0.0;
}

void UPotatoBuffComponent::ApplyBuff(FName BuffId, EPotatoBuffType Type, float Magnitude, float Duration, AActor* Source)
{
	if (Type == EPotatoBuffType::None) return;
	if (Duration <= 0.f) return;

	const double NowTime = Now();
	const double NewEnd = NowTime + (double)Duration;

	// 같은 BuffId+Type이 있으면 갱신(정책: end time 연장 + magnitude는 더 큰 값 우선)
	for (FActiveBuff& B : ActiveBuffs)
	{
		if (B.Type == Type && B.BuffId == BuffId)
		{
			B.EndTime = FMath::Max(B.EndTime, NewEnd);
			B.Magnitude = FMath::Max(B.Magnitude, Magnitude);
			B.Duration = Duration;
			B.SourceActor = Source;
			return;
		}
	}

	FActiveBuff NewBuff;
	NewBuff.BuffId = BuffId;
	NewBuff.Type = Type;
	NewBuff.Magnitude = Magnitude;
	NewBuff.Duration = Duration;
	NewBuff.EndTime = NewEnd;
	NewBuff.SourceActor = Source;

	ActiveBuffs.Add(NewBuff);
}

bool UPotatoBuffComponent::HasBuffType(EPotatoBuffType Type) const
{
	const double NowTime = Now();
	for (const FActiveBuff& B : ActiveBuffs)
	{
		if (B.Type == Type && B.EndTime > NowTime)
			return true;
	}
	return false;
}

float UPotatoBuffComponent::GetBestDamageReduction() const
{
	// DamageReduction은 “감소율”로 취급 (0.9 = 90% 감소)
	const double NowTime = Now();
	float Best = 0.f;

	for (const FActiveBuff& B : ActiveBuffs)
	{
		if (B.Type != EPotatoBuffType::DamageReduction) continue;
		if (B.EndTime <= NowTime) continue;
		Best = FMath::Max(Best, B.Magnitude);
	}

	return FMath::Clamp(Best, 0.f, 0.99f);
}

void UPotatoBuffComponent::ClearAllBuffs()
{
	ActiveBuffs.Reset();
}

void UPotatoBuffComponent::TickBuffs()
{
	RemoveExpired(Now());
}

void UPotatoBuffComponent::RemoveExpired(double NowTime)
{
	for (int32 i = ActiveBuffs.Num() - 1; i >= 0; --i)
	{
		if (ActiveBuffs[i].EndTime <= NowTime)
		{
			ActiveBuffs.RemoveAt(i);
		}
	}
}