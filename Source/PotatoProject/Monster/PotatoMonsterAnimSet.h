#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PotatoMonsterAnimSet.generated.h"

class UBlendSpace;
class UAnimMontage;

UCLASS(BlueprintType)
class POTATOPROJECT_API UPotatoMonsterAnimSet : public UDataAsset
{
	GENERATED_BODY()
public:
	// ---- Locomotion ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Locomotion")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace = nullptr;

	// ---- Attack ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Attack")
	TObjectPtr<UAnimMontage> BasicAttackMontage = nullptr;

	// "공속" (초 단위 공격 간격)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Attack", meta = (ClampMin = "0.01"))
	float BasicAttackInterval = 1.0f;

	// ---- Ranged ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged")
	bool bIsRanged = false;

	// 투사체 액터(또는 APotatoProjectile)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	FName MuzzleSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged", meta = (EditCondition = "bIsRanged"))
	FVector MuzzleOffset = FVector::ZeroVector;

	// (선택)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Hit")
	TObjectPtr<UAnimMontage> HitReactMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Death")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;
};
