#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class AActor;
class UAnimMontage;
class APotatoMonster;

struct FPotatoMonsterSpecialSkillPresetRow;

// 짧은 애니를 "최소 가시 시간" 이상 보이게 하기 위해 재생 속도를 낮춰 최종 길이를 MinVisible로 맞춤
float ComputeMinVisiblePlayRate(float BaseLen, float MinVisible, float& OutFinalLen);

// 몽타주 재생 (디버그 로그 포함)
bool PlayMontage(AActor* Owner, UAnimMontage* Montage, float Rate);

// 기본 공격 몽타주 fallback
UAnimMontage* GetFallbackBasicAttackMontage(APotatoMonster* Monster);

// 스킬 Row가 "공격성(데미지/투사체/AoE)"인지 판별
bool IsAttackLikeSkill(const FPotatoMonsterSpecialSkillPresetRow& Row);

// Soft montage 로드 (동기)
UAnimMontage* LoadMontageSafe(const TSoftObjectPtr<UAnimMontage>& Soft);

void EnableRagdollFallback(APotatoMonster* M);