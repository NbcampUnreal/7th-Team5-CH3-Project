// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/SpecialSkillComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

USpecialSkillComponent::USpecialSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Tick 불필요 (타이머/NextTick 기반)
}

void USpecialSkillComponent::BeginPlay()
{
	Super::BeginPlay();
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

void USpecialSkillComponent::SetTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

bool USpecialSkillComponent::IsSkillReady(FName SkillId) const
{
	const double* Next = NextReadyTimeBySkill.Find(SkillId);
	return !Next || Now() >= *Next;
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

bool USpecialSkillComponent::IsBusy() const
{
	return State != ESpecialSkillState::Idle;
}

EMonsterSpecialExecution USpecialSkillComponent::ResolveExecution(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	if (Row.Execution != EMonsterSpecialExecution::None)
	{
		return Row.Execution;
	}

	// 하위호환: Shape 기반 역추론
	switch (Row.Shape)
	{
	case EMonsterSpecialShape::Projectile: return EMonsterSpecialExecution::Projectile;
	case EMonsterSpecialShape::Aura:       return EMonsterSpecialExecution::ContactDOT;
	case EMonsterSpecialShape::SelfBuff:   return EMonsterSpecialExecution::SelfBuff;
	case EMonsterSpecialShape::Circle:
	case EMonsterSpecialShape::Cone:
	case EMonsterSpecialShape::Line:
		return EMonsterSpecialExecution::InstantAoE;
	default:
		return EMonsterSpecialExecution::None;
	}
}

bool USpecialSkillComponent::CheckTrigger(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;

	// TargetType 반영
	if (Row.TargetType == EMonsterSpecialTargetType::Self)
	{
		Target = Owner;
	}
	else if (Row.TargetType != EMonsterSpecialTargetType::Location)
	{
		if (!IsTargetValid(Target)) return false;
	}

	// 거리 체크
	if (Target && Row.TargetType != EMonsterSpecialTargetType::Location)
	{
		const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

		if (Row.Trigger == EMonsterSpecialTrigger::OnNearTarget && Row.TriggerRange > 0.f)
		{
			if (Dist > Row.TriggerRange) return false;
		}

		if (Dist < Row.MinRange) return false;
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

bool USpecialSkillComponent::CanTryStartSkill(FName SkillId) const
{
	// "서비스에서 쿨다운 조건일 때만 TryStart" 목적
	// - Trigger/Target 체크는 TryStartSkill 내부에서 수행
	if (SkillId.IsNone()) return false;

	if (IsBusy()) return false;
	if (!IsSkillReady(SkillId)) return false;

	const FPotatoMonsterSpecialSkillPresetRow* Row = FindRow(SkillId);
	if (!Row) return false;

	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return false;

	return true;
}

bool USpecialSkillComponent::TryStartSkill(FName SkillId, AActor* InTarget)
{
	if (IsBusy()) return false;
	if (!IsSkillReady(SkillId)) return false;

	const FPotatoMonsterSpecialSkillPresetRow* Row = FindRow(SkillId);
	if (!Row) return false;

	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed()) return false;

	ActiveSkillId = SkillId;
	CurrentTarget = InTarget;

	if (!CheckTrigger(*Row, CurrentTarget))
	{
		ActiveSkillId = NAME_None;
		CurrentTarget = nullptr; // ✅ 실패 시 타겟 오염 방지
		return false;
	}

	// 타이머 정리
	if (UWorld* W = GetWorld())
	{
		FTimerManager& TM = W->GetTimerManager();
		TM.ClearTimer(TelegraphTH);
		TM.ClearTimer(CastTH);
	}

	bQueuedExecuteNextTick = false;
	bQueuedEndNextTick = false;

	BeginTelegraph(*Row);
	return true;
}

void USpecialSkillComponent::BeginTelegraph(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	AActor* Owner = GetOwner();
	UWorld* W = GetWorld();
	if (!Owner || Owner->IsActorBeingDestroyed() || !W)
	{
		CancelSkill(ESpecialSkillCancelReason::OwnerInvalid);
		return;
	}

	SetState(ESpecialSkillState::Telegraph);
	BP_OnTelegraphBegin(ActiveSkillId);

	if (Row.TelegraphTime <= 0.f)
	{
		BeginCast(Row);
		return;
	}

	W->GetTimerManager().SetTimer(
		TelegraphTH,
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

	SetState(ESpecialSkillState::Executing);

	const FPotatoMonsterSpecialSkillPresetRow RowCopy = Row;

	if (Row.bExecuteOnNextTick)
	{
		W->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &USpecialSkillComponent::ExecuteSkill_Internal, RowCopy)
		);
	}
	else
	{
		ExecuteSkill_Internal(RowCopy);
	}
}

bool USpecialSkillComponent::PassesFxDistanceGate(const FVector& SpawnLoc, const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	if (Row.MaxFxDistance <= 0.f) return true;

	UWorld* World = GetWorld();
	if (!World) return true;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return true;

	return FVector::Dist(PlayerPawn->GetActorLocation(), SpawnLoc) <= Row.MaxFxDistance;
}

UClass* USpecialSkillComponent::ResolveProjectileClass(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	if (Row.ProjectileClass.IsNull()) return nullptr;
	if (UClass* Loaded = Row.ProjectileClass.Get()) return Loaded;
	return Row.ProjectileClass.LoadSynchronous();
}

FTransform USpecialSkillComponent::ResolveProjectileSpawnTransform(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return FTransform::Identity;

	FVector Loc = Owner->GetActorLocation();
	FRotator Rot = Owner->GetActorRotation();

	const ACharacter* AsChar = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Skel = AsChar ? AsChar->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();

	if (Skel && !Row.SpawnSocket.IsNone() && Skel->DoesSocketExist(Row.SpawnSocket))
	{
		const FTransform SocketXf = Skel->GetSocketTransform(Row.SpawnSocket, RTS_World);
		Loc = SocketXf.GetLocation();
		Rot = SocketXf.GetRotation().Rotator();
	}

	Loc += Rot.RotateVector(Row.SpawnOffset);
	return FTransform(Rot, Loc);
}

AActor* USpecialSkillComponent::SpawnProjectileFromPreset(const FPotatoMonsterSpecialSkillPresetRow& Row) const
{
	UClass* ProjClass = ResolveProjectileClass(Row);
	if (!ProjClass) return nullptr;

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || Owner->IsActorBeingDestroyed()) return nullptr;

	const FTransform SpawnXf = ResolveProjectileSpawnTransform(Row);

	if (!PassesFxDistanceGate(SpawnXf.GetLocation(), Row))
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<AActor>(ProjClass, SpawnXf, Params);
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

	BP_OnExecute(ActiveSkillId);

	const EMonsterSpecialExecution Exec = ResolveExecution(RowCopy);

	switch (Exec)
	{
	case EMonsterSpecialExecution::Projectile:
		SpawnProjectileFromPreset(RowCopy);
		break;
	default:
		break;
	}

	ArmCooldown(ActiveSkillId, RowCopy.Cooldown);
	QueueEndNextTick(false);
}

void USpecialSkillComponent::ArmCooldown(const FName SkillId, float Cooldown)
{
	NextReadyTimeBySkill.Add(SkillId, Now() + FMath::Max(0.f, Cooldown));
}

void USpecialSkillComponent::EndSkill_Internal_NextTick(bool bCancelled)
{
	EndSkill_Internal(bCancelled);
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

void USpecialSkillComponent::EndSkill_Internal(bool bCancelled)
{
	bQueuedEndNextTick = false;

	if (IsValid(this))
	{
		BP_OnEnd(ActiveSkillId, bCancelled);
	}

	ActiveSkillId = NAME_None;
	CurrentTarget = nullptr; // ✅ 종료 시 타겟 정리
	SetState(ESpecialSkillState::Idle);
}

void USpecialSkillComponent::CancelSkill(ESpecialSkillCancelReason Reason)
{
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