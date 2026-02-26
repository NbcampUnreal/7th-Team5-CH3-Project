#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "PotatoEnums.generated.h"


UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Wood UMETA(DisplayName = "나무"),
	Stone UMETA(DisplayName = "광석"),
	Crop UMETA(DisplayName = "농작물"),
	Livestock UMETA(DisplayName = "축산물")
};

UENUM(BlueprintType)
enum class EMonsterType : uint8
{
	None UMETA(DisplayName = "None"),
	Rat UMETA(DisplayName = "들쥐"),
	Snake UMETA(DisplayName = "뱀"),
	MushroomSmile UMETA(DisplayName = "웃는버섯"),
	Skeleton UMETA(DisplayName = "스켈레톤"),
	Cactus UMETA(DisplayName = "선인장"),
	MushroomAngry UMETA(DisplayName = "화난버섯"),
	Stingray UMETA(DisplayName = "못난가오리"),
	Slime UMETA(DisplayName = "슬라임"),
	TurtleShell UMETA(DisplayName = "가시거북"),
	Mimic UMETA(DisplayName = "미믹"),
	Spider UMETA(DisplayName = "거미")
};


UENUM(BlueprintType)
enum class EMonsterRank : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss")
};

/** “어떤 기믹/스킬 로직인지”를 구분하는 키 (콘텐츠 키) */
UENUM(BlueprintType)
enum class EMonsterSpecialLogic : uint8
{
	None UMETA(DisplayName = "None"),

	// 이동/상태
	Split       UMETA(DisplayName = "Split"),        // 분열/소환(보통 OnDeath)
	Charge      UMETA(DisplayName = "Charge"),       // 돌진
	Teleport    UMETA(DisplayName = "Teleport"),     // 텔레포트
	Disguise    UMETA(DisplayName = "Disguise"),     // 위장/은신
	HardenShell UMETA(DisplayName = "HardenShell"),  // 껍질(피해감소/무적 등)

	// 공격
	AoESlash        UMETA(DisplayName = "AoESlash"),        // 즉시 AoE (원/부채꼴/라인)
	ContactDOT      UMETA(DisplayName = "ContactDOT"),      // ✅ 표기 통일: 접촉 DOT(오라/가시)
	PoisonStingShot UMETA(DisplayName = "PoisonStingShot"), // 독침(투사체/광역)
};

/** 특수 기믹이 “언제” 발동하는지 */
UENUM(BlueprintType)
enum class EMonsterSpecialTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	OnCooldown   UMETA(DisplayName = "OnCooldown"),    // 쿨마다 시도
	OnHit        UMETA(DisplayName = "OnHit"),         // 피격 시
	OnAttack     UMETA(DisplayName = "OnAttack"),      // 공격 시 (※ 현재 설계에선 Proc 경로에서만 TryStart)
	OnDeath      UMETA(DisplayName = "OnDeath"),       // 사망 시
	OnNearTarget UMETA(DisplayName = "OnNearTarget"),  // 근접 시(거리 조건)
};

/** 판정 모양(범위/형태) */
UENUM(BlueprintType)
enum class EMonsterSpecialShape : uint8
{
	None       UMETA(DisplayName = "None"),
	Circle     UMETA(DisplayName = "Circle"),
	Cone       UMETA(DisplayName = "Cone"),
	Line       UMETA(DisplayName = "Line"),
	Projectile UMETA(DisplayName = "Projectile"),
	Aura       UMETA(DisplayName = "Aura"),
	SelfBuff   UMETA(DisplayName = "SelfBuff"),
};

/** 실행 방식(코드에서 실제로 어떤 실행 루틴을 타는가) */
UENUM(BlueprintType)
enum class EMonsterSpecialExecution : uint8
{
	None UMETA(DisplayName="None"),

	InstantAoE   UMETA(DisplayName="InstantAoE"),    // Circle/Cone/Line 즉시 판정
	Projectile   UMETA(DisplayName="Projectile"),    // 투사체 스폰
	ContactDOT   UMETA(DisplayName="ContactDOT"),    // DOT 적용(대상 or 오라)
	SelfBuff     UMETA(DisplayName="SelfBuff"),      // 자기 버프
	SummonSplit  UMETA(DisplayName="SummonSplit"),   // 분열/소환
	Movement     UMETA(DisplayName="Movement"),      // 돌진/텔레포트 등
};

// ============================================================
// Targeting
// - TargetType은 “선택 방식” + “필터”로 사용
// - 실제 구현은 SpecialSkillComponent::CheckTrigger에서 검증
// ============================================================

UENUM(BlueprintType)
enum class EMonsterSpecialTargetType : uint8
{
	/** 현재 타겟(블랙보드/컴뱃이 넣어준 CurrentTarget) */
	CurrentTarget UMETA(DisplayName="CurrentTarget"),

	/** CurrentTarget이어도 “Player로 필터링” (Tag/Interface/클래스로 필터) */
	PlayerOnly    UMETA(DisplayName="PlayerOnly"),

	/** CurrentTarget이어도 “Structure로 필터링” */
	StructureOnly UMETA(DisplayName="StructureOnly"),

	/** 자기 자신 */
	Self          UMETA(DisplayName="Self"),

	/** 위치 기반(장판/불기둥 등) */
	Location      UMETA(DisplayName="Location"),
};

// ============================================================
// DOT
// ============================================================

UENUM(BlueprintType)
enum class EMonsterDotStackPolicy : uint8
{
	/** DPS 유지 + Duration만 연장(갱신) */
	RefreshDuration UMETA(DisplayName="RefreshDuration"),

	/** 신규 값으로 덮어쓰기(DPS/TickInterval/Duration 모두 신규 기준) */
	OverrideValues  UMETA(DisplayName="OverrideValues"),

	/** 누적(기본 구현: DPS += 신규DPS, Duration은 더 긴 값으로) */
	Stack           UMETA(DisplayName="Stack"),
};

UENUM(BlueprintType)
enum class EBuildingType : uint8
{
	Farm UMETA(DisplayName = "밭"),
	Ranch UMETA(DisplayName = "축사"),
	LumberMill UMETA(DisplayName = "벌목장"),
	Mine UMETA(DisplayName = "광산")
};

UENUM(BlueprintType)
enum class EAnimalType : uint8
{
	Cow UMETA(DisplayName = "소"),
	Pig UMETA(DisplayName = "돼지"),
	Chicken UMETA(DisplayName = "닭")
};

UENUM(BlueprintType)
enum class ENPCType : uint8
{
	Lumberjack UMETA(DisplayName = "벌목꾼"),
	Miner UMETA(DisplayName = "광부")
};

UENUM(BlueprintType)
enum class EBarricadeType : uint8
{
	WoodenFence UMETA(DisplayName = "나무 울타리"),
	StoneWall UMETA(DisplayName = "석벽"),
	Cart UMETA(DisplayName = "수레"),
	Barrel UMETA(DisplayName = "나무통"),
	SpikedFence UMETA(DisplayName = "가시 울타리")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle UMETA(DisplayName = "Idle"), // 발사 or 재장전 준비 완료
	Reloading UMETA(DisplayName = "Reloading"), // 재장전 실행중
	Equipped UMETA(DisplayName = "Equipping"), // 무기 교체중
};

UENUM()
enum class EDayPhase : uint8
{
	Day UMETA(DisplayName = "Day"),
	Evening UMETA(DisplayName = "Evening"),
	Night UMETA(DisplayName = "Night"),
	Dawn UMETA(DisplayName = "Dawn")
};
