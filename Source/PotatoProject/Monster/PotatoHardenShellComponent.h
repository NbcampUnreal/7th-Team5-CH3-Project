#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoHardenShellComponent.generated.h"

class USkeletalMeshComponent;

/**
 * HP% 이하로 떨어지면 “하드닝” 상태 진입:
 * - 데미지 배율(예: 0.1)
 * - 머티리얼 색(검보라) 파라미터 변경
 *
 * 사용 방식:
 * - APotatoMonster::TakeDamage()에서
 *   1) HardenComp 있으면 Damage = HardenComp->ModifyIncomingDamage(Damage)
 *   2) HP 감소 반영 후 HardenComp->PostDamageCheck()
 */
UCLASS(ClassGroup=(Potato), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoHardenShellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoHardenShellComponent();

	/** 하드닝 진입 HP% (예: 0.3 = 30% 이하) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	float TriggerHpPercent = 0.3f;

	/** 하드닝 중 들어오는 데미지 배율 (0.1 = 1/10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	float DamageMultiplierWhenHardened = 0.1f;

	/** HP 프로퍼티 이름(리플렉션) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	FName HpPropertyName = TEXT("HP");

	/** MaxHP 프로퍼티 이름(리플렉션) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	FName MaxHpPropertyName = TEXT("MaxHP");

	/** 머티리얼 파라미터 이름(벡터) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	FName TintParamName = TEXT("ShellTint");

	/** 하드닝 색(검보라 계열) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Harden")
	FLinearColor HardenTint = FLinearColor(0.05f, 0.0f, 0.12f, 1.0f);

	/** 하드닝 상태 */
	UFUNCTION(BlueprintCallable, Category="Potato|Harden")
	bool IsHardened() const { return bHardened; }

	/** TakeDamage 들어오기 전: 데미지 수정 */
	UFUNCTION(BlueprintCallable, Category="Potato|Harden")
	float ModifyIncomingDamage(float InDamage) const;

	/** TakeDamage 처리 후: HP 기준 체크해서 하드닝 진입 */
	UFUNCTION(BlueprintCallable, Category="Potato|Harden")
	void PostDamageCheck();

protected:
	virtual void BeginPlay() override;

private:
	bool ReadHp(float& OutHP, float& OutMaxHP) const;
	void ApplyHardenMaterial();

private:
	UPROPERTY() AActor* CachedOwner = nullptr;
	UPROPERTY() USkeletalMeshComponent* CachedMesh = nullptr;

	UPROPERTY() bool bHardened = false;
};