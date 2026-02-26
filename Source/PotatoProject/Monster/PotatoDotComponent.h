// Combat/PotatoDotComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Core/PotatoEnums.h"
#include "PotatoDotComponent.generated.h"

/**
 * UPotatoDotComponent
 * - Target(Owner)에 DOT(지속 피해)를 적용하는 최소 구현
 * - Tick 없이 Timer로만 구동
 * - ApplyDot(Source, Dps, Duration, TickInterval, Policy) 단일 API
 *
 * 정책(기본 구현):
 * - RefreshDuration: 현재 DPS 유지, Duration만 갱신
 * - StackDps: DPS 누적(+), Duration은 Max(현재/신규)
 * - ReplaceStronger: 신규 DPS가 더 크면 교체 + Duration 갱신, 아니면 Duration만 갱신
 * - IgnoreIfActive: 활성 중이면 무시
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API UPotatoDotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPotatoDotComponent();

	// DOT 적용
	UFUNCTION(BlueprintCallable, Category="Potato|Combat|DOT")
	void ApplyDot(AActor* Source, float Dps, float Duration, float TickInterval, EMonsterDotStackPolicy Policy);

	// 활성 여부
	UFUNCTION(BlueprintCallable, Category="Potato|Combat|DOT")
	bool IsDotActive() const { return bActive; }

	// 현재 DPS
	UFUNCTION(BlueprintCallable, Category="Potato|Combat|DOT")
	float GetCurrentDps() const { return CurrentDps; }

	// 남은 시간
	UFUNCTION(BlueprintCallable, Category="Potato|Combat|DOT")
	float GetRemainingTime() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// internal tick
	void TickDot();

	// stop/cleanup
	void StopDot();

	// resolve effective policy (enum 변화 대비)
	EMonsterDotStackPolicy SafePolicy(EMonsterDotStackPolicy InPolicy) const { return InPolicy; }

private:
	UPROPERTY() bool bActive = false;

	UPROPERTY() TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY() float CurrentDps = 0.f;
	UPROPERTY() float CurrentTickInterval = 0.5f;

	// World time seconds
	UPROPERTY() double EndTime = 0.0;

	FTimerHandle DotTickTH;
};