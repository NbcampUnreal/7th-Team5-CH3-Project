// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/SpecialSkillComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "SpecialSkillPresentation.h"
#include "SpecialSkillExecution.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"

USpecialSkillComponent::USpecialSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpecialSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedOwner = GetOwner();
}

void USpecialSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		FTimerManager& TM = W->GetTimerManager();
		TM.ClearTimer(TelegraphTH);
		TM.ClearTimer(CastTH);
	}

	bQueuedExecuteNextTick = false;
	bQueuedEndNextTick = false;

	Super::EndPlay(EndPlayReason);
}

double USpecialSkillComponent::Now() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetTimeSeconds() : 0.0;
}

bool USpecialSkillComponent::IsTargetValid(AActor* Target) const
{
	return IsValid(Target) && !Target->IsActorBeingDestroyed();
}

const FPotatoMonsterSpecialSkillPresetRow* USpecialSkillComponent::FindRow(FName SkillId) const
{
	if (!SkillPresetTable || SkillId.IsNone()) return nullptr;

	return SkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
		SkillId, TEXT("USpecialSkillComponent::FindRow"));
}

void USpecialSkillComponent::SetState(ESpecialSkillState NewState)
{
	if (State == NewState) return;
	State = NewState;
	OnStateChanged.Broadcast(ActiveSkillId, State);
}

float USpecialSkillComponent::ComputeBaseDamage(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	return FMath::Max(0.f, DefaultBaseDamage);
}

float USpecialSkillComponent::ComputeFinalDamage(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	const float Base = ComputeBaseDamage(Row);
	const float Mul = Row.DamageMultiplier * SpecialDamageScale;
	return FMath::Max(0.f, Base * Mul);
}

float USpecialSkillComponent::ComputeFinalCooldown(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	return FMath::Max(0.f, Row.Cooldown * SpecialCooldownScale);
}

bool USpecialSkillComponent::CheckTrigger(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;

	// ✅ PlayerOnly / StructureOnly 안정 필터 (기존 로직 유지)
	auto PassTargetTypeFilter = [&](AActor* T) -> bool
	{
		if (!IsTargetValid(T)) return false;

		switch (Row.TargetType)
		{
		case EMonsterSpecialTargetType::PlayerOnly:
		{
			if (T->ActorHasTag(TEXT("Player"))) return true;

			if (APawn* P = Cast<APawn>(T))
			{
				return (Cast<APlayerController>(P->GetController()) != nullptr);
			}
			return false;
		}

		case EMonsterSpecialTargetType::StructureOnly:
			{
				if (const APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(T))
				{
					return (S->StructureData && S->StructureData->bIsDestructible && S->CurrentHealth > 0.f);
				}
				return false;
			}

		default:
			return true;
		}
	};

	// TargetType 반영
	if (Row.TargetType == EMonsterSpecialTargetType::Self)
	{
		Target = Owner;
	}
	else if (Row.TargetType != EMonsterSpecialTargetType::Location)
	{
		if (!IsTargetValid(Target)) return false;
		if (!PassTargetTypeFilter(Target)) return false;
	}

	// 거리 체크
	if (Target && Row.TargetType != EMonsterSpecialTargetType::Location)
	{
		const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

		if (Row.Trigger == EMonsterSpecialTrigger::OnNearTarget && Row.TriggerRange > 0.f)
		{
			if (Dist > Row.TriggerRange) return false;
		}

		if (Row.MinRange > 0.f && Dist < Row.MinRange) return false;
		if (Row.MaxRange > 0.f && Dist > Row.MaxRange) return false;
	}

	// LoS(옵션)
	if (Row.bRequireLineOfSight && Target && Row.TargetType != EMonsterSpecialTargetType::Location)
	{
		UWorld* World = GetWorld();
		if (!World) return false;

		FHitResult Hit;
		const FVector Start = Owner->GetActorLocation();
		const FVector End = Target->GetActorLocation();

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillLoS), false, Owner);
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		if (bHit && Hit.GetActor() != Target)
		{
			return false;
		}
	}

	return true;
}

void USpecialSkillComponent::ArmCooldown(const FName SkillId, float CooldownSeconds)
{
	NextReadyTimeBySkill.Add(SkillId, Now() + FMath::Max(0.f, CooldownSeconds));
}

bool USpecialSkillComponent::ConsumeHitOnce(AActor* Victim)
{
	if (!IsValid(Victim)) return false;

	TWeakObjectPtr<AActor> Key(Victim);
	if (HitOnceSet.Contains(Key)) return false;
	HitOnceSet.Add(Key);
	return true;
}

// -----------------------------
// Presentation facade
// -----------------------------
void USpecialSkillComponent::PlayPresentation_Telegraph(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	FSpecialSkillPresentation::PlayTelegraph(this, Row, Target);
}

void USpecialSkillComponent::PlayPresentation_Cast(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	FSpecialSkillPresentation::PlayCast(this, Row, Target);
}

void USpecialSkillComponent::PlayPresentation_Execute(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	FSpecialSkillPresentation::PlayExecute(this, Row, Target);
}

void USpecialSkillComponent::PlayPresentation_End(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	FSpecialSkillPresentation::PlayEnd(this, Row, Target);
}

// -----------------------------
// Execution facade
// -----------------------------
void USpecialSkillComponent::ExecuteResolved(const FPotatoMonsterSpecialSkillPresetRow& RowCopy, AActor* ResolvedTarget)
{
	FSpecialSkillExecution::Execute(this, RowCopy, ResolvedTarget);
}

// -----------------------------
// Inject once (정석)
// -----------------------------
void USpecialSkillComponent::InitFromFinalStats(const FPotatoMonsterFinalStats& Stats)
{
	DefaultSkillId       = Stats.DefaultSpecialSkillId;
	SpecialCooldownScale = Stats.SpecialCooldownScale;
	SpecialDamageScale   = Stats.SpecialDamageScale;

	// base damage policy: AttackDamage를 base로 쓰는 게 보통 가장 직관적
	DefaultBaseDamage = Stats.AttackDamage;

	// Proc
	bEnableOnAttackProc   = Stats.bEnableOnAttackSpecialProc;
	OnAttackProcChance    = FMath::Clamp(Stats.OnAttackSpecialChance, 0.f, 1.f);
	OnAttackProcCooldown  = FMath::Max(0.f, Stats.OnAttackSpecialProcCooldown);

	// reset attack proc gate
	NextAttackProcReadyTime = 0.0;
}

// -----------------------------
// Trigger wrappers (정석)
// -----------------------------
bool USpecialSkillComponent::TryStartOnCooldown(AActor* Target)
{
	if (DefaultSkillId.IsNone()) return false;
	return TryStartSkill(DefaultSkillId, Target);
}

bool USpecialSkillComponent::TryStartOnHit(AActor* Target)
{
	if (DefaultSkillId.IsNone()) return false;
	return TryStartSkill(DefaultSkillId, Target);
}

bool USpecialSkillComponent::TryStartOnDeath(AActor* Target)
{
	if (DefaultSkillId.IsNone()) return false;
	return TryStartSkill(DefaultSkillId, Target);
}

bool USpecialSkillComponent::PassAttackProcGate()
{
	if (!bEnableOnAttackProc) return false;

	const double T = Now();
	if (T < NextAttackProcReadyTime) return false;

	// chance
	if (OnAttackProcChance <= 0.f) return false;
	const float R = FMath::FRand();
	if (R > OnAttackProcChance) return false;

	// consume cooldown
	NextAttackProcReadyTime = T + OnAttackProcCooldown;
	return true;
}

bool USpecialSkillComponent::TryStartOnAttackProc(AActor* Target)
{
	if (DefaultSkillId.IsNone()) return false;
	if (!PassAttackProcGate()) return false;
	return TryStartSkill(DefaultSkillId, Target);
}

// -----------------------------
// Public API (low-level engine)
// -----------------------------
bool USpecialSkillComponent::CanTryStartSkill(FName SkillId) const
{
	if (SkillId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] CanTryStartSkill FAIL: SkillId=None"));
		return false;
	}
	if (IsBusy())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] CanTryStartSkill FAIL: Busy State=%d Skill=%s"),
			(int32)State, *SkillId.ToString());
		return false;
	}
	if (!IsSkillReady(SkillId))
	{
		const double* Next = NextReadyTimeBySkill.Find(SkillId);
		UE_LOG(LogTemp, Warning, TEXT("[Skill] CanTryStartSkill FAIL: Cooldown Skill=%s Now=%.2f Next=%.2f"),
			*SkillId.ToString(), Now(), Next ? *Next : -1.0);
		return false;
	}
	const FPotatoMonsterSpecialSkillPresetRow* Row = FindRow(SkillId);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] CanTryStartSkill FAIL: RowNotFound Skill=%s Table=%s"),
			*SkillId.ToString(), *GetNameSafe(SkillPresetTable));
		return false;
	}
	return true;
}

bool USpecialSkillComponent::TryStartSkill(FName SkillId, AActor* InTarget)
{
	if (!CanTryStartSkill(SkillId)) return false;

	const FPotatoMonsterSpecialSkillPresetRow* Row = FindRow(SkillId);
	if (!Row) return false;

	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return false;

	// ✅ 세션 시작
	++SkillSessionId;
	const uint32 ThisSession = SkillSessionId;

	// 타이머 정리
	if (UWorld* W = GetWorld())
	{
		FTimerManager& TM = W->GetTimerManager();
		TM.ClearTimer(TelegraphTH);
		TM.ClearTimer(CastTH);
	}

	bQueuedExecuteNextTick = false;
	bQueuedEndNextTick = false;
	LastCancelReason = ESpecialSkillCancelReason::None;

	ActiveSkillId = SkillId;
	CurrentTarget = InTarget;

	if (!CheckTrigger(*Row, CurrentTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] CheckTrigger FAIL Skill=%s Target=%s"),
			*SkillId.ToString(), *GetNameSafe(CurrentTarget));

		ActiveSkillId = NAME_None;
		CurrentTarget = nullptr;
		return false;
	}

	HitOnceSet.Reset();

	BeginTelegraph(*Row); // 내부에서 세션 체크하도록 수정할 것
	return true;
}

bool USpecialSkillComponent::IsBusy() const
{
	return State != ESpecialSkillState::Idle;
}

// -----------------------------
// State machine
// -----------------------------
void USpecialSkillComponent::BeginTelegraph(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	AActor* Owner = GetOwner();
	UWorld* W = GetWorld();
	if (!Owner || Owner->IsActorBeingDestroyed() || !W)
	{
		CancelSkill(ESpecialSkillCancelReason::OwnerInvalid);
		return;
	}

	const uint32 ThisSession = SkillSessionId;

	SetState(ESpecialSkillState::Telegraph);
	PlayPresentation_Telegraph(Row, CurrentTarget);
	BP_OnTelegraphBegin(ActiveSkillId);

	if (Row.TelegraphTime <= 0.f)
	{
		BeginCast(Row);
		return;
	}

	W->GetTimerManager().SetTimer(
		TelegraphTH,
		FTimerDelegate::CreateWeakLambda(this, [this, Row, ThisSession]()
		{
			if (!IsValid(this) || !IsSession(ThisSession)) return;

			AActor* Owner2 = GetOwner();
			if (!Owner2 || Owner2->IsActorBeingDestroyed())
			{
				CancelSkill(ESpecialSkillCancelReason::OwnerInvalid);
				return;
			}

			if (Row.TargetType != EMonsterSpecialTargetType::Self
				&& Row.TargetType != EMonsterSpecialTargetType::Location
				&& !IsTargetValid(CurrentTarget))
			{
				CancelSkill(ESpecialSkillCancelReason::TargetInvalid);
				return;
			}

			BeginCast(Row);
		}),
		Row.TelegraphTime,
		false
	);
}

void USpecialSkillComponent::BeginCast(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	AActor* Owner = GetOwner();
	UWorld* W = GetWorld();
	if (!Owner || Owner->IsActorBeingDestroyed() || !W)
	{
		CancelSkill(ESpecialSkillCancelReason::OwnerInvalid);
		return;
	}

	SetState(ESpecialSkillState::Casting);

	PlayPresentation_Cast(Row, CurrentTarget);
	BP_OnCastBegin(ActiveSkillId);

	if (Row.CastTime <= 0.f)
	{
		QueueExecuteNextTick(Row);
		return;
	}

	W->GetTimerManager().SetTimer(
		CastTH,
		FTimerDelegate::CreateWeakLambda(this, [this, Row]()
		{
			AActor* Owner2 = GetOwner();
			if (!Owner2 || Owner2->IsActorBeingDestroyed())
			{
				CancelSkill(ESpecialSkillCancelReason::OwnerInvalid);
				return;
			}

			if (Row.TargetType != EMonsterSpecialTargetType::Self
				&& Row.TargetType != EMonsterSpecialTargetType::Location
				&& !IsTargetValid(CurrentTarget))
			{
				CancelSkill(ESpecialSkillCancelReason::TargetInvalid);
				return;
			}

			QueueExecuteNextTick(Row);
		}),
		Row.CastTime,
		false
	);
}

void USpecialSkillComponent::QueueExecuteNextTick(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	if (bQueuedExecuteNextTick) return;
	bQueuedExecuteNextTick = true;

	AActor* Owner = GetOwner();
	UWorld* W = GetWorld();
	if (!Owner || Owner->IsActorBeingDestroyed() || !W)
	{
		bQueuedExecuteNextTick = false;
		QueueEndNextTick(true);
		return;
	}

	const uint32 ThisSession = SkillSessionId;

	SetState(ESpecialSkillState::Executing);

	const FPotatoMonsterSpecialSkillPresetRow RowCopy = Row;

	if (Row.bExecuteOnNextTick)
	{
		W->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this, RowCopy, ThisSession]()
			{
				if (!IsValid(this) || !IsSession(ThisSession)) return;
				ExecuteSkill_Internal(RowCopy);
			})
		);
	}
	else
	{
		ExecuteSkill_Internal(RowCopy);
	}
}

void USpecialSkillComponent::ExecuteSkill_Internal(FPotatoMonsterSpecialSkillPresetRow RowCopy)
{
	bQueuedExecuteNextTick = false;

	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed())
	{
		QueueEndNextTick(true);
		return;
	}

	AActor* Target = CurrentTarget;
	if (RowCopy.TargetType == EMonsterSpecialTargetType::Self)
	{
		Target = Owner;
	}

	if (RowCopy.TargetType != EMonsterSpecialTargetType::Self
		&& RowCopy.TargetType != EMonsterSpecialTargetType::Location
		&& !IsTargetValid(Target))
	{
		QueueEndNextTick(true);
		return;
	}

	PlayPresentation_Execute(RowCopy, Target);
	BP_OnExecute(ActiveSkillId);

	ExecuteResolved(RowCopy, Target);

	ArmCooldown(ActiveSkillId, ComputeFinalCooldown(RowCopy));
	QueueEndNextTick(false);
}

void USpecialSkillComponent::QueueEndNextTick(bool bCancelled)
{
	if (bQueuedEndNextTick) return;
	bQueuedEndNextTick = true;

	UWorld* W = GetWorld();
	if (!W)
	{
		EndSkill_Internal(bCancelled);
		return;
	}

	W->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &USpecialSkillComponent::EndSkill_Internal_NextTick, bCancelled)
	);
}

void USpecialSkillComponent::EndSkill_Internal_NextTick(bool bCancelled)
{
	EndSkill_Internal(bCancelled);
}

void USpecialSkillComponent::EndSkill_Internal(bool bCancelled)
{
	// End도 현재 세션에 대해서만 처리하는 게 안전함
	// (만약 Cancel 이후 곧바로 새 스킬 시작하면, 이전 End가 새 스킬을 지워버릴 수 있음)
	// -> 여기서는 호출 전에 세션검사 하는 패턴을 추천. (NextTick 예약 때 캡처)

	bQueuedEndNextTick = false;

	if (ActiveSkillId != NAME_None)
	{
		if (const FPotatoMonsterSpecialSkillPresetRow* Row = FindRow(ActiveSkillId))
		{
			AActor* Resolved = ResolveTargetForRow(*Row, CurrentTarget);

			// ✅ Target이 무효/Null이어도 End 프레젠테이션이 안전해야 함.
			// Presentation이 Actor 접근한다면 여기서 valid만 넘겨라.
			if (Row->TargetType == EMonsterSpecialTargetType::Location || IsTargetValid(Resolved) || Row->TargetType == EMonsterSpecialTargetType::Self)
			{
				PlayPresentation_End(*Row, Resolved);
			}
		}
	}

	if (IsValid(this))
	{
		BP_OnEnd(ActiveSkillId, bCancelled);
	}

	ActiveSkillId = NAME_None;
	CurrentTarget = nullptr;
	SetState(ESpecialSkillState::Idle);
}
void USpecialSkillComponent::CancelSkill(ESpecialSkillCancelReason Reason)
{
	LastCancelReason = Reason;

	UE_LOG(LogTemp, Warning, TEXT("[Skill] CancelSkill Skill=%s Reason=%d State=%d"),
		*ActiveSkillId.ToString(), (int32)Reason, (int32)State);

	if (UWorld* W = GetWorld())
	{
		FTimerManager& TM = W->GetTimerManager();
		TM.ClearTimer(TelegraphTH);
		TM.ClearTimer(CastTH);
	}

	bQueuedExecuteNextTick = false;

	if (!bQueuedEndNextTick)
	{
		QueueEndNextTick(true);
	}
}
void USpecialSkillComponent::CancelActiveSkill()
{
	CancelSkill(ESpecialSkillCancelReason::None);
}

bool USpecialSkillComponent::IsSkillReady(FName SkillId) const
{
	const double* Next = NextReadyTimeBySkill.Find(SkillId);
	return (!Next) || (Now() >= *Next);
}

AActor* USpecialSkillComponent::ResolveTargetForRow(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* InTarget) const
{
	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return nullptr;

	switch (Row.TargetType)
	{
	case EMonsterSpecialTargetType::Self:
		return Owner;

	case EMonsterSpecialTargetType::Location:
		// Location 스킬이면 Actor 타겟을 "의미상" 안 쓴다고 가정
		// (Execution/Presentation 쪽이 Location을 따로 계산해야 함)
		return nullptr;

	default:
		return InTarget;
	}
}