#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "PotatoEnums.generated.h"


UENUM(BlueprintType)
enum class EResourceType : uint8
{
    Wood        UMETA(DisplayName = "나무"),
    Stone     UMETA(DisplayName = "광석"),
    Crop   UMETA(DisplayName = "농작물"),
    Livestock        UMETA(DisplayName = "축산물")
};

UENUM(BlueprintType)
enum class EMonsterType : uint8
{
    Zombie        UMETA(DisplayName = "좀비"),
};

UENUM(BlueprintType)
enum class EBuildingType : uint8
{
    Farm        UMETA(DisplayName = "밭"),
    Ranch     UMETA(DisplayName = "축사"),
    LumberMill   UMETA(DisplayName = "벌목장"),
    Mine        UMETA(DisplayName = "광산")
};

UENUM(BlueprintType)
enum class EAnimalType : uint8
{
    Cow        UMETA(DisplayName = "소"),
    Pig     UMETA(DisplayName = "돼지"),
    Chicken   UMETA(DisplayName = "닭")
};

UENUM(BlueprintType)
enum class ENPCType : uint8
{
    Lumberjack        UMETA(DisplayName = "벌목꾼"),
    Miner     UMETA(DisplayName = "광부")
};

UENUM(BlueprintType)
enum class EBarricadeType : uint8
{
    WoodenFence        UMETA(DisplayName = "나무 울타리"),
    StoneWall     UMETA(DisplayName = "석벽"),
    Cart   UMETA(DisplayName = "수레"),
    Barrel        UMETA(DisplayName = "나무통"),
    SpikedFence        UMETA(DisplayName = "가시 울타리")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Potato        UMETA(DisplayName = "감자"),
    Corn     UMETA(DisplayName = "옥수수"),
    Pumpkin   UMETA(DisplayName = "호박"),
    Carrot        UMETA(DisplayName = "당근")
};