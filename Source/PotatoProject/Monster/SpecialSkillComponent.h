#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoMonsterSpecialSkillPresetRow.h" // 네 Row 헤더 경로에 맞춰 조정
#include "SpecialSkillComponent.generated.h"

// 상태
UENUM(BlueprintType)
enum class ESpecialSkillState : uint8
{
	Idle,
	Telegraph,
	Casting,
	Executing,
};

UENUM(BlueprintType)
enum class ESpecialSkillCancelReason : uint8
{
	None,
	TargetInvalid,
	OwnerInvalid,
	Forced,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpecialSkillStateChanged, FName, SkillId, ESpecialSkillState, NewState);

/**
 * USpecialSkillComponent
 * - DataTable 기반 스킬 실행 오케스트레이터
 * - Execute는 기본 NextTick에서 수행(AnimNotify/Timer 직접 실행 금지 원칙 준수)
 * - BP 이벤트는 연출용(스케일/머티리얼/VFX/SFX)
 */
UCLASS(ClassGroup=(Monster), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API USpecialSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpecialSkillComponent();

	// =========================
	// Config
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpecialSkill")
	TObjectPtr<UDataTable> SkillPresetTable = nullptr;

	// =========================
	// Runtime State
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SpecialSkill")
	ESpecialSkillState State = ESpecialSkillState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SpecialSkill")
	FName ActiveSkillId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SpecialSkill")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	// 스킬별 다음 사용 가능 시간(초)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SpecialSkill")
	TMap<FName, double> NextReadyTimeBySkill;

	// 상태 변경 이벤트(필요하면 BT/AnimBP에서 사용)
	UPROPERTY(BlueprintAssignable, Category="SpecialSkill")
	FOnSpecialSkillStateChanged OnStateChanged;

	// =========================
	// BP hooks (연출 전용)
	// =========================
	UFUNCTION(BlueprintImplementableEvent, Category="SpecialSkill")
	void BP_OnTelegraphBegin(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="SpecialSkill")
	void BP_OnCastBegin(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="SpecialSkill")
	void BP_OnExecute(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="SpecialSkill")
	void BP_OnEnd(FName SkillId, bool bCancelled);

	// =========================
	// Public API
	// =========================
	UFUNCTION(BlueprintCallable, Category="SpecialSkill")
	bool TryStartSkill(FName SkillId, AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category="SpecialSkill")
	void CancelSkill(ESpecialSkillCancelReason Reason);
	void EndPlay(EEndPlayReason::Type EndPlayReason);

	UFUNCTION(BlueprintCallable, Category="SpecialSkill")
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category="SpecialSkill")
	bool IsBusy() const;

	UFUNCTION(BlueprintCallable, Category="SpecialSkill")
	bool IsSkillReady(FName SkillId) const;
	
	UFUNCTION()
	void ExecuteSkill_Internal(FPotatoMonsterSpecialSkillPresetRow RowCopy);

	UFUNCTION()
	void EndSkill_Internal_NextTick(bool bCancelled);
	
	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	bool CanTryStartSkill(FName SkillId) const;

protected:
	virtual void BeginPlay() override;

private:
	// timers
	FTimerHandle TelegraphTH;
	FTimerHandle CastTH;

	// NextTick 중복 방지
	bool bQueuedExecuteNextTick = false;
	bool bQueuedEndNextTick = false;

	// ---- helpers ----
	double Now() const;
	bool IsTargetValid(AActor* Target) const;

	const FPotatoMonsterSpecialSkillPresetRow* FindRow(FName SkillId) const;
	bool CheckTrigger(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target) const;

	void SetState(ESpecialSkillState NewState);

	// pipeline
	void BeginTelegraph(const FPotatoMonsterSpecialSkillPresetRow& Row);
	void BeginCast(const FPotatoMonsterSpecialSkillPresetRow& Row);
	void QueueExecuteNextTick(const FPotatoMonsterSpecialSkillPresetRow& Row);

	void QueueEndNextTick(bool bCancelled);
	void EndSkill_Internal(bool bCancelled);

	void ArmCooldown(const FName SkillId, float Cooldown);

	// Execution/Spawn helpers (패치용: Projectile만 포함)
	EMonsterSpecialExecution ResolveExecution(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	UClass* ResolveProjectileClass(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	FTransform ResolveProjectileSpawnTransform(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	AActor* SpawnProjectileFromPreset(const FPotatoMonsterSpecialSkillPresetRow& Row) const;

	// FX 거리 게이트(간단 훅)
	bool PassesFxDistanceGate(const FVector& SpawnLoc, const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	

};