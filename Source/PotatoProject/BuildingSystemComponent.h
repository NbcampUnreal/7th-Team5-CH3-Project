// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingSystemComponent.generated.h"

class APotatoPlaceableStructure;
class UPotatoStructureData;
class UInputMappingContext;
class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UBuildingSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuildingSystemComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// =================================================================
	// Input Configuration (Enhanced Input)
	// =================================================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> BuildIMC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PlaceStructureAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RotateStructureAction;
	
	// 디버깅용 슬롯 선택 IA
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectSlotAction;
	
	// =================================================================
	// Configuration (Blueprint에서 편집 가능)
	// =================================================================
	
	/** 단일 그리드 셀 크기: 언리얼 유닛 단위 (50.0f = 0.5m)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	float GritUnitSize = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	float MaxBuildDistance = 1000.0f;
	
	/** 디버깅용 슬롯*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	TArray<TObjectPtr<const UPotatoStructureData>> StructureSlots;
	
	// =================================================================
	// State (UI/애니메이션용 읽기 전용)
	// =================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	bool bIsBuildMode = false;
	
	/** 현재 배치하려는 구조물의 데이터 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	const UPotatoStructureData* CurrentStructureData;
	
	/** 고스트 액터: 배치 전 시각적 표현 */
	UPROPERTY()
	APotatoPlaceableStructure* CurrentGhostActor;
	
	/** 현재 회전 인덱스 (0=0, 1=90, 2=180, 3=270) */
	int32 CurrentRotationIndex = 0;
	
	// =================================================================
	// Core Logic API
	// =================================================================
	
public:
	/** 빌드 모드 토글: 캐릭터에 의해 호출됨(B키) */
	UFUNCTION(BlueprintCallable, Category = "Building")
	void ToggleBuildMode(bool bEnable);
	
	/** 건물 유형 선택: 입력(키 1~4) 또는 UI(위젯 클릭)에 의해 호출됨 */
	UFUNCTION(BlueprintCallable, Category = "Building")
	void SelectStructure(const UPotatoStructureData* NewData);
	
private:
	// =================================================================
	// Internal Logic & Input Handlers
	// =================================================================
};
