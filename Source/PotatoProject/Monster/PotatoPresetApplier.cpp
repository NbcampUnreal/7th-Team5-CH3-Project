// PotatoPresetApplier.cpp (Single DefaultSpecialSkillId version) - FINAL (+ Split Fix + Debug Logs)
// - TypePreset: DefaultSpecialSkillId 단일 바인딩
// - RankPreset: SpecialCooldownScale / SpecialDamageScale + Proc 튜닝만
// - SpecialSkillPresetTable: "캐시(Logic/Cooldown/DamageMultiplier 원본)"만 채움 (스케일 곱 X)
//   * 최종 쿨/데미지 스케일 적용은 SpecialSkillComponent에서 ComputeFinal...로 처리하는 것을 정석으로 유지
//
//  FIX:
// - SplitSpec 복사 로직이 RankRow NULL fallback 경로에만 있었음 → 정상 경로에도 동일하게 적용
//  DEBUG:
// - TypeRow/RankRow/SkillRow/DefaultId/Return Split 상태 로그 추가

#include "PotatoPresetApplier.h"

#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "BehaviorTree/BehaviorTree.h"

#include "PotatoMonsterAnimSet.h"
#include "PotatoMonsterTypePresetRow.h"
#include "PotatoMonsterRankPresetRow.h"
#include "PotatoMonsterSpecialSkillPresetRow.h"

#include "UObject/UnrealType.h"
#include "Utils/PotatoPresetApplyUtils.h"

// forward
static UPotatoMonsterAnimSet* LoadAnimSetSync(const TSoftObjectPtr<UPotatoMonsterAnimSet>& SoftPtr);

FName UPotatoPresetApplier::GetRankRowName(EMonsterRank InRank)
{
	switch (InRank)
	{
	case EMonsterRank::Normal: return FName("Normal");
	case EMonsterRank::Elite:  return FName("Elite");
	case EMonsterRank::Boss:   return FName("Boss");
	default:                   return FName("Normal");
	}
}

FName UPotatoPresetApplier::GetTypeRowName(EMonsterType InType)
{
	const UEnum* Enum = StaticEnum<EMonsterType>();
	if (!Enum) return NAME_None;

	const FString Name = Enum->GetNameStringByValue((int64)InType);
	return FName(*Name);
}

FPotatoMonsterFinalStats UPotatoPresetApplier::BuildFinalStats(
	UObject* WorldContextObject,
	EMonsterType MonsterType,
	EMonsterRank Rank,
	float WaveBaseHP,
	float PlayerReferenceSpeed,
	UDataTable* TypePresetTable,
	UDataTable* RankPresetTable,
	UDataTable* SpecialSkillPresetTable,
	UBehaviorTree* DefaultBehaviorTree
)
{
	FPotatoMonsterFinalStats Out;

	// -------------------------
	// 0) 기본값(테이블 없을 때도 동작)
	// -------------------------
	float TypeBaseHP = 100.f;
	float TypeBaseAttackDamage = 10.f;
	float TypeBaseAttackRange = 150.f;
	float TypeMoveSpeedMul = 1.f;
	bool  TypeIsRanged = false;

	// 단일 스킬 바인딩 (Type 기준)
	FName TypeDefaultSkillId = NAME_None;

	Out.BehaviorTree = DefaultBehaviorTree;
	Out.AnimSet = nullptr;

	// Proc 기본값(안전한 디폴트)
	Out.bEnableOnAttackSpecialProc = true;
	Out.OnAttackSpecialChance = 0.20f;
	Out.OnAttackSpecialProcCooldown = 1.50f;

	// 단일 스킬 기본값
	Out.DefaultSpecialSkillId = NAME_None;
	Out.SpecialCooldownScale = 1.0f;
	Out.SpecialDamageScale = 1.0f;

	// (옵션 캐시) BT/BB 호환 및 디버그용: "원본 Row값"만 저장 (스케일 곱 X)
	Out.DefaultSpecialLogic = EMonsterSpecialLogic::None;
	Out.DefaultSpecialCooldown = 0.f;
	Out.DefaultSpecialDamageMultiplier = 1.f;

	// Split 기본값
	Out.bEnableSplit = false;
	Out.SplitSpec = FPotatoSplitSpec();

	// -------------------------
	// 1) TypePreset 로드
	// -------------------------
	const FPotatoMonsterTypePresetRow* TypeRow = nullptr;
	if (TypePresetTable)
	{
		const FName TypeRowName = GetTypeRowName(MonsterType);
		if (TypeRowName != NAME_None)
		{
			TypeRow = TypePresetTable->FindRow<FPotatoMonsterTypePresetRow>(
				TypeRowName, TEXT("PresetApplier::BuildFinalStats(TypePreset)")
			);
		}
	}

	if (TypeRow)
	{
		TypeBaseHP = TypeRow->BaseHP;
		TypeBaseAttackDamage = TypeRow->BaseAttackDamage;
		TypeBaseAttackRange = TypeRow->BaseAttackRange;
		TypeMoveSpeedMul = TypeRow->MoveSpeedMultiplier;
		TypeIsRanged = TypeRow->bIsRanged;

		Out.bEnableHardenShell = TypeRow->bEnableHardenShell;
		Out.HardenDamageMultiplier = FMath::Max(0.f, TypeRow->HardenDamageMultiplier);
		Out.HardenTriggerStepPercent = FMath::Clamp(TypeRow->HardenTriggerStepPercent, 0.01f, 1.0f);
		Out.HardenDurationSeconds    = FMath::Max(0.01f, TypeRow->HardenDurationSeconds);
		Out.HardenTintStrengthParamName = TypeRow->HardenTintStrengthParamName;
		Out.HardenTintStrengthValue     = TypeRow->HardenTintStrengthValue;

		//  단일 DefaultSpecialSkillId
		TypeDefaultSkillId = TypeRow->DefaultSpecialSkillId;

		//  AnimSet 로드/주입 (Type 기준)
		Out.AnimSet = LoadAnimSetSync(TypeRow->AnimSet);

		//  BT override (SoftObjectPtr)
		if (!TypeRow->OverrideBehaviorTree.IsNull())
		{
			Out.BehaviorTree = TypeRow->OverrideBehaviorTree.LoadSynchronous();
		}
		else
		{
			Out.BehaviorTree = DefaultBehaviorTree;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Preset] TypeRow missing. Type=%d Table=%s"),
			(int32)MonsterType, *GetNameSafe(TypePresetTable));

		Out.BehaviorTree = DefaultBehaviorTree;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] Tables Type=%s Rank=%s Skill=%s"),
		*GetNameSafe(TypePresetTable),
		*GetNameSafe(RankPresetTable),
		*GetNameSafe(SpecialSkillPresetTable));

	UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] TypeDefaultSkillId=%s TypeRow=%s"),
		*TypeDefaultSkillId.ToString(),
		TypeRow ? TEXT("OK") : TEXT("NULL"));

	Out.AttackDamage = TypeBaseAttackDamage;
	Out.AttackRange = TypeBaseAttackRange;

	// 원거리 여부 “정답” 통일: AnimSet이 있으면 AnimSet 기준, 없으면 TypeRow 값
	Out.bIsRanged = (Out.AnimSet != nullptr) ? Out.AnimSet->bIsRanged : TypeIsRanged;

	// WaveBaseHP가 있으면 TypeBaseHP 대신 베이스로 사용(정책 유지)
	const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : TypeBaseHP;

	// -------------------------
	// 2) RankPreset 로드
	// -------------------------
	const FPotatoMonsterRankPresetRow* RankRow = nullptr;
	if (RankPresetTable)
	{
		const FName RankRowName = GetRankRowName(Rank);
		RankRow = RankPresetTable->FindRow<FPotatoMonsterRankPresetRow>(
			RankRowName, TEXT("PresetApplier::BuildFinalStats(RankPreset)")
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] RankRow=%s Rank=%d"),
		RankRow ? TEXT("OK") : TEXT("NULL"), (int32)Rank);

	if (!RankRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Preset] RankRow missing. Rank=%d Table=%s (fallback applied)"),
			(int32)Rank, *GetNameSafe(RankPresetTable));

		// Rank 테이블 없으면 기본값 fallback
		Out.AppliedHpMultiplier = 1.f;
		Out.MaxHP = BaseHP;

		// fallback movespeed 정책(기존 유지)
		Out.AppliedMoveSpeedRatio = 0.6f;
		Out.MoveSpeed = PlayerReferenceSpeed * Out.AppliedMoveSpeedRatio * TypeMoveSpeedMul;

		Out.StructureDamageMultiplier = 1.f;

		//  단일 스킬(타입만이라도 반영)
		Out.DefaultSpecialSkillId = TypeDefaultSkillId;
		Out.SpecialCooldownScale = 1.0f;
		Out.SpecialDamageScale = 1.0f;

		//  SkillPreset 캐시 + Split 적용 (원본만 저장)
		if (Out.DefaultSpecialSkillId != NAME_None && SpecialSkillPresetTable)
		{
			const FPotatoMonsterSpecialSkillPresetRow* SkillRow =
				SpecialSkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
					Out.DefaultSpecialSkillId, TEXT("PresetApplier::BuildFinalStats(SkillPreset-NoRank)")
				);

			UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] SkillRow(NoRank)=%s ForId=%s"),
				SkillRow ? TEXT("OK") : TEXT("NULL"), *Out.DefaultSpecialSkillId.ToString());

			if (SkillRow)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] Row(NoRank) Logic=%d Exec=%d bEnableSplit=%d ThNum=%d"),
					(int32)SkillRow->Logic, (int32)SkillRow->Execution,
					SkillRow->bEnableSplit ? 1 : 0,
					SkillRow->SplitThresholdPercents.Num());

				Out.DefaultSpecialLogic = SkillRow->Logic;
				Out.DefaultSpecialCooldown = SkillRow->Cooldown;
				Out.DefaultSpecialDamageMultiplier = SkillRow->DamageMultiplier;

				ApplySplitFromSkillRow(SkillRow, Out, TypeDefaultSkillId);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Preset] SkillRow not found. SkillId=%s Table=%s"),
					*Out.DefaultSpecialSkillId.ToString(), *GetNameSafe(SpecialSkillPresetTable));

				Out.DefaultSpecialSkillId = NAME_None;
				Out.DefaultSpecialLogic = EMonsterSpecialLogic::None;
				Out.DefaultSpecialCooldown = 0.f;
				Out.DefaultSpecialDamageMultiplier = 1.f;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] RETURN(NoRank) Split Enable=%d ThNum=%d DefaultId=%s"),
			Out.bEnableSplit ? 1 : 0,
			Out.SplitSpec.ThresholdPercents.Num(),
			*Out.DefaultSpecialSkillId.ToString());

		// Proc는 위 기본값 유지
		return Out;
	}

	// -------------------------
	// Rank normal path
	// -------------------------
	float HpMul = RankRow->HpMultiplierMin;
	if (RankRow->HpMultiplierMax > RankRow->HpMultiplierMin + KINDA_SMALL_NUMBER)
	{
		HpMul = FMath::FRandRange(RankRow->HpMultiplierMin, RankRow->HpMultiplierMax);
	}

	Out.AppliedHpMultiplier = HpMul;
	Out.MaxHP = BaseHP * Out.AppliedHpMultiplier;

	Out.AppliedMoveSpeedRatio = RankRow->MoveSpeedRatioToPlayer;
	Out.MoveSpeed = PlayerReferenceSpeed * Out.AppliedMoveSpeedRatio * TypeMoveSpeedMul;

	Out.StructureDamageMultiplier = RankRow->StructureDamageMultiplier;

	//  Rank-wide tuning scales (최종 스케일은 SkillComp에서 사용)
	Out.SpecialCooldownScale = FMath::Max(0.01f, RankRow->SpecialCooldownScale);
	Out.SpecialDamageScale = FMath::Max(0.0f, RankRow->SpecialDamageScale);

	// Proc
	Out.bEnableOnAttackSpecialProc = RankRow->bEnableOnAttackSpecialProc;
	Out.OnAttackSpecialChance = RankRow->OnAttackSpecialChance;
	Out.OnAttackSpecialProcCooldown = RankRow->OnAttackSpecialProcCooldown;

	if (TypeRow && TypeRow->bOverrideOnAttackSpecialProc)
	{
		Out.bEnableOnAttackSpecialProc = TypeRow->bEnableOnAttackSpecialProc;
		Out.OnAttackSpecialChance = TypeRow->OnAttackSpecialChance;
		Out.OnAttackSpecialProcCooldown = TypeRow->OnAttackSpecialProcCooldown;
	}

	Out.OnAttackSpecialChance = FMath::Clamp(Out.OnAttackSpecialChance, 0.f, 1.f);
	Out.OnAttackSpecialProcCooldown = FMath::Max(0.f, Out.OnAttackSpecialProcCooldown);

	// -------------------------
	// 3) DefaultSpecialSkillId 결정 (단일 구조: Type only)
	// -------------------------
	Out.DefaultSpecialSkillId = TypeDefaultSkillId;

	// 캐시 초기화(원본 기준)
	Out.DefaultSpecialLogic = EMonsterSpecialLogic::None;
	Out.DefaultSpecialCooldown = 0.f;
	Out.DefaultSpecialDamageMultiplier = 1.f;

	// -------------------------
	// 4) SpecialSkillPreset 캐시 적용 (원본: Logic/Cooldown/DmgMul)
	//    +  SplitSpec도 여기서 같이 반영해야 정상 경로에서 동작함
	// -------------------------
	if (Out.DefaultSpecialSkillId != NAME_None && SpecialSkillPresetTable)
	{
		const FPotatoMonsterSpecialSkillPresetRow* SkillRow =
			SpecialSkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
				Out.DefaultSpecialSkillId, TEXT("PresetApplier::BuildFinalStats(SkillPreset)")
			);

		UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] SkillRow=%s ForId=%s"),
			SkillRow ? TEXT("OK") : TEXT("NULL"),
			*Out.DefaultSpecialSkillId.ToString());

		if (SkillRow)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] Row Logic=%d Exec=%d bEnableSplit=%d ThNum=%d"),
				(int32)SkillRow->Logic, (int32)SkillRow->Execution,
				SkillRow->bEnableSplit ? 1 : 0,
				SkillRow->SplitThresholdPercents.Num());

			Out.DefaultSpecialLogic = SkillRow->Logic;
			Out.DefaultSpecialCooldown = SkillRow->Cooldown;                 //  원본
			Out.DefaultSpecialDamageMultiplier = SkillRow->DamageMultiplier; //  원본

			//  FIX: Split 반영(정상 경로)
			ApplySplitFromSkillRow(SkillRow, Out, TypeDefaultSkillId);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Preset] SkillRow not found. SkillId=%s Table=%s"),
				*Out.DefaultSpecialSkillId.ToString(), *GetNameSafe(SpecialSkillPresetTable));

			Out.DefaultSpecialSkillId = NAME_None;
			Out.DefaultSpecialLogic = EMonsterSpecialLogic::None;
			Out.DefaultSpecialCooldown = 0.f;
			Out.DefaultSpecialDamageMultiplier = 1.f;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] Skip SkillPreset. DefaultId=%s SkillTable=%s"),
			*Out.DefaultSpecialSkillId.ToString(),
			*GetNameSafe(SpecialSkillPresetTable));
	}

	UE_LOG(LogTemp, Warning, TEXT("[PresetDBG] RETURN Split Enable=%d ThNum=%d DefaultId=%s"),
		Out.bEnableSplit ? 1 : 0,
		Out.SplitSpec.ThresholdPercents.Num(),
		*Out.DefaultSpecialSkillId.ToString());

	return Out;
}

// AnimSet 로드(동기)
static UPotatoMonsterAnimSet* LoadAnimSetSync(const TSoftObjectPtr<UPotatoMonsterAnimSet>& SoftPtr)
{
	if (SoftPtr.IsNull()) return nullptr;
	if (UPotatoMonsterAnimSet* Already = SoftPtr.Get()) return Already;

	FStreamableManager& SM = UAssetManager::GetStreamableManager();
	return Cast<UPotatoMonsterAnimSet>(SM.LoadSynchronous(SoftPtr.ToSoftObjectPath()));
}