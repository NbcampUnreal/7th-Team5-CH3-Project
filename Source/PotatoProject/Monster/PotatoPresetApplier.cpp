#include "PotatoPresetApplier.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"

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

    FName TypeDefaultSkillId = NAME_None;

    Out.BehaviorTree = DefaultBehaviorTree;

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
        TypeDefaultSkillId = TypeRow->DefaultSpecialSkillId;

        // BT override (네 스타일: IsValid/Get, 아니면 LoadSync)
        if (TypeRow->OverrideBehaviorTree.IsValid())
        {
            Out.BehaviorTree = TypeRow->OverrideBehaviorTree.Get();
        }
        else if (!TypeRow->OverrideBehaviorTree.IsNull())
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
        Out.BehaviorTree = DefaultBehaviorTree;
    }

    Out.AttackDamage = TypeBaseAttackDamage;
    Out.AttackRange = TypeBaseAttackRange;
    Out.bIsRanged = TypeIsRanged;

    // WaveBaseHP가 있으면 TypeBaseHP 대신 베이스로 사용(현재 정책 유지)
    const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : TypeBaseHP;

    // -------------------------
    // 2) RankPreset 로드
    // -------------------------
    const FPotatoMonsterRankPresetRow* RankRow = nullptr;
    float RankCooldownMul = 1.f;
    float RankSpecialDmgMul = 1.f;
    FName RankDefaultSkillId = NAME_None;

    if (RankPresetTable)
    {
        const FName RankRowName = GetRankRowName(Rank);
        RankRow = RankPresetTable->FindRow<FPotatoMonsterRankPresetRow>(
            RankRowName, TEXT("PresetApplier::BuildFinalStats(RankPreset)")
        );
    }

    if (!RankRow)
    {
        // Rank 테이블 없으면 기본값으로 마무리 (fallback)
        Out.AppliedHpMultiplier = 1.f;
        Out.MaxHP = BaseHP;
        Out.AppliedMoveSpeedRatio = 0.6f;
        Out.MoveSpeed = PlayerReferenceSpeed * Out.AppliedMoveSpeedRatio * TypeMoveSpeedMul;
        Out.StructureDamageMultiplier = 1.f;
        Out.SpecialSkillId = NAME_None;
        Out.SpecialLogic = EMonsterSpecialLogic::None;
        Out.SpecialCooldown = 0.f;
        Out.SpecialDamageMultiplier = 1.f;
        return Out;
    }

    // HP multiplier 랜덤(스폰 1회)
    float HpMul = RankRow->HpMultiplierMin;
    if (RankRow->HpMultiplierMax > RankRow->HpMultiplierMin + KINDA_SMALL_NUMBER)
    {
        HpMul = FMath::FRandRange(RankRow->HpMultiplierMin, RankRow->HpMultiplierMax);
    }

    Out.AppliedHpMultiplier = HpMul;
    Out.MaxHP = BaseHP * Out.AppliedHpMultiplier;

    Out.AppliedMoveSpeedRatio = RankRow->MoveSpeedRatioToPlayer;

    // Boss 포함 랭크별 값으로 통일: RankPreset의 Ratio를 사용
    Out.MoveSpeed = PlayerReferenceSpeed * Out.AppliedMoveSpeedRatio * TypeMoveSpeedMul;

    Out.StructureDamageMultiplier = RankRow->StructureDamageMultiplier;

    RankDefaultSkillId = RankRow->DefaultSpecialSkillId;
    RankCooldownMul = RankRow->SpecialCooldownMultiplier;
    RankSpecialDmgMul = RankRow->SpecialDamageMultiplier;

    // -------------------------
    // 3) SpecialSkillId 결정 (Type > Rank)
    // -------------------------
    Out.SpecialSkillId =
        (TypeDefaultSkillId != NAME_None) ? TypeDefaultSkillId :
        (RankDefaultSkillId != NAME_None) ? RankDefaultSkillId :
        NAME_None;

    Out.SpecialLogic = EMonsterSpecialLogic::None;
    Out.SpecialCooldown = 0.f;
    Out.SpecialDamageMultiplier = 1.f;

    // -------------------------
    // 4) SpecialSkillPreset 적용 (수치/로직)
    // -------------------------
    if (Out.SpecialSkillId != NAME_None && SpecialSkillPresetTable)
    {
        const FPotatoMonsterSpecialSkillPresetRow* SkillRow =
            SpecialSkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
                Out.SpecialSkillId, TEXT("PresetApplier::BuildFinalStats(SkillPreset)")
            );

        if (SkillRow)
        {
            Out.SpecialLogic = SkillRow->Logic;
            Out.SpecialCooldown = SkillRow->Cooldown * RankCooldownMul;
            Out.SpecialDamageMultiplier = SkillRow->DamageMultiplier * RankSpecialDmgMul;
        }
        else
        {
            Out.SpecialSkillId = NAME_None;
            Out.SpecialLogic = EMonsterSpecialLogic::None;
            Out.SpecialCooldown = 0.f;
            Out.SpecialDamageMultiplier = 1.f;
        }
    }

    return Out;
}
