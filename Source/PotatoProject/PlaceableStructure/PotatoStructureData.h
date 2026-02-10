#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PotatoStructureData.generated.h"

class APotatoPlaceableStructure;

/** 설치 가능한 구조물의 카테고리 정의: 낮/밤 규칙을 위해 */
UENUM(BlueprintType)
enum class EStructureCategory : uint8
{
	Building UMETA(ToolTip = "밭, 축사, 벌목장, 광산, 식량 저장고"),
	Barricade UMETA(ToolTip = "나무 울타리, 석벽, 수레, 나무통, 가시 울타리")
};

/**
 * 건설 가능한 구조물에 대한 설정을 정의하는 Data Asset
 * 에디터에서 인스턴스 생성 필요 (우클릭 -> Miscellaneous -> Data Asset)
 */
UCLASS()
class POTATOPROJECT_API UPotatoStructureData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// UI 및 디스플레이 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTexture2D> Icon;
	
	/** 실제로 스폰될 BP 클래스 (e.g. BP_Field, BP_Barrel) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TSubclassOf<APotatoPlaceableStructure> StructureClass;
	
	// 배치 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	FIntPoint GridSize = FIntPoint(1, 1);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	EStructureCategory Category = EStructureCategory::Building;
	
	// 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	int32 ConstructionCpst = 10;
	
	/** 밤에 건설이 가능한지 여부를 결정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bIsDestructible = true;
	
	/** 구조물의 최대 체력: bIsDestructible이 false인 경우 비활성화됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (EditCondition = "bIsDestructible"))
	float MaxHealth = 100.0f;
};
