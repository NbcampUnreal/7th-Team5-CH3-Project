// Source/PotatoProject/Monster/Utils/PotatoAnimUtils.cpp
#include "Monster/Utils/PotatoAnimUtils.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Monster/PotatoMonster.h"
#include "Monster/PotatoCombatComponent.h"
#include "Monster/PotatoMonsterAnimSet.h"
#include "Monster/PotatoMonsterSpecialSkillPresetRow.h"
#include "Core/PotatoEnums.h"

float ComputeMinVisiblePlayRate(float BaseLen, float MinVisible, float& OutFinalLen)
{
	OutFinalLen = BaseLen;

	if (BaseLen <= KINDA_SMALL_NUMBER) return 1.f;
	if (MinVisible <= 0.f) return 1.f;

	if (BaseLen < MinVisible)
	{
		OutFinalLen = MinVisible;
		return BaseLen / MinVisible; // 0~1 배속
	}

	return 1.f;
}

bool PlayMontage(AActor* Owner, UAnimMontage* Montage, float Rate)
{
	ACharacter* C = Cast<ACharacter>(Owner);
	if (!IsValid(C) || C->IsActorBeingDestroyed()) return false;

	USkeletalMeshComponent* Mesh = C->GetMesh();
	UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!Anim || !Montage) return false;

	return Anim->Montage_Play(Montage, Rate) > 0.f;
}

UAnimMontage* GetFallbackBasicAttackMontage(APotatoMonster* Monster)
{
	if (!Monster) return nullptr;
	if (!IsValid(Monster->CombatComp)) return nullptr;

	const UPotatoMonsterAnimSet* AnimSet = Monster->CombatComp->GetAnimSet();
	return AnimSet ? AnimSet->BasicAttackMontage : nullptr;
}

bool IsAttackLikeSkill(const FPotatoMonsterSpecialSkillPresetRow& Row)
{
	// Execution이 명시되어 있으면 가장 신뢰
	if (Row.Execution != EMonsterSpecialExecution::None)
	{
		switch (Row.Execution)
		{
		case EMonsterSpecialExecution::InstantAoE:
		case EMonsterSpecialExecution::ContactDOT:
		case EMonsterSpecialExecution::Projectile:
			return true;
		default:
			return false;
		}
	}

	// Execution이 None이면 Shape로 추정
	switch (Row.Shape)
	{
	case EMonsterSpecialShape::Projectile:
	case EMonsterSpecialShape::Aura:
	case EMonsterSpecialShape::Circle:
	case EMonsterSpecialShape::Cone:
	case EMonsterSpecialShape::Line:
		return true;
	default:
		return false;
	}
}

UAnimMontage* LoadMontageSafe(const TSoftObjectPtr<UAnimMontage>& Soft)
{
	if (Soft.IsNull()) return nullptr;
	return Soft.LoadSynchronous();
}