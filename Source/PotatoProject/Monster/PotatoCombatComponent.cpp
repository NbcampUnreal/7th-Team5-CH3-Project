// PotatoCombatComponent.cpp

#include "PotatoCombatComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "PotatoMonster.h"
#include "Animation/AnimInstance.h"

UPotatoCombatComponent::UPotatoCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoCombatComponent::InitFromStats(const FPotatoMonsterFinalStats& InStats)
{
	Stats = InStats;
}

bool UPotatoCombatComponent::RequestBasicAttack(AActor* Target)
{

    if (!GetWorld() || !Target || !GetOwner())
    {
        return false;
    }

    const double Now = GetWorld()->GetTimeSeconds();

    if (Now < NextAttackTime)
    {
        return false;
    }

    if (bIsAttacking)
    {
        return false;
    }


    PendingBasicTarget = Target;
    bIsAttacking = true;

    APotatoMonster* Monster = Cast<APotatoMonster>(GetOwner());
    if (!Monster)
    {
        bIsAttacking = false;
        return false;
    }

    USkeletalMeshComponent* Mesh = Monster->GetMesh();
    UAnimInstance* AnimInst = Mesh ? Mesh->GetAnimInstance() : nullptr;

    if (!AnimInst)
    {
        bIsAttacking = false;
        return false;
    }

    if (!Monster->BasicAttackMontage)
    {
        bIsAttacking = false;
        return false;
    }

    NextAttackTime = Now + 1.0;
    return true;
}

void UPotatoCombatComponent::ApplyPendingBasicDamage()
{
	AActor* Target = PendingBasicTarget.Get();
	if (!Target || !GetOwner()) return;

	// 사거리 다시 확인
	 if (!IsTargetInRange(Target)) return;

	ApplyBasicDamage(Target);
}

void UPotatoCombatComponent::EndBasicAttack()
{
	bIsAttacking = false;
	PendingBasicTarget.Reset();
}

bool UPotatoCombatComponent::IsTargetInRange(AActor* Target) const
{
	if (!Target || !GetOwner()) return false;
	const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	return Dist <= Stats.AttackRange;
}

bool UPotatoCombatComponent::IsStructureTarget(AActor* Target) const
{
	static const FName TAG_Structure(TEXT("Structure"));
	return Target && Target->ActorHasTag(TAG_Structure);
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

	UGameplayStatics::ApplyDamage(Target, Damage, InstigatorController, GetOwner(), nullptr);
}
