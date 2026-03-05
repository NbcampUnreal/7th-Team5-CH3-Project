#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "PotatoAuraDamageComponent.generated.h"

class USphereComponent;
class UDamageType;

UCLASS(ClassGroup=(Potato), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoAuraDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static const FName AuraSphereName;

	UPotatoAuraDamageComponent();

	// ✅ Shipping/패키징 안정성: Sphere가 teardown/파괴/Unregister 상태인지 체크
	bool IsSphereSafeToTouch() const;

	// 런타임 설정
	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void Configure(float InRadius, float InDps, float InTickInterval);

	// 몬스터에서 명시적으로 제어하는 API
	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void StartAura();

	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void StopAura();

public:
	// None/Empty면 "아무도 안 때림"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	TArray<FName> RequiredTargetTags;

	// 기본: 몬스터끼리 데미지 금지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	bool bAllowDamageToMonsters = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	TSubclassOf<UDamageType> DamageTypeClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Sphere 생성/재사용
	void EnsureSphereCreated();

	// Collision/Overlap 토글
	void ApplySphereOffState();
	void ApplySphereOnState();

	// BeginPlay 이후에만 실제 ON 처리
	void StartAuraInternal();

	// 타겟 필터
	bool IsValidTarget(AActor* A) const;

	// ✅ AddDynamic 바인딩 대상은 반드시 UFUNCTION
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	// 데미지 틱
	void TickAura();

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CachedOwner = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> Sphere = nullptr;

	FTimerHandle AuraTickTH;

	float Radius = 0.f;
	float Dps = 0.f;
	float TickInterval = 0.25f;

	// BeginPlay 전에 StartAura 호출되면 예약
	bool bPendingStart = false;
	bool bBegunPlay = false;

	TSet<TWeakObjectPtr<AActor>> OverlappingTargets;
	TMap<TWeakObjectPtr<AActor>, FHitResult> LastSweepHitByTarget;
};