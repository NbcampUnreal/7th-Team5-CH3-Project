#pragma once

#include "CoreMinimal.h"

class AActor;
class UAnimInstance;
class UAnimMontage;
class APotatoMonster;

struct FPotatoMonsterSpecialSkillPresetRow;

// ------------------------------------------------------------
// Anim helpers
// ------------------------------------------------------------

float ComputeMinVisiblePlayRate(float BaseLen, float MinVisible, float& OutFinalLen);

bool PlayMontage(AActor* Owner, UAnimMontage* Montage, float Rate = 1.f);

UAnimMontage* GetFallbackBasicAttackMontage(APotatoMonster* Monster);

bool IsAttackLikeSkill(const FPotatoMonsterSpecialSkillPresetRow& Row);

UAnimMontage* LoadMontageSafe(const TSoftObjectPtr<UAnimMontage>& Soft);