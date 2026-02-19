#include "PotatoMonster.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "PotatoPresetApplier.h"
#include "PotatoCombatComponent.h"

APotatoMonster::APotatoMonster()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CombatComp = CreateDefaultSubobject<UPotatoCombatComponent>(TEXT("CombatComp"));
}

void APotatoMonster::BeginPlay()
{
	Super::BeginPlay();
	// ApplyPresetsOnce는 AIController::OnPossess에서 호출
}

void APotatoMonster::ApplyPresetsOnce()
{
	if (bPresetsApplied) return;
	bPresetsApplied = true;

	FinalStats = UPotatoPresetApplier::BuildFinalStats(
		this,
		MonsterType,
		Rank,
		WaveBaseHP,
		PlayerReferenceSpeed,
		TypePresetTable,
		RankPresetTable,
		SpecialSkillPresetTable,
		DefaultBehaviorTree
	);

	MaxHealth = FinalStats.MaxHP;
	Health = MaxHealth;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FinalStats.MoveSpeed;
	}

	ResolvedBehaviorTree = FinalStats.BehaviorTree;

	if (CombatComp)
	{
		CombatComp->InitFromStats(FinalStats);
	}
}

float APotatoMonster::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (bIsDead)
	{
		return 0.f;
	}
	const float Applied = FMath::Max(0.f, DamageAmount);
	if (Applied <= 0.f)
	{
		return 0.f;
	}
	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);

	if (Health > 0.f)
	{
		const UPotatoMonsterAnimSet* AS = GetAnimSet();
		if (AS && AS->HitReactMontage)
		{
			if (USkeletalMeshComponent* Skel = GetMesh())
			{
				if (UAnimInstance* AnimInst = Skel->GetAnimInstance())
				{
					AnimInst->Montage_Play(AS->HitReactMontage, 1.0f);
				}
			}
		}
	}
	else
	{
		Die();
	}

	return Applied;
}

void APotatoMonster::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	SetActorEnableCollision(false);

	const UPotatoMonsterAnimSet* AS = GetAnimSet();
	if (AS && AS->DeathMontage)
	{
		if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Play(AS->DeathMontage, 1.0f);
		}
	}

	SetLifeSpan(2.0f);
}
void APotatoMonster::AdvanceLaneIndex()
{
	// LanePoints의 마지막을 넘기면 GetCurrentLaneTarget이 Warehouse로 fallback
	LaneIndex = FMath::Clamp(LaneIndex + 1, 0, LanePoints.Num());
}

AActor* APotatoMonster::GetCurrentLaneTarget() const
{
	if (LanePoints.IsValidIndex(LaneIndex))
	{
		return LanePoints[LaneIndex];
	}
	return WarehouseActor;
}

void APotatoMonster::SetAnimSet(UPotatoMonsterAnimSet* InSet)
{
	AnimSet = InSet;

	// AnimInstance에 주입(로코모션/공격에서 참고 가능)
	//if (USkeletalMeshComponent* Skel = GetMesh())
	//{
	//	if (UAnimInstance* AI = Skel->GetAnimInstance())
	//	{
	//		if (UPotatoMonsterAnimInstance* PMI = Cast<UPotatoMonsterAnimInstance>(AI))
	//		{
	//			PMI->SetAnimSet(AnimSet);
	//		}
	//	}
	//}
}