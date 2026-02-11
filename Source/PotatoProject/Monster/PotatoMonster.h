#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"

#include "../Core/PotatoEnums.h"
#include "PotatoMonsterRankPresetRow.h"    // FPotatoMonsterRankPresetRow
#include "PotatoMonsterTypePresetRow.h"    // FPotatoMonsterTypePresetRow

#include "PotatoMonster.generated.h"

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
	// Rank / Type (BP 자식에서 고정)
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

	// 특수 스킬 프리셋(행 RowName = SkillId)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UDataTable> SpecialSkillPresetTable = nullptr;

	// =========================
	// Wave / Balance Inject
	// =========================
	// 웨이브 기본 HP(있으면 TypePreset BaseHP 대신 이걸 베이스로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Wave", meta = (ExposeOnSpawn = true))
	float WaveBaseHP = 0.0f;

	// 이동속도 산정 기준(플레이어 기준값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Balance")
	float PlayerReferenceSpeed = 600.0f;

	// =========================
	// AI
	// =========================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UBehaviorTree> ResolvedBehaviorTree = nullptr;

	// =========================
	// Runtime (최종 스탯)
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float Health = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float Speed = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	float AttackDamage = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	float AttackRange = 150.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Combat")
	float StructureDamageMultiplier = 1.0f;

	// =========================
	// Applied (Preset 결과)
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Applied")
	float AppliedHpMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Applied")
	float AppliedMoveSpeedRatio = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Applied")
	EMonsterSpecialLogic AppliedSpecialLogic = EMonsterSpecialLogic::None;

	// =========================
	// Special (Resolve 결과)
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Special")
	FName ResolvedSpecialSkillId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Special")
	float ResolvedSpecialCooldown = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Special")
	float ResolvedSpecialDamageMultiplier = 1.f;

	// =========================
	// Target / State (Spawner 주입)
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Target", meta = (ExposeOnSpawn = true))
	TObjectPtr<AActor> WarehouseActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Target")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|State")
	bool bIsDead = false;

	// =========================
	// Preset Apply
	// =========================
	UFUNCTION(BlueprintCallable, Category = "Monster|Preset")
	void ApplyPresets(); // TypePreset -> RankPreset -> SpecialSkillPreset

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	UBehaviorTree* GetBehaviorTreeToRun() const
	{
		return ResolvedBehaviorTree ? ResolvedBehaviorTree : DefaultBehaviorTree;
	}

	// === Lane Path System ===

/** 현재 레인이 가진 Waypoint 배열 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Lane")
	TArray<TObjectPtr<AActor>> LanePoints;

	/** 현재 진행 중인 Lane 인덱스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Lane")
	int32 LaneIndex = 0;

	/** 현재 이동해야 할 타겟 반환 (Lane → Warehouse 순서) */
	UFUNCTION(BlueprintCallable, Category = "Lane")
	AActor* GetCurrentLaneTarget() const;

	/** 다음 Lane 타겟으로 인덱스 증가 */
	UFUNCTION(BlueprintCallable, Category = "Lane")
	void AdvanceLaneIndex();

	// =========================
	// Combat / State
	// =========================
	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	void Attack(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Monster|Combat")
	void ApplyDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Monster|Target")
	AActor* FindTarget();
	UPROPERTY(VisibleAnywhere, Category = "Preset")
	bool bPresetsApplied = false;
protected:
	void ApplyPresetsFallback();
	static FName GetRankRowName(EMonsterRank InRank);
	static FName GetTypeRowName(EMonsterType InType);
private:
	int32 CurrentLaneIndex = 0;
	UPROPERTY()
	TArray<TObjectPtr<AActor>> LaneTargets;

};
