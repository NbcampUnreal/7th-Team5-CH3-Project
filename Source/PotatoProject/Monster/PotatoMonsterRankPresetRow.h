#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterRankPresetRow.generated.h"

USTRUCT(BlueprintType)
struct POTATOPROJECT_API FPotatoMonsterRankPresetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float HpMultiplierMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float HpMultiplierMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	float MoveSpeedRatioToPlayer = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	float StructureDamageMultiplier = 1.0f;

	//  단일 스킬 구조: 스킬 로우에 곱해지는 랭크 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special|Tuning", meta=(ClampMin="0.01"))
	float SpecialCooldownScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special|Tuning", meta=(ClampMin="0.0"))
	float SpecialDamageScale = 1.0f;

	// Proc
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc")
	bool bEnableOnAttackSpecialProc = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OnAttackSpecialChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Special Proc", meta=(ClampMin="0.0"))
	float OnAttackSpecialProcCooldown = 1.50f;
};