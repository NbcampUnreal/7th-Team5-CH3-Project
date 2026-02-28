// Source/PotatoProject/Monster/Utils/PotatoAnimUtils.cpp
#include "Monster/Utils/PotatoAnimUtils.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Monster/PotatoMonster.h"
#include "Monster/PotatoCombatComponent.h"
#include "Monster/PotatoMonsterAnimSet.h"
#include "Monster/PotatoMonsterSpecialSkillPresetRow.h"
#include "Core/PotatoEnums.h"

DEFINE_LOG_CATEGORY_STATIC(LogPotatoAnimUtils, Log, All);

float ComputeMinVisiblePlayRate(float BaseLen, float MinVisible, float& OutFinalLen)
{
	OutFinalLen = BaseLen;

	if (BaseLen <= KINDA_SMALL_NUMBER) return 1.f;
	if (MinVisible <= 0.f) return 1.f;

	// BaseLen이 더 짧으면 느리게 해서 최종 길이를 MinVisible로 늘림
	if (BaseLen < MinVisible)
	{
		OutFinalLen = MinVisible;

		const float Rate = BaseLen / MinVisible; // 0~1
		// 0에 너무 가까우면 멈춘 듯 보이므로 하한 가드(원하면 조정)
		return FMath::Clamp(Rate, 0.05f, 1.0f);
	}

	return 1.f;
}

static void DumpMontageState(UAnimInstance* Anim, const TCHAR* Prefix)
{
	if (!Anim) return;

	// 현재 재생 중인 몽타주가 있는지 확인
	UAnimMontage* Cur = Anim->GetCurrentActiveMontage();
	if (!Cur)
	{
		UE_LOG(LogPotatoAnimUtils, Verbose, TEXT("%s No active montage."), Prefix);
		return;
	}

	const float Pos = Anim->Montage_GetPosition(Cur);
	const float Len = Cur->GetPlayLength();
	const bool bPlaying = Anim->Montage_IsPlaying(Cur);

	UE_LOG(LogPotatoAnimUtils, Verbose, TEXT("%s ActiveMontage=%s Playing=%d Pos=%.3f/%.3f"),
		Prefix, *GetNameSafe(Cur), bPlaying ? 1 : 0, Pos, Len);
}

bool PlayMontage(AActor* Owner, UAnimMontage* Montage, float Rate)
{
	ACharacter* C = Cast<ACharacter>(Owner);
	if (!IsValid(C) || C->IsActorBeingDestroyed())
	{
		UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[PlayMontage] Invalid Owner=%s"), *GetNameSafe(Owner));
		return false;
	}

	USkeletalMeshComponent* Mesh = C->GetMesh();
	if (!IsValid(Mesh) || !Mesh->IsRegistered())
	{
		UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[PlayMontage] Mesh invalid/unregistered Owner=%s Mesh=%s Reg=%d"),
			*GetNameSafe(C), *GetNameSafe(Mesh), Mesh ? (Mesh->IsRegistered() ? 1 : 0) : 0);
		return false;
	}

	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (!IsValid(Anim) || !IsValid(Montage))
	{
		UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[PlayMontage] Anim/Montage invalid Owner=%s Anim=%s Montage=%s"),
			*GetNameSafe(C), *GetNameSafe(Anim), *GetNameSafe(Montage));
		return false;
	}

	Rate = FMath::Max(0.01f, Rate);

	// 디버깅: 재생 전 상태 덤프
	DumpMontageState(Anim, TEXT("[PlayMontage][Before]"));

	// 이미 같은 몽타주가 돌고 있으면 굳이 다시 재생하지 않거나(정책),
	// 재시작할지 결정할 수 있음. (일단은 로그만)
	if (Anim->Montage_IsPlaying(Montage))
	{
		UE_LOG(LogPotatoAnimUtils, Verbose, TEXT("[PlayMontage] Montage already playing=%s Owner=%s"),
			*GetNameSafe(Montage), *GetNameSafe(C));
	}

	const float Played = Anim->Montage_Play(Montage, Rate);

	if (Played <= 0.f)
	{
		UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[PlayMontage] Montage_Play FAILED Owner=%s Montage=%s Rate=%.3f"),
			*GetNameSafe(C), *GetNameSafe(Montage), Rate);
		DumpMontageState(Anim, TEXT("[PlayMontage][AfterFail]"));
		return false;
	}

	UE_LOG(LogPotatoAnimUtils, Verbose, TEXT("[PlayMontage] Montage_Play OK Owner=%s Montage=%s Rate=%.3f Returned=%.3f"),
		*GetNameSafe(C), *GetNameSafe(Montage), Rate, Played);

	// 재생 직후 상태
	DumpMontageState(Anim, TEXT("[PlayMontage][AfterOK]"));
	return true;
}

UAnimMontage* GetFallbackBasicAttackMontage(APotatoMonster* Monster)
{
	if (!IsValid(Monster)) return nullptr;
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

// ------------------------------------------------------------
// Ragdoll fallback (디버깅/안전 가드 강화)
// ------------------------------------------------------------
void EnableRagdollFallback(APotatoMonster* M)
{
	if (!IsValid(M)) return;

	UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[RagdollFallback] Enable for %s"), *GetNameSafe(M));

	if (UCapsuleComponent* Cap = M->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Cap->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	USkeletalMeshComponent* Mesh = M->GetMesh();
	if (!IsValid(Mesh))
	{
		UE_LOG(LogPotatoAnimUtils, Warning, TEXT("[RagdollFallback] Mesh invalid %s"), *GetNameSafe(M));
		return;
	}

	// 소켓/루트 분리 시 위치 튀는 경우가 많아서, 필요할 때만 Detach 권장
	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeAllRigidBodies();
	Mesh->bBlendPhysics = true;

	// SingleNode로 바꾸면 이후 애니 재생이 꼬일 수 있음.
	// 완전 ragdoll로 죽는 연출이면 괜찮지만, "fallback" 용이면 주석 권장.
	// Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}