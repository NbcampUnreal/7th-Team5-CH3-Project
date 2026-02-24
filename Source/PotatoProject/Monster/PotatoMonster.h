#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/WidgetComponent.h"
#include "Engine/EngineTypes.h"
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

	// =========================
	// HitReact Tuning
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|HitReact", meta = (AllowPrivateAccess = "true"))
	float HitReactCooldown = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|HitReact", meta = (AllowPrivateAccess = "true"))
	float HitReactMaxStunTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Anim|HitReact")
	float HitReactMinVisibleTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Anim|Death")
	float DeathMinVisibleTime = 2.0f;

	// =========================
	// UI
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HPBarWidgetComp = nullptr; // ✅ TObjectPtr로 통일

	UPROPERTY(Transient)
	TObjectPtr<UHealthBar> HPBarWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarExtraZ = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarMinZ = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarMaxZ = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float HPBarPerMonsterOffsetZ = 0.f;

	void RefreshHPBar();
	void UpdateHPBarLocation();

	// =========================
	// SFX Budget
	// =========================
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

	UPROPERTY(EditAnywhere, Category = "SFX|Hit")
	float HitSFXCooldown = 0.10f;

	// =========================
	// VFX Budget (최소)
	// =========================
	UPROPERTY(EditAnywhere, Category = "VFX|Hit")
	float HitVFXCooldown = 0.06f;

	UPROPERTY(EditAnywhere, Category = "VFX|Hit")
	float HitVFXNearDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Hit")
	float HitVFXFarDistance = 1400.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Hit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitVFXFarChance = 0.25f;

	UPROPERTY(EditAnywhere, Category = "VFX|Hit")
	float HitVFXGlobalWindowSec = 0.10f;

	UPROPERTY(EditAnywhere, Category = "VFX|Hit")
	int32 HitVFXGlobalMaxCount = 6;

	UPROPERTY(EditAnywhere, Category = "VFX|Death")
	float DeathVFXCooldown = 0.20f;

	UPROPERTY(EditAnywhere, Category = "VFX|Death")
	float DeathVFXNearDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Death")
	float DeathVFXFarDistance = 2200.f;

	UPROPERTY(EditAnywhere, Category = "VFX|Death", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathVFXFarChance = 0.35f;

	UPROPERTY(EditAnywhere, Category = "VFX|Death")
	float DeathVFXGlobalWindowSec = 0.25f;

	UPROPERTY(EditAnywhere, Category = "VFX|Death")
	int32 DeathVFXGlobalMaxCount = 3;

	// =========================
	// Damage Text
	// =========================
	UPROPERTY()
	TObjectPtr<class APotatoDamageTextPoolActor> DamageTextPool = nullptr;

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

	// -------------------------
	// TakeDamage delegates (HitLocation 수집)
	// -------------------------
	UFUNCTION()
	void OnMonsterTakePointDamage(AActor* DamagedActor, float Damage,
		AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent,
		FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
		AActor* DamageCauser);

	UFUNCTION()
	void OnMonsterTakeRadialDamage(AActor* DamagedActor, float Damage,
		const UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo,
		AController* InstigatedBy, AActor* DamageCauser);

	// -------------------------
	// Runtime State (non-UPROPERTY)
	// -------------------------
	float LastHitReactTime = -1000.f;
	float LastHitSFXTime = -9999.f;
	float LastHitVFXTime = -9999.f;
	float LastDeathVFXTime = -9999.f;

	FTimerHandle HitReactResumeTH;

	int32 DamageStackIndex = 0;
	float LastDamageTime = 0.f;

	bool bHasLastHitPoint = false;
	FVector LastHitPointWS = FVector::ZeroVector;
	FName LastHitBoneName = NAME_None;
};