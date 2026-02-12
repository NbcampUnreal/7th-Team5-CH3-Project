#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"

#include "../Core/PotatoEnums.h"
#include "PotatoMonsterFinalStats.h"

#include "PotatoMonster.generated.h"

class UPotatoCombatComponent;

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
};
