#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoAuraDamageComponent.generated.h"

class USphereComponent;

/**
 * 오버랩(예: Player 태그) 동안 지속 데미지 주는 오라 컴포넌트
 * - 선인장에 붙이면 “가시 데미지” 구현 끝
 */
UCLASS(ClassGroup=(Potato), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoAuraDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoAuraDamageComponent();

	/** 반경/데미지 세팅(원하면 BP에서 직접 세팅해도 됨) */
	UFUNCTION(BlueprintCallable, Category="Potato|Aura")
	void Configure(float InRadius, float InDps, float InTickInterval);

	/** 태그 필터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Aura")
	FName RequiredTargetTag = TEXT("Player");

	/** 초당 데미지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Aura")
	float Dps = 5.f;

	/** 틱 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Aura")
	float TickInterval = 0.2f;

	/** 오라 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Aura")
	float Radius = 120.f;

	/** DamageType */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Potato|Aura")
	TSubclassOf<UDamageType> DamageTypeClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void TickAura();

	bool IsValidTarget(AActor* A) const;

private:
	UPROPERTY() AActor* CachedOwner = nullptr;

	UPROPERTY() USphereComponent* Sphere = nullptr;

	UPROPERTY() TSet<TWeakObjectPtr<AActor>> OverlappingTargets;

	FTimerHandle AuraTickTH;
};