#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PotatoMonsterFinalStats.h"
#include "PotatoMonsterSpecialSkillPresetRow.h"

#include "SpecialSkillComponent.generated.h"

UENUM(BlueprintType)
enum class ESpecialSkillState : uint8
{
	Idle,
	Telegraph,
	Casting,
	Executing
};

UENUM(BlueprintType)
enum class ESpecialSkillCancelReason : uint8
{
	None,
	OwnerInvalid,
	TargetInvalid
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpecialSkillStateChanged, FName, SkillId, ESpecialSkillState, NewState);

UCLASS(ClassGroup=(Potato), meta=(BlueprintSpawnableComponent))
class POTATOPROJECT_API USpecialSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpecialSkillComponent();

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -----------------------------
	// Internal helpers
	// -----------------------------
	double Now() const;
	bool IsTargetValid(AActor* Target) const;

	const FPotatoMonsterSpecialSkillPresetRow* FindRow(FName SkillId) const;
	void SetState(ESpecialSkillState NewState);

	// 계산/트리거 (Execution helper가 일부 사용)
	float ComputeBaseDamage(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	float ComputeFinalDamage(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	float ComputeFinalCooldown(const FPotatoMonsterSpecialSkillPresetRow& Row) const;
	bool  CheckTrigger(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target) const;

	void ArmCooldown(const FName SkillId, float CooldownSeconds);

	// HitOnce
	bool ConsumeHitOnce(AActor* Victim);

	// Presentation facade
	void PlayPresentation_Telegraph(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	void PlayPresentation_Cast(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	void PlayPresentation_Execute(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);
	void PlayPresentation_End(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target);

	// Execution facade
	void ExecuteResolved(const FPotatoMonsterSpecialSkillPresetRow& RowCopy, AActor* ResolvedTarget);

	// State machine
	void BeginTelegraph(const FPotatoMonsterSpecialSkillPresetRow& Row);
	void BeginCast(const FPotatoMonsterSpecialSkillPresetRow& Row);
	void QueueExecuteNextTick(const FPotatoMonsterSpecialSkillPresetRow& Row);
	void ExecuteSkill_Internal(FPotatoMonsterSpecialSkillPresetRow RowCopy);

	void QueueEndNextTick(bool bCancelled);
	void EndSkill_Internal_NextTick(bool bCancelled);
	void EndSkill_Internal(bool bCancelled);
	void CancelSkill(ESpecialSkillCancelReason Reason);

	// Attack Proc gate (OnAttack)
	/*bool PassAttackProcGate();*/

public:
	// ============================================================
	// 정석 API: 주입은 단 한 번 (Monster::ApplyPresetsOnce 직후)
	// ============================================================
	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	void InitFromFinalStats(const FPotatoMonsterFinalStats& Stats);

	// ============================================================
	// 정석 API: 외부는 트리거만 호출 (SkillId/Scale을 건드리지 않음)
	// ============================================================
	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill|Trigger")
	bool TryStartOnCooldown(AActor* Target);

	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill|Trigger")
	bool TryStartOnHit(AActor* Target);

	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill|Trigger")
	bool TryStartOnDeath(AActor* Target);

	/*UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill|Trigger")
	bool TryStartOnAttackProc(AActor* Target);*/

	// ============================================================
	// Low-level API (엔진): SkillId 직접 호출 (가능하면 래퍼 사용 권장)
	// ============================================================
	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	bool CanTryStartSkill(FName SkillId) const;

	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	bool TryStartSkill(FName SkillId, AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	bool IsBusy() const;

	// Debug / observers
	UPROPERTY(BlueprintAssignable, Category="Potato|SpecialSkill")
	FOnSpecialSkillStateChanged OnStateChanged;

	// 스킬 테이블 (DataTable)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill")
	TObjectPtr<UDataTable> SkillPresetTable = nullptr;
	
	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	void CancelActiveSkill();

	// -----------------------------
	// Runtime state
	// -----------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill")
	ESpecialSkillState State = ESpecialSkillState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill")
	FName ActiveSkillId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UFUNCTION(BlueprintCallable, Category="Potato|SpecialSkill")
	FName GetActiveSkillId() const { return ActiveSkillId; }
	// -----------------------------
	// Injected config (from FinalStats)
	// -----------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected")
	FName DefaultSkillId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected")
	float SpecialCooldownScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected")
	float SpecialDamageScale = 1.0f;

	// “기본 데미지” 기준 (프로젝트 정책에 맞게: AttackDamage를 주입하는 걸 추천)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected")
	float DefaultBaseDamage = 10.f;

	/*// OnAttack Proc (Injected)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected|Proc")
	bool bEnableOnAttackProc = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected|Proc")
	float OnAttackProcChance = 0.20f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Potato|SpecialSkill|Injected|Proc")
	float OnAttackProcCooldown = 1.50f;*/

	// -----------------------------
	// Timers / queues
	// -----------------------------
	FTimerHandle TelegraphTH;
	FTimerHandle CastTH;

	bool bQueuedExecuteNextTick = false;
	bool bQueuedEndNextTick = false;
	bool bPlayedSkillMontageThisSession = false;
	bool TryPlaySkillAttackMontage(const FPotatoMonsterSpecialSkillPresetRow& Row);
	// per-skill cooldown
	TMap<FName, double> NextReadyTimeBySkill;

	// HitOnce per skill
	TSet<TWeakObjectPtr<AActor>> HitOnceSet;

	// Attack proc cooldown gate
	double NextAttackProcReadyTime = 0.0;

	// Owner cache
	TWeakObjectPtr<AActor> CachedOwner;

	// -----------------------------
	// Blueprint hooks (optional)
	// -----------------------------
	UFUNCTION(BlueprintImplementableEvent, Category="Potato|SpecialSkill|BP")
	void BP_OnTelegraphBegin(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="Potato|SpecialSkill|BP")
	void BP_OnCastBegin(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="Potato|SpecialSkill|BP")
	void BP_OnExecute(FName SkillId);

	UFUNCTION(BlueprintImplementableEvent, Category="Potato|SpecialSkill|BP")
	void BP_OnEnd(FName SkillId, bool bCancelled);
	
protected:
	// 쿨다운 체크 (누락 방지)
	bool IsSkillReady(FName SkillId) const;

	// 스킬 시도 세션 가드
	uint32 SkillSessionId = 0;

	// 현재 세션인지 확인
	bool IsSession(uint32 InSession) const { return SkillSessionId == InSession; }

	// Row 기준으로 타겟 resolve (End/Execute/Trigger 공통)
	AActor* ResolveTargetForRow(const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* InTarget) const;

	// 캔슬 이유 저장(디버그용)
	ESpecialSkillCancelReason LastCancelReason = ESpecialSkillCancelReason::None;
};