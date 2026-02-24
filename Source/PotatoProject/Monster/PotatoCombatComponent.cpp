// ============================================================================
// PotatoCombatComponent.cpp (UE5.6) - AnimSet 기반 + SFX(거리게이트/버짓/쿨다운) 포함 완성본
// ============================================================================

#include "PotatoCombatComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "PotatoMonster.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/TargetPoint.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "GameFramework/DamageType.h"

#include "PotatoProjectileDamageable.h"

// AnimSet
#include "PotatoMonsterAnimSet.h"

#include "FXUtils/PotatoFXUtils.h"

static const FName TAG_LanePoint(TEXT("LanePoint"));

// ------------------------------------------------------------
// Helper: Target의 충돌 Bounds (Origin/Extent) 가져오기
// - true: collision이 있는 컴포넌트 기준(가능한 경우)
// - 실패/빈값 대비 fallback 포함
// ------------------------------------------------------------
static void GetTargetBoundsSafe(AActor* Target, FVector& OutOrigin, FVector& OutExtent)
{
	if (!Target)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		return;
	}

	Target->GetActorBounds(true, OutOrigin, OutExtent);

	if (OutExtent.IsNearlyZero())
	{
		Target->GetActorBounds(false, OutOrigin, OutExtent);
	}
}

UPotatoCombatComponent::UPotatoCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoCombatComponent::InitFromStats(const FPotatoMonsterFinalStats& InStats)
{
	Stats = InStats;
}

const UPotatoMonsterAnimSet* UPotatoCombatComponent::GetAnimSet() const
{
	return Stats.AnimSet;
}

// ------------------------------------------------------------
// 최종 허용 타겟 판정 (원본 유지)
// ------------------------------------------------------------
static bool IsAllowedAttackTarget(const APotatoMonster* Monster, const AActor* Target)
{
	if (!Monster || !Target) return false;

	if (Target->ActorHasTag(TAG_LanePoint))
		return false;

	if (Target->IsA(ATargetPoint::StaticClass()))
		return false;

	// 1) Player 허용
	if (const APawn* Pawn = Cast<APawn>(Target))
	{
		if (Pawn->IsPlayerControlled())
			return true;
	}

	// 2) Warehouse 허용
	if (Monster->WarehouseActor.Get() && Target == Monster->WarehouseActor.Get())
		return true;

	// 3) 파괴가능 구조물 허용
	if (const APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(Target))
	{
		if (S->StructureData && S->StructureData->bIsDestructible)
			return true;
	}

	return false;
}
// ------------------------------------------------------------
// 몽타주 종료 시 공격상태 해제 보장 (AnimSet 기반)
// ------------------------------------------------------------
void UPotatoCombatComponent::OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (AnimSet && AnimSet->BasicAttackMontage && Montage != AnimSet->BasicAttackMontage)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] MontageEnded -> EndBasicAttack (Interrupted=%d)"), bInterrupted ? 1 : 0);
	EndBasicAttack();
}

bool UPotatoCombatComponent::RequestBasicAttack(AActor* Target)
{
	if (!GetWorld() || !Target || !GetOwner())
		return false;

	const double Now = GetWorld()->GetTimeSeconds();

	if (Now < NextAttackTime)
		return false;

	if (bIsAttacking)
		return false;

	APotatoMonster* Monster = Cast<APotatoMonster>(GetOwner());
	if (!Monster)
	{
		EndBasicAttack();
		return false;
	}

	// (A) 공격 시작 직전 최종 방어선
	if (!IsAllowedAttackTarget(Monster, Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Reject target at Request: %s (%s) LanePoint=%d"),
			*GetNameSafe(Target),
			*GetNameSafe(Target->GetClass()),
			Target->ActorHasTag(TAG_LanePoint) ? 1 : 0);

		EndBasicAttack();
		return false;
	}

	if (!Target->CanBeDamaged())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Reject target (CanBeDamaged=false): %s (%s)"),
			*GetNameSafe(Target),
			*GetNameSafe(Target->GetClass()));

		EndBasicAttack();
		return false;
	}

	USkeletalMeshComponent* Mesh = Monster->GetMesh();
	UAnimInstance* AnimInst = Mesh ? Mesh->GetAnimInstance() : nullptr;

	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (!AnimInst || !AnimSet || !AnimSet->BasicAttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] No AnimSet or BasicAttackMontage. Owner=%s AnimSet=%s"),
			*GetNameSafe(GetOwner()), *GetNameSafe(AnimSet));
		EndBasicAttack();
		return false;
	}

	const float PlayRate = 1.0f;

	const float Played = AnimInst->Montage_Play(AnimSet->BasicAttackMontage, PlayRate);
	if (Played <= 0.f)
	{
		EndBasicAttack();
		return false;
	}

	// 여기서부터 공격 상태 ON
	PendingBasicTarget = Target;
	bIsAttacking = true;

	//  AttackStart SFX
	if (Now - LastAttackStartSFXTime >= AttackStartSFXCooldown)
	{
		const FVector Loc = GetOwner()->GetActorLocation();

		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, Loc,
			AttackSFXNearDistance, AttackSFXFarDistance, AttackSFXFarChance);

		const bool bBudgetOK = PotatoFX::PassGlobalBudget((float)Now, CombatSFXGlobalWindowSec, CombatSFXGlobalMaxCount,
			PotatoFX::EGlobalBudgetChannel::CombatSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AnimSet->AttackStartSFX, Loc);
			LastAttackStartSFXTime = Now;
		}
	}

	// 몽타주 종료 시 EndBasicAttack 보장
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UPotatoCombatComponent::OnBasicAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, AnimSet->BasicAttackMontage);

	// 공격 간격: 데이터화된 공속 우선
	const float BaseInterval = FMath::Max(0.01f, AnimSet->BasicAttackInterval);
	const float Interval = FMath::Max(0.05f, BaseInterval * AttackIntervalScale + AttackIntervalExtra);
	NextAttackTime = Now + (double)Interval;

	UE_LOG(LogTemp, Warning, TEXT("[Combat] RequestBasicAttack OK -> Target=%s Class=%s | Interval=%.3f | AnimSet=%s"),
		*GetNameSafe(Target), *GetNameSafe(Target->GetClass()), Interval, *GetNameSafe(AnimSet));

	return true;
}

void UPotatoCombatComponent::ApplyPendingBasicDamage()
{
	UE_LOG(LogTemp, Warning, TEXT("[Combat] ApplyPendingBasicDamage() CALLED"));
	bQueuedApplyDamageNextTick = false;
	APotatoMonster* Monster = Cast<APotatoMonster>(GetOwner());
	AActor* Target = PendingBasicTarget.Get();

	if (!Monster || !IsValid(Target))
	{
		EndBasicAttack();
		return;
	}

	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (!AnimSet)
	{
		EndBasicAttack();
		return;
	}

	// 원거리면 근접 데미지 노티파이는 무시(정책)
	if (AnimSet->bIsRanged)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] ApplyPendingBasicDamage ignored (Ranged). Target=%s"), *GetNameSafe(Target));
		return;
	}

	// (B) Notify 시점 재검증
	if (!IsAllowedAttackTarget(Monster, Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Pending target invalid at Apply -> cancel: %s (%s) LanePoint=%d"),
			*GetNameSafe(Target),
			*GetNameSafe(Target->GetClass()),
			Target->ActorHasTag(TAG_LanePoint) ? 1 : 0);

		EndBasicAttack();
		return;
	}

	if (!Target->CanBeDamaged())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Target CanBeDamaged=false at Apply -> cancel: %s (%s)"),
			*GetNameSafe(Target),
			*GetNameSafe(Target->GetClass()));

		EndBasicAttack();
		return;
	}

	if (!IsTargetInRange(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Out of range at Apply -> EndBasicAttack. Target=%s"), *GetNameSafe(Target));
		EndBasicAttack();
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	//  AttackHit SFX (근접 타격)
	if (Now - LastAttackHitSFXTime >= AttackHitSFXCooldown)
	{
		const FVector Loc = Target->GetActorLocation();

		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, Loc,
			AttackSFXNearDistance, AttackSFXFarDistance, AttackSFXFarChance);

		const bool bBudgetOK = PotatoFX::PassGlobalBudget((float)Now, CombatSFXGlobalWindowSec, CombatSFXGlobalMaxCount,
			PotatoFX::EGlobalBudgetChannel::CombatSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AnimSet->AttackHitSFX, Loc);
			LastAttackHitSFXTime = Now;
		}
	}

	ApplyBasicDamage(Target);

	// 1타 정책 유지
	EndBasicAttack();
}

// ------------------------------------------------------------
// 원거리 발사 Notify용
// ------------------------------------------------------------
void UPotatoCombatComponent::FirePendingRangedProjectile()
{
	UE_LOG(LogTemp, Warning, TEXT("[Combat] FirePendingRangedProjectile() CALLED"));
	bQueuedFireProjectileNextTick = false;
	APotatoMonster* Monster = Cast<APotatoMonster>(GetOwner());
	AActor* Target = PendingBasicTarget.Get();

	if (!Monster || !IsValid(Target))
	{
		EndBasicAttack();
		return;
	}

	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (!AnimSet || !AnimSet->bIsRanged)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] FireProjectile ignored (Not Ranged). AnimSet=%s"), *GetNameSafe(AnimSet));
		return;
	}

	// Notify 시점 재검증
	if (!IsAllowedAttackTarget(Monster, Target) || !Target->CanBeDamaged())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] FireProjectile target invalid -> cancel. Target=%s"), *GetNameSafe(Target));
		EndBasicAttack();
		return;
	}

	// 사거리 밖이면 종료
	if (!IsTargetInRange(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Out of range at FireProjectile -> EndBasicAttack. Target=%s"), *GetNameSafe(Target));
		EndBasicAttack();
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	//  ProjectileFire SFX (머즐 위치 우선)
	if (Now - LastProjectileFireSFXTime >= ProjectileFireSFXCooldown)
	{
		FVector MuzzleLoc;
		FRotator MuzzleRot;
		const bool bHasMuzzle = GetMuzzleTransform(MuzzleLoc, MuzzleRot);

		const FVector Loc = bHasMuzzle ? MuzzleLoc : Monster->GetActorLocation();

		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, Loc,
			AttackSFXNearDistance, AttackSFXFarDistance, AttackSFXFarChance);

		const bool bBudgetOK = PotatoFX::PassGlobalBudget((float)Now, CombatSFXGlobalWindowSec, CombatSFXGlobalMaxCount,
			PotatoFX::EGlobalBudgetChannel::CombatSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AnimSet->ProjectileFireSFX, Loc);
			LastProjectileFireSFXTime = Now;
		}
	}

	SpawnProjectileToTarget(Target);

	// 원거리도 1회 공격 정책이면 종료
	EndBasicAttack();
}

void UPotatoCombatComponent::EndBasicAttack()
{
	bIsAttacking = false;
	PendingBasicTarget.Reset();
}

// ------------------------------------------------------------
// 사거리 체크 개선: Bounds 기반(충돌 기준) 표면 거리 근사
// ------------------------------------------------------------
bool UPotatoCombatComponent::IsTargetInRange(AActor* Target) const
{
	if (!Target || !GetOwner()) return false;

	const FVector From = GetOwner()->GetActorLocation();
	const float Range = Stats.AttackRange + FMath::Max(0.f, AttackRangePadding);

	FVector Origin, Extent;
	GetTargetBoundsSafe(Target, Origin, Extent);

	const float CenterDist2D = FVector::Dist2D(From, Origin);
	const float Radius2D = FVector(Extent.X, Extent.Y, 0.f).Size();

	const float SurfaceDist2D = FMath::Max(0.f, CenterDist2D - Radius2D);

	return SurfaceDist2D <= Range;
}

// ------------------------------------------------------------
// 구조물 판정(원본 유지)
// ------------------------------------------------------------
bool UPotatoCombatComponent::IsStructureTarget(AActor* Target) const
{
	if (const APotatoPlaceableStructure* S = Cast<APotatoPlaceableStructure>(Target))
	{
		return (S->StructureData && S->StructureData->bIsDestructible);
	}
	return false;
}

void UPotatoCombatComponent::ApplyBasicDamage(AActor* Target) const
{
	if (!Target) return;

	AController* InstigatorController = nullptr;
	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		InstigatorController = PawnOwner->GetController();
	}

	float Damage = Stats.AttackDamage;

	if (IsStructureTarget(Target))
	{
		Damage *= Stats.StructureDamageMultiplier;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] ApplyDamage -> Target=%s Class=%s Damage=%.2f"),
		*GetNameSafe(Target),
		*GetNameSafe(Target->GetClass()),
		Damage);

	UGameplayStatics::ApplyDamage(
		Target,
		Damage,
		InstigatorController,
		GetOwner(),
		UDamageType::StaticClass()
	);
}

// ------------------------------------------------------------
// Muzzle 위치 계산 + Projectile 스폰
// ------------------------------------------------------------
bool UPotatoCombatComponent::GetMuzzleTransform(FVector& OutLoc, FRotator& OutRot) const
{
	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (!AnimSet) return false;

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh) return false;

	if (AnimSet->MuzzleSocketName.IsNone())
	{
		OutLoc = Owner->GetActorLocation();
		OutRot = Owner->GetActorRotation();
		return true;
	}

	OutLoc = Mesh->GetSocketLocation(AnimSet->MuzzleSocketName);
	OutRot = Mesh->GetSocketRotation(AnimSet->MuzzleSocketName);

	OutLoc += OutRot.RotateVector(AnimSet->MuzzleOffset);
	return true;
}

void UPotatoCombatComponent::SpawnProjectileToTarget(AActor* Target) const
{
	const UPotatoMonsterAnimSet* AnimSet = GetAnimSet();
	if (!AnimSet || !AnimSet->ProjectileClass || !Target || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnProjectile failed. AnimSet=%s ProjectileClass=%s Target=%s"),
			*GetNameSafe(AnimSet),
			AnimSet ? *GetNameSafe(AnimSet->ProjectileClass) : TEXT("None"),
			*GetNameSafe(Target));
		return;
	}

	FVector MuzzleLoc;
	FRotator MuzzleRot;
	if (!GetMuzzleTransform(MuzzleLoc, MuzzleRot))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] GetMuzzleTransform failed. Owner=%s"), *GetNameSafe(GetOwner()));
		return;
	}

	// AimPoint: Target pivot 대신 collision bounds Origin으로
	FVector AimPoint;
	{
		FVector Origin, Extent;
		GetTargetBoundsSafe(Target, Origin, Extent);
		AimPoint = Origin;
	}

	const FVector AimDir = (AimPoint - MuzzleLoc).GetSafeNormal();
	const FRotator SpawnRot = AimDir.Rotation();

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Cast<APawn>(GetOwner());

	AActor* Proj = GetWorld()->SpawnActor<AActor>(AnimSet->ProjectileClass, MuzzleLoc, SpawnRot, Params);
	if (!Proj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnActor failed. Class=%s"), *GetNameSafe(AnimSet->ProjectileClass.Get()));
		return;
	}

	// 데미지 주입(있으면)
	if (Proj->GetClass()->ImplementsInterface(UPotatoProjectileDamageable::StaticClass()))
	{
		IPotatoProjectileDamageable::Execute_SetProjectileDamage(Proj, Stats.AttackDamage);
	}

	// 방향/속도 강제 고정
	if (UProjectileMovementComponent* PM = Proj->FindComponentByClass<UProjectileMovementComponent>())
	{
		const float Speed = (PM->InitialSpeed > 0.f) ? PM->InitialSpeed : 1200.f;
		PM->Velocity = AimDir * Speed;
		PM->UpdateComponentVelocity();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnClass=%s"), *GetNameSafe(AnimSet->ProjectileClass.Get()));
	UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnProjectile -> Proj=%s Target=%s Muzzle=%s AimPoint=%s"),
		*GetNameSafe(Proj), *GetNameSafe(Target), *MuzzleLoc.ToString(), *AimPoint.ToString());
}