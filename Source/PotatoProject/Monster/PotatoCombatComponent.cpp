#include "PotatoCombatComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "PotatoMonster.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/TargetPoint.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "GameFramework/DamageType.h"

#include "PotatoProjectileDamageable.h"
#include "PotatoMonsterProjectile.h"

// AnimSet
#include "PotatoMonsterAnimSet.h"

#include "FXUtils/PotatoFXUtils.h"

// SpecialSkill
#include "Monster/SpecialSkillComponent.h"

static const FName TAG_LanePoint(TEXT("LanePoint"));

// ------------------------------------------------------------
// Helper: Target의 충돌 Bounds (Origin/Extent) 가져오기
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
	NextOnAttackSpecialProcTime = 0.0;
}

const UPotatoMonsterAnimSet* UPotatoCombatComponent::GetAnimSet() const
{
	return Stats.AnimSet;
}

// ------------------------------------------------------------
// 최종 허용 타겟 판정
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
// Proc: OnAttack Special
// ------------------------------------------------------------
void UPotatoCombatComponent::TryProcOnAttackSpecial(APotatoMonster* Monster, AActor* Target, double Now)
{
	if (!Monster || !Target) return;

	if (!Stats.bEnableOnAttackSpecialProc) return;

	const float Chance = FMath::Clamp(Stats.OnAttackSpecialChance, 0.f, 1.f);
	if (Chance <= 0.f) return;

	if (Now < NextOnAttackSpecialProcTime) return;

	// 확률 판정
	if (FMath::FRand() > Chance) return;

	if (!Monster->SpecialSkillComp) return;

	const FName SkillId = Monster->SpecialSkill_OnAttack;
	if (SkillId.IsNone()) return;

	// Ready 프리체크(불필요 호출 억제)
	if (!Monster->SpecialSkillComp->CanTryStartSkill(SkillId))
	{
		return;
	}

	const bool bStarted = Monster->SpecialSkillComp->TryStartSkill(SkillId, Target);
	if (bStarted)
	{
		NextOnAttackSpecialProcTime = Now + (double)FMath::Max(0.f, Stats.OnAttackSpecialProcCooldown);
	}
}

// ------------------------------------------------------------
// 몽타주 종료 시 공격상태 해제 보장
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

	// 공격 시작 직전 최종 방어선
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

	// 공격 상태 ON
	PendingBasicTarget = Target;
	bIsAttacking = true;

	// ✅ OnAttack 스페셜: Proc 성공 시에만 TryStart
	TryProcOnAttackSpecial(Monster, Target, Now);

	// AttackStart SFX
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

	// 공격 간격
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

	// Notify 시점 재검증
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

	// AttackHit SFX (근접 타격)
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

	if (!IsTargetInRange(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Out of range at FireProjectile -> EndBasicAttack. Target=%s"), *GetNameSafe(Target));
		EndBasicAttack();
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	// ProjectileFire SFX (머즐 위치 우선)
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

	EndBasicAttack();
}

void UPotatoCombatComponent::EndBasicAttack()
{
	bIsAttacking = false;
	PendingBasicTarget.Reset();
}

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

	FVector AimPoint;
	{
		FVector Origin, Extent;
		GetTargetBoundsSafe(Target, Origin, Extent);
		AimPoint = Origin;
	}

	FVector AimDir = (AimPoint - MuzzleLoc).GetSafeNormal();
	if (AimDir.IsNearlyZero())
	{
		AimDir = GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector;
	}

	const FRotator SpawnRot = AimDir.Rotation();

	const float SpawnForwardOffset = 40.f;
	const FVector SpawnLoc = MuzzleLoc + AimDir * SpawnForwardOffset;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Cast<APawn>(GetOwner());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Proj = GetWorld()->SpawnActor<AActor>(AnimSet->ProjectileClass, SpawnLoc, SpawnRot, Params);
	if (!Proj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnActor failed. Class=%s"), *GetNameSafe(AnimSet->ProjectileClass.Get()));
		return;
	}

	if (Proj->GetClass()->ImplementsInterface(UPotatoProjectileDamageable::StaticClass()))
	{
		IPotatoProjectileDamageable::Execute_SetProjectileDamage(Proj, Stats.AttackDamage);
	}

	if (UProjectileMovementComponent* PM = Proj->FindComponentByClass<UProjectileMovementComponent>())
	{
		const float Speed = (PM->InitialSpeed > 0.f) ? PM->InitialSpeed : 1200.f;
		PM->Velocity = AimDir * Speed;
		PM->UpdateComponentVelocity();

		UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnProjectile(PM) -> Proj=%s Speed=%.1f Target=%s Muzzle=%s AimPoint=%s"),
			*GetNameSafe(Proj), Speed, *GetNameSafe(Target), *SpawnLoc.ToString(), *AimPoint.ToString());
		return;
	}

	if (APotatoMonsterProjectile* TraceProj = Cast<APotatoMonsterProjectile>(Proj))
	{
		const float TraceSpeed = 1800.f;
		TraceProj->InitVelocity(AimDir, TraceSpeed);

		UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnProjectile(Trace) -> Proj=%s Speed=%.1f Target=%s Muzzle=%s AimPoint=%s"),
			*GetNameSafe(Proj), TraceSpeed, *GetNameSafe(Target), *SpawnLoc.ToString(), *AimPoint.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat] SpawnProjectile UNKNOWN TYPE -> Proj=%s Class=%s Target=%s"),
		*GetNameSafe(Proj), *GetNameSafe(Proj->GetClass()), *GetNameSafe(Target));
}