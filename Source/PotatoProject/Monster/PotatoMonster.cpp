#include "PotatoMonster.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// SpecialSkill row struct 헤더가 cpp에서 필요함 (FindRow 템플릿)
#include "PotatoMonsterSpecialSkillPresetRow.h" // 너의 파일명에 맞춰 수정

FName APotatoMonster::GetRankRowName(EMonsterRank InRank)
{
    switch (InRank)
    {
    case EMonsterRank::Normal: return FName("Normal");
    case EMonsterRank::Elite:  return FName("Elite");
    case EMonsterRank::Boss:   return FName("Boss");
    default:                   return FName("Normal");
    }
}

FName APotatoMonster::GetTypeRowName(EMonsterType InType)
{
    const UEnum* Enum = StaticEnum<EMonsterType>();
    if (!Enum) return NAME_None;

    const FString Name = Enum->GetNameStringByValue((int64)InType);
    return FName(*Name);
}

APotatoMonster::APotatoMonster()
{
    PrimaryActorTick.bCanEverTick = false;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bPresetsApplied = false;
}

void APotatoMonster::BeginPlay()
{
    Super::BeginPlay();

    // ✅ 정석 흐름: ApplyPresets()는 AIController::OnPossess에서 1회 호출
    // 여기서는 호출하지 않음
}

void APotatoMonster::ApplyPresetsFallback()
{
    AttackDamage = 10.0f;
    AttackRange = 150.0f;

    AppliedHpMultiplier = (Rank == EMonsterRank::Elite) ? 2.5f :
        (Rank == EMonsterRank::Boss) ? 10.0f : 1.0f;

    AppliedMoveSpeedRatio = (Rank == EMonsterRank::Elite) ? 0.8f : 0.6f;

    StructureDamageMultiplier = (Rank == EMonsterRank::Elite) ? 1.5f :
        (Rank == EMonsterRank::Boss) ? 3.0f : 1.0f;

    const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : 100.0f;
    MaxHealth = BaseHP * AppliedHpMultiplier;
    Health = MaxHealth;

    Speed = PlayerReferenceSpeed * AppliedMoveSpeedRatio;
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = Speed;
    }

    ResolvedBehaviorTree = DefaultBehaviorTree;

    ResolvedSpecialSkillId = NAME_None;
    AppliedSpecialLogic = EMonsterSpecialLogic::None;
    ResolvedSpecialCooldown = 0.f;
    ResolvedSpecialDamageMultiplier = 1.f;
}

void APotatoMonster::ApplyPresets()
{
    // ✅ 1회 적용 가드 (Spawner/Controller 중복 호출 방지)
    if (bPresetsApplied)
    {
        return;
    }
    bPresetsApplied = true;

    // -------------------------
    // 기본값
    // -------------------------
    float TypeBaseHP = 100.0f;
    float TypeBaseAttackDamage = 10.0f;
    float TypeBaseAttackRange = 150.0f;
    float TypeMoveSpeedMul = 1.0f;

    FName TypeDefaultSkillId = NAME_None;

    ResolvedBehaviorTree = DefaultBehaviorTree;

    // -------------------------
    // 1) TypePreset
    // -------------------------
    const FPotatoMonsterTypePresetRow* TypeRow = nullptr;

    if (TypePresetTable)
    {
        const FName TypeRowName = GetTypeRowName(MonsterType);
        if (TypeRowName != NAME_None)
        {
            TypeRow = TypePresetTable->FindRow<FPotatoMonsterTypePresetRow>(
                TypeRowName, TEXT("APotatoMonster::ApplyPresets(TypePreset)")
            );
        }
    }

    if (TypeRow)
    {
        TypeBaseHP = TypeRow->BaseHP;
        TypeBaseAttackDamage = TypeRow->BaseAttackDamage;
        TypeBaseAttackRange = TypeRow->BaseAttackRange;
        TypeMoveSpeedMul = TypeRow->MoveSpeedMultiplier;

        TypeDefaultSkillId = TypeRow->DefaultSpecialSkillId;

        if (TypeRow->OverrideBehaviorTree.IsValid())
        {
            ResolvedBehaviorTree = TypeRow->OverrideBehaviorTree.Get();
        }
        else if (!TypeRow->OverrideBehaviorTree.IsNull())
        {
            ResolvedBehaviorTree = TypeRow->OverrideBehaviorTree.LoadSynchronous();
        }
        else
        {
            ResolvedBehaviorTree = DefaultBehaviorTree;
        }
    }
    else
    {
        ResolvedBehaviorTree = DefaultBehaviorTree;
    }

    AttackDamage = TypeBaseAttackDamage;
    AttackRange = TypeBaseAttackRange;

    const float BaseHP = (WaveBaseHP > 0.f) ? WaveBaseHP : TypeBaseHP;

    // -------------------------
    // 2) RankPreset
    // -------------------------
    const FPotatoMonsterRankPresetRow* RankRow = nullptr;

    float RankCooldownMul = 1.0f;
    float RankSpecialDmgMul = 1.0f;
    FName RankDefaultSkillId = NAME_None;

    if (RankPresetTable)
    {
        const FName RankRowName = GetRankRowName(Rank);
        RankRow = RankPresetTable->FindRow<FPotatoMonsterRankPresetRow>(
            RankRowName, TEXT("APotatoMonster::ApplyPresets(RankPreset)")
        );
    }

    if (!RankRow)
    {
        ApplyPresetsFallback();
        return;
    }

    float HpMul = RankRow->HpMultiplierMin;
    if (RankRow->HpMultiplierMax > RankRow->HpMultiplierMin + KINDA_SMALL_NUMBER)
    {
        HpMul = FMath::FRandRange(RankRow->HpMultiplierMin, RankRow->HpMultiplierMax);
    }

    AppliedHpMultiplier = HpMul;
    MaxHealth = BaseHP * AppliedHpMultiplier;
    Health = MaxHealth;

    AppliedMoveSpeedRatio = RankRow->MoveSpeedRatioToPlayer;
    Speed = PlayerReferenceSpeed * AppliedMoveSpeedRatio * TypeMoveSpeedMul;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = Speed;
    }

    StructureDamageMultiplier = RankRow->StructureDamageMultiplier;

    RankDefaultSkillId = RankRow->DefaultSpecialSkillId;
    RankCooldownMul = RankRow->SpecialCooldownMultiplier;
    RankSpecialDmgMul = RankRow->SpecialDamageMultiplier;

    // -------------------------
    // 3) 최종 SpecialSkillId 선택 (Type > Rank)
    // -------------------------
    ResolvedSpecialSkillId =
        (TypeDefaultSkillId != NAME_None) ? TypeDefaultSkillId :
        (RankDefaultSkillId != NAME_None) ? RankDefaultSkillId :
        NAME_None;

    AppliedSpecialLogic = EMonsterSpecialLogic::None;
    ResolvedSpecialCooldown = 0.f;
    ResolvedSpecialDamageMultiplier = 1.f;

    // -------------------------
    // 4) SpecialSkillPreset 적용
    // -------------------------
    if (ResolvedSpecialSkillId != NAME_None && SpecialSkillPresetTable)
    {
        const FPotatoMonsterSpecialSkillPresetRow* SkillRow =
            SpecialSkillPresetTable->FindRow<FPotatoMonsterSpecialSkillPresetRow>(
                ResolvedSpecialSkillId, TEXT("APotatoMonster::ApplyPresets(SkillPreset)")
            );

        if (SkillRow)
        {
            AppliedSpecialLogic = SkillRow->Logic;
            ResolvedSpecialCooldown = SkillRow->Cooldown * RankCooldownMul;
            ResolvedSpecialDamageMultiplier = SkillRow->DamageMultiplier * RankSpecialDmgMul;
        }
        else
        {
            ResolvedSpecialSkillId = NAME_None;
            AppliedSpecialLogic = EMonsterSpecialLogic::None;
            ResolvedSpecialCooldown = 0.f;
            ResolvedSpecialDamageMultiplier = 1.f;
        }
    }
}

void APotatoMonster::Attack(AActor* Target)
{
    if (bIsDead || !Target) return;

    const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    if (Dist > AttackRange) return;

    const float FinalDamage = AttackDamage;
    UGameplayStatics::ApplyDamage(Target, FinalDamage, GetController(), this, nullptr);
}

void APotatoMonster::ApplyDamage(float Damage)
{
    if (bIsDead || Damage <= 0.f) return;

    Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
    if (Health <= 0.f) Die();
}

void APotatoMonster::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }
}

AActor* APotatoMonster::FindTarget()
{
    if (CurrentTarget) return CurrentTarget;
    return WarehouseActor;
}

void APotatoMonster::AdvanceLaneIndex()
{
    // ✅ 끝 이후 무한 증가 방지(중요)
    const int32 MaxEndIndex = LanePoints.Num(); // 끝 상태(Num)까지는 허용
    LaneIndex = FMath::Clamp(LaneIndex + 1, 0, MaxEndIndex);
}

AActor* APotatoMonster::GetCurrentLaneTarget() const
{
    if (LanePoints.IsValidIndex(LaneIndex))
    {
        return LanePoints[LaneIndex];
    }

    return WarehouseActor;
}
