// Combat/PotatoBuffComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoBuffComponent.generated.h"

// 버프 타입(필요한 것만 최소)
UENUM(BlueprintType)
enum class EPotatoBuffType : uint8
{
	None UMETA(DisplayName="None"),

	// 가시거북: 피해감소(단단한 껍질)
	DamageReduction UMETA(DisplayName="DamageReduction"),

	// 확장용
	MoveSpeedMultiplier UMETA(DisplayName="MoveSpeedMultiplier"),
	Shield UMETA(DisplayName="Shield"),
};

// ✅ 이게 없어서 UHT가 터지는 케이스가 많음
USTRUCT(BlueprintType)
struct POTATOPROJECT_API FActiveBuff
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff")
	EPotatoBuffType Type = EPotatoBuffType::None;

	// DamageReduction: 0.9면 90% 감소(= 10%만 받음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff")
	float Magnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff")
	float Duration = 0.f;

	// 내부 런타임
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Buff")
	double EndTime = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Buff")
	TWeakObjectPtr<AActor> SourceActor;

	// 식별용(스킬/프리셋 id 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff")
	FName BuffId = NAME_None;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoBuffComponent();

	// =========================
	// Apply / Query
	// =========================

	// DamageReduction: Magnitude = 0.9 (90% 감소)
	UFUNCTION(BlueprintCallable, Category="Potato|Buff")
	void ApplyBuff(FName BuffId, EPotatoBuffType Type, float Magnitude, float Duration, AActor* Source);

	UFUNCTION(BlueprintCallable, Category="Potato|Buff")
	bool HasBuffType(EPotatoBuffType Type) const;

	// DamageReduction 합성(여러 버프가 있을 수 있으니 최대값 사용)
	UFUNCTION(BlueprintCallable, Category="Potato|Buff")
	float GetBestDamageReduction() const;

	UFUNCTION(BlueprintCallable, Category="Potato|Buff")
	void ClearAllBuffs();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Potato|Buff")
	TArray<FActiveBuff> ActiveBuffs;

	FTimerHandle TickTH;

	double Now() const;
	void TickBuffs();

	void RemoveExpired(double NowTime);
};