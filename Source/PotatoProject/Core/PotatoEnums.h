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
