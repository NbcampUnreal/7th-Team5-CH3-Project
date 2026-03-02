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

	// 런타임 설정
	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void Configure(float InRadius, float InDps, float InTickInterval);

	// 몬스터에서 명시적으로 제어하는 API (Activate/Deactivate 직접 호출 대신 이걸 쓰는 걸 추천)
	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void StartAura();

	UFUNCTION(BlueprintCallable, Category="Potato|AuraDamage")
	void StopAura();

public:
	// None이면 "아무도 안 때림"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	TArray<FName> RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	bool bAllowDamageToMonsters = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|AuraDamage")
	TSubclassOf<UDamageType> DamageTypeClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void EnsureSphereCreated();
	void ApplySphereOffState();
	void ApplySphereOnState();

	void StartAuraInternal(); // BeginPlay 이후에만 실제 ON 처리

	bool IsValidTarget(AActor* A) const;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

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

	// ✅ 핵심: BeginPlay 전에 StartAura 호출되면 여기로 예약
	bool bPendingStart = false;
	bool bBegunPlay = false;

	TSet<TWeakObjectPtr<AActor>> OverlappingTargets;
	TMap<TWeakObjectPtr<AActor>, FHitResult> LastSweepHitByTarget;
};