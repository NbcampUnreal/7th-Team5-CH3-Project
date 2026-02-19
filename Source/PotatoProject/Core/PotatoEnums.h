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
	Monkey UMETA(DisplayName = "원숭이"),
	MushroomSmile UMETA(DisplayName = "웃는버섯"),
	Skeleton UMETA(DisplayName = "스켈레톤"),
	Cactus UMETA(DisplayName = "선인장"),
	MushroomAngry UMETA(DisplayName = "화난버섯"),
	Stingray UMETA(DisplayName = "못난가오리"),
	Slime UMETA(DisplayName = "슬라임"),
	TurtleShell UMETA(DisplayName = "가시거북"),
	Mimic UMETA(DisplayName = "미믹"),
};


UENUM(BlueprintType)
enum class EMonsterRank : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss")
};

/** “어떤 기믹/스킬 로직인지”를 구분하는 키 */
UENUM(BlueprintType)
enum class EMonsterSpecialLogic : uint8
{
	None UMETA(DisplayName = "None"),

	// 이동/상태
	Split UMETA(DisplayName = "Split"), // 슬라임 분열(보통 OnDeath 트리거)
	Charge UMETA(DisplayName = "Charge"), // 돌진
	Teleport UMETA(DisplayName = "Teleport"), // 텔레포트
	Disguise UMETA(DisplayName = "Disguise"), // 위장/은신
	HardenShell UMETA(DisplayName = "HardenShell"), // 껍질(피해감소/무적 등)

	// 공격 형태
	AoESlash UMETA(DisplayName = "AoESlash"), // 광역 베기(부채꼴/원형)
	ContactDoT UMETA(DisplayName = "ContactDoT"), // 접촉 지속피해(오라/힛박스)
	PoisonStingShot UMETA(DisplayName = "PoisonStingShot"), // 독침 발사(투사체/광역)
};

/** 특수 기믹이 “언제” 발동하는지 (테이블에서 조절 가능하게) */
UENUM(BlueprintType)
enum class EMonsterSpecialTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	OnCooldown UMETA(DisplayName = "OnCooldown"), // 쿨마다 시도(일반적인 스킬)
	OnHit UMETA(DisplayName = "OnHit"), // 피격 시
	OnAttack UMETA(DisplayName = "OnAttack"), // 공격 시
	OnDeath UMETA(DisplayName = "OnDeath"), // 사망 시(분열 등)
	OnNearTarget UMETA(DisplayName = "OnNearTarget") // 근접 시(가시/오라 등)
};

/** 범위/형태 */
UENUM(BlueprintType)
enum class EMonsterSpecialShape : uint8
{
	None UMETA(DisplayName = "None"),
	Circle UMETA(DisplayName = "Circle"), // 원형 AoE
	Cone UMETA(DisplayName = "Cone"), // 부채꼴
	Line UMETA(DisplayName = "Line"), // 직선/돌진 판정용
	Projectile UMETA(DisplayName = "Projectile"), // 투사체
	Aura UMETA(DisplayName = "Aura"), // 상시 오라(접촉 도트)
	SelfBuff UMETA(DisplayName = "SelfBuff"), // 자기 버프(껍질)
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
