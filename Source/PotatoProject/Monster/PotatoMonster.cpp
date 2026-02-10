#include "PotatoMonster.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// SpecialSkill row struct 헤더가 cpp에서 필요함 (FindRow 템플릿)
#include "PotatoMonsterSpecialSkillPresetRow.h" // 너의 파일명에 맞춰 수정

FName APotatoMonster::GetRankRowName(EMonsterRank InRank)
{
	switch (InRank)
	{
	case EMonsterRank::Normal: return FName("Normal");
	case EMonsterRank::Elite:  return FName("Elite");
	case EMonsterRank::Boss:   return FName("Boss");
	default:                   return FName("Normal");
	}
}

FName APotatoMonster::GetTypeRowName(EMonsterType InType)
{
	const UEnum* Enum = StaticEnum<EMonsterType>();
	if (!Enum) return NAME_None;

	// DataTable RowName을 enum 항목 이름("Zombie", "Slime"...)로 맞춤
	const FString Name = Enum->GetNameStringByValue((int64)InType);
	return FName(*Name);
}

APotatoMonster::APotatoMonster()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void APotatoMonster::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 직후 Spawner에서 ApplyPresets 호출하는 흐름이면 여기서 굳이 안 불러도 됨.
	// 테스트 단계에서 자동 적용하고 싶으면 주석 해제:
	// ApplyPresets();
}

void APotatoMonster::ApplyPresetsFallback()
{
	AttackDamage = 10.0f;
	AttackRange = 150.0f;

	AppliedHpMultiplier = (Rank == EMonsterRank::Elite) ? 2.5f :
		(Rank == EMonsterRank::Boss) ? 10.0f : 1.0f;

	AppliedMoveSpeedRatio = (Rank == EMonsterRank::Elite) ? 0.8f : 0.6f;

	StructureDamageMultiplier = (Rank == EMonsterRank::Elite) ? 1.5f :
		(Rank == EMonsterRank::Boss) ? 3.0f : 1.0f;

	const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : 100.0f;
	MaxHealth = BaseHP * AppliedHpMultiplier;
	Health = MaxHealth;

	Speed = PlayerReferenceSpeed * AppliedMoveSpeedRatio;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = Speed;
	}

	ResolvedBehaviorTree = DefaultBehaviorTree;

	// --- Special (fallback) ---
	ResolvedSpecialSkillId = NAME_None;
	AppliedSpecialLogic = EMonsterSpecialLogic::None;
	ResolvedSpecialCooldown = 0.f;
	ResolvedSpecialDamageMultiplier = 1.f;
}

void APotatoMonster::ApplyPresets()
{
	// -------------------------
	// 기본값
	// -------------------------
	float TypeBaseHP = 100.0f;
	float TypeBaseAttackDamage = 10.0f;
	float TypeBaseAttackRange = 150.0f;
	float TypeMoveSpeedMul = 1.0f;

	FName TypeDefaultSkillId = NAME_None;

	ResolvedBehaviorTree = DefaultBehaviorTree;

	// -------------------------
	// 1) TypePreset
	// -------------------------
	const FPotatoMonsterTypePresetRow* TypeRow = nullptr;

	if (TypePresetTable)
	{
		const FName TypeRowName = GetTypeRowName(MonsterType);
		if (TypeRowName != NAME_None)
		{
			TypeRow = TypePresetTable->FindRow<FPotatoMonsterTypePresetRow>(
				TypeRowName, TEXT("APotatoMonster::ApplyPresets(TypePreset)")
			);
		}
	}

	if (TypeRow)
	{
		TypeBaseHP = TypeRow->BaseHP;
		TypeBaseAttackDamage = TypeRow->BaseAttackDamage;
		TypeBaseAttackRange = TypeRow->BaseAttackRange;
		TypeMoveSpeedMul = TypeRow->MoveSpeedMultiplier;

		TypeDefaultSkillId = TypeRow->DefaultSpecialSkillId;

		// SoftObject BT 로드
		if (TypeRow->OverrideBehaviorTree.IsValid())
		{
			ResolvedBehaviorTree = TypeRow->OverrideBehaviorTree.Get();
		}
		else if (!TypeRow->OverrideBehaviorTree.IsNull())
		{
			ResolvedBehaviorTree = TypeRow->OverrideBehaviorTree.LoadSynchronous();
		}
		else
		{
			ResolvedBehaviorTree = DefaultBehaviorTree;
		}
	}
	else
	{
		ResolvedBehaviorTree = DefaultBehaviorTree;
	}

	AttackDamage = TypeBaseAttackDamage;
	AttackRange = TypeBaseAttackRange;

	const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : TypeBaseHP;

	// -------------------------
	// 2) RankPreset
	// -------------------------
	const FPotatoMonsterRankPresetRow* RankRow = nullptr;

	float RankCooldownMul = 1.0f;
	float RankSpecialDmgMul = 1.0f;
	FName RankDefaultSkillId = NAME_None;

	if (RankPresetTable)
	{
		const FName RankRowName = GetRankRowName(Rank);
		RankRow = RankPresetTable->FindRow<FPotatoMonsterRankPresetRow>(
			RankRowName, TEXT("APotatoMonster::ApplyPresets(RankPreset)")
		);
	}

	if (!RankRow)
	{
		ApplyPresetsFallback();
		return;
	}

	// HP multiplier
	float HpMul = RankRow->HpMultiplierMin;
	if (RankRow->HpMultiplierMax > RankRow->HpMultiplierMin + KINDA_SMALL_NUMBER)
	{
		HpMul = FMath::FRandRange(RankRow->HpMultiplierMin, RankRow->HpMultiplierMax);
	}

	AppliedHpMultiplier = HpMul;
	MaxHealth = BaseHP * AppliedHpMultiplier;
	Health = MaxHealth;

	AppliedMoveSpeedRatio = RankRow->MoveSpeedRatioToPlayer;
	Speed = PlayerReferenceSpeed * AppliedMoveSpeedRatio * TypeMoveSpeedMul;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = Speed;
	}

	StructureDamageMultiplier = RankRow->StructureDamageMultiplier;

	// Rank special knobs
	RankDefaultSkillId = RankRow->DefaultSpecialSkillId;
	RankCooldownMul = RankRow->SpecialCooldownMultiplier;
	RankSpecialDmgMul = RankRow->SpecialDamageMultiplier;

	// -------------------------
	// 3) 최종 SpecialSkillId 선택 (Type > Rank)  // Theme 제거됨
	// -------------------------
	ResolvedSpecialSkillId =
		(TypeDefaultSkillId != NAME_None) ? TypeDefaultSkillId :
		(RankDefaultSkillId != NAME_None) ? RankDefaultSkillId :
		NAME_None;

	// 기본값
	AppliedSpecialLogic = EMonsterSpecialLogic::None;
	ResolvedSpecialCooldown = 0.f;

	// 스킬이 없더라도 Rank 보정은 의미 없으니 기본 1로 두는 편이 안전
	ResolvedSpecialDamageMultiplier = 1.f;

	// -------------------------
	// 4) SpecialSkillPreset 적용 (있으면)
	// -------------------------
	if (ResolvedSpecialSkillId != NAME_None && SpecialSkillPresetTable)
	{
		const FPotatoMonsterSpecialSkillPresetRow* SkillRow =
			SpecialSkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
				ResolvedSpecialSkillId, TEXT("APotatoMonster::ApplyPresets(SkillPreset)")
			);

		if (SkillRow)
		{
			AppliedSpecialLogic = SkillRow->Logic;

			// 쿨다운: Skill 기본 * Rank 보정
			ResolvedSpecialCooldown = SkillRow->Cooldown * RankCooldownMul;

			// 대미지 배수: Skill 기본 * Rank 보정
			ResolvedSpecialDamageMultiplier = SkillRow->DamageMultiplier * RankSpecialDmgMul;

			// NOTE: 범위/각도/도트/투사체 등은
			// 여기서 멤버로 캐싱하거나, SpecialComponent로 넘기는 구조 추천.
			// 예) ResolvedSkillRadius = SkillRow->Radius;
		}
		else
		{
			// 스킬ID는 있는데 행이 없으면 비활성화
			ResolvedSpecialSkillId = NAME_None;
			AppliedSpecialLogic = EMonsterSpecialLogic::None;
			ResolvedSpecialCooldown = 0.f;
			ResolvedSpecialDamageMultiplier = 1.f;
		}
	}
}

void APotatoMonster::Attack(AActor* Target)
{
	if (bIsDead || !Target) return;

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Dist > AttackRange) return;

	float FinalDamage = AttackDamage;

	// TODO: 구조물 배율 적용은 대상 태그/클래스 확정 시 적용
	UGameplayStatics::ApplyDamage(Target, FinalDamage, GetController(), this, nullptr);
}

void APotatoMonster::ApplyDamage(float Damage)
{
	if (bIsDead || Damage <= 0.f) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	if (Health <= 0.f) Die();
}

void APotatoMonster::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// TODO: OnDeath 트리거 스킬 처리 (Split 등)
	// - SkillRow.Trigger == OnDeath일 때 실행하도록 SpecialComponent로 빼는 걸 추천

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
}

AActor* APotatoMonster::FindTarget()
{
	if (CurrentTarget) return CurrentTarget;
	return WarehouseActor;
}
