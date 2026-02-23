#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/WidgetComponent.h"
#include "../Core/PotatoEnums.h"
#include "PotatoMonsterFinalStats.h"
#include "PotatoMonsterAnimSet.h"
#include "PotatoMonster.generated.h"

class UPotatoCombatComponent;
class UPotatoMonsterAnimSet;
class UHealthBar;
class UCapsuleComponent;

UCLASS()
class POTATOPROJECT_API APotatoMonster : public ACharacter
{
	GENERATED_BODY()

public:
	APotatoMonster();

protected:
	virtual void BeginPlay() override;

public:
	// =========================
	// Rank / Type
	// =========================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Config")
	EMonsterRank Rank = EMonsterRank::Normal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Config")
	EMonsterType MonsterType = EMonsterType::None;

	// =========================
	// DataTables
	// =========================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UDataTable> RankPresetTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UDataTable> TypePresetTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UDataTable> SpecialSkillPresetTable = nullptr;

	// =========================
	// Wave / Balance Inject
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Wave", meta = (ExposeOnSpawn = true))
	float WaveBaseHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Balance")
	float PlayerReferenceSpeed = 600.0f;

	// =========================
	// AI
	// =========================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UBehaviorTree> ResolvedBehaviorTree = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	UBehaviorTree* GetBehaviorTreeToRun() const
	{
		return ResolvedBehaviorTree ? ResolvedBehaviorTree : DefaultBehaviorTree;
	}

	// =========================
	// FinalStats
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	FPotatoMonsterFinalStats FinalStats;

	// =========================
	// HP / Death
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float Health = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|State")
	bool bIsDead = false;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void Die();

	// =========================
	// Target (Spawner inject)
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Target", meta = (ExposeOnSpawn = true))
	TObjectPtr<AActor> WarehouseActor = nullptr;

	// =========================
	// Preset Apply
	// =========================
	UFUNCTION(BlueprintCallable, Category = "Monster|Preset")
	void ApplyPresetsOnce();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Preset")
	bool bPresetsApplied = false;

	// =========================
	// Lane
	// =========================
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Lane")
	TArray<TObjectPtr<AActor>> LanePoints;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Lane")
	int32 LaneIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Lane")
	AActor* GetCurrentLaneTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Lane")
	void AdvanceLaneIndex();

	// =========================
	// Components
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Components")
	TObjectPtr<UPotatoCombatComponent> CombatComp = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Monster|Anim")
	TObjectPtr<UAnimMontage> BasicAttackMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<UPotatoMonsterAnimSet> AnimSet = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Anim")
	UPotatoMonsterAnimSet* GetAnimSet() const { return AnimSet; }

	void SetAnimSet(UPotatoMonsterAnimSet* InSet);


	// ===== HitReact Tuning =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|HitReact", meta = (AllowPrivateAccess = "true"))
	float HitReactCooldown = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|HitReact", meta = (AllowPrivateAccess = "true"))
	float HitReactMaxStunTime = 0.6f; // 너무 길어지지 않게 클램프용

	float LastHitReactTime = -1000.f;

	FTimerHandle HitReactResumeTH;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HPBarWidgetComp = nullptr;

	UPROPERTY(Transient)
	UHealthBar* HPBarWidget = nullptr;

	// 머리 위 추가 여유 (기본 20)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarExtraZ = 20.f;

	// 너무 작은/큰 몬스터 대비 클램프
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarMinZ = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarMaxZ = 400.f;

	// 예외 몬스터만 BP에서 추가 보정
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarPerMonsterOffsetZ = 0.f;

	// HP바 갱신 (Health/MaxHealth 기반)
	void RefreshHPBar();

	// Mesh Bounds 기반 자동 배치
	void UpdateHPBarLocation();


	UPROPERTY(EditAnywhere, Category = "SFX|Budget")
	float HitSFXNearDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "SFX|Budget")
	float HitSFXFarDistance = 1400.f;

	UPROPERTY(EditAnywhere, Category = "SFX|Budget", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitSFXFarChance = 0.20f;

	UPROPERTY(EditAnywhere, Category = "SFX|Budget")
	float DeathSFXNearDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "SFX|Budget")
	float DeathSFXFarDistance = 2200.f;

	UPROPERTY(EditAnywhere, Category = "SFX|Budget", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathSFXFarChance = 0.35f;

	// Hit SFX 스팸 방지
	UPROPERTY(EditAnywhere, Category = "SFX|Hit")
	float HitSFXCooldown = 0.10f;

	float LastHitSFXTime = -9999.f;

	UPROPERTY(EditDefaultsOnly, Category = "Anim|HitReact")
	float HitReactMinVisibleTime = 0.35f; // 추천: 0.25~0.45

	UPROPERTY(EditDefaultsOnly, Category = "Anim|Death")
	float DeathMinVisibleTime = 2.0f;     // 너가 원한 최소 노출시간

	UPROPERTY()
	class APotatoDamageTextPoolActor* DamageTextPool = nullptr;

	int32 DamageStackIndex = 0;
	float LastDamageTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float DamageStackResetTime = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float DamageStackOffsetStep = 18.f;

private:
		// -------------------------
		// Hit React Helpers
		// -------------------------
		void TryPlayHitReact();
		void ResumeMovementAfterHitReact();

		// -------------------------
		// AI Stop Helper
		// -------------------------
		void StopAIForDead();

};
