// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildingSystemComponent.generated.h"

class APotatoPlaceableStructure;
class UPotatoStructureData;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UBuildingSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuildingSystemComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void SetupInputBindings();
	
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CycleStructureAction;
	
	// =================================================================
	// Configuration (Blueprint에서 편집 가능)
	// =================================================================
	
	/** 단일 그리드 셀 크기: 언리얼 유닛 단위 (50.0f = 0.5m)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	float GridUnitSize = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	float MaxBuildDistance = 1500.0f;
	
	/** 사용 가능한 모든 구조물 목록: 순환을 위한 정렬된 리스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	TArray<TObjectPtr<const UPotatoStructureData>> StructureSlots;
	
	// =================================================================
	// State (UI용 읽기 전용)
	// =================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsBuildMode = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 CurrentSlotIndex = 0;
	
	/** 현재 회전 인덱스 (0=0, 1=90, 2=180, 3=270) */
	int32 CurrentRotationIndex = 0;
	
	/** 고스트 액터: 배치 전 시각적 표현 */
	UPROPERTY()
	TObjectPtr<APotatoPlaceableStructure> GhostActor;
	
	// =================================================================
	// Visual
	// =================================================================
	/** 배치가 유효할 때 적용할 머티리얼: 녹색 */
	UPROPERTY(EditDefaultsOnly, Category = "Building|Visuals")
	TObjectPtr<UMaterialInterface> GhostValidMaterial;
	
	/** 배치가 유효하지 않을 때 적용할 머티리얼: 빨간색 */
	UPROPERTY(EditDefaultsOnly, Category = "Building|Visuals")
	TObjectPtr<UMaterialInterface> GhostInvalidMaterial;
	
public:
	/** 빌드 모드 토글: 플레이어 컨트롤러에 의해 호출됨(B키) */
	UFUNCTION(BlueprintCallable, Category = "Building")
	void ToggleBuildMode();
	
private:
	// =================================================================
	// Internal Logic & Input Handlers
	// =================================================================
	
	void OnPlaceStructure(const FInputActionValue& Value);
	void OnRotateStructure(const FInputActionValue& Value);
	void OnCycleStructure(const FInputActionValue& Value);
	
	FVector CalculateSnappedLocation(const FVector& HitLocation) const;
	const UPotatoStructureData* GetSelectedData() const;
    bool CheckPlacementValidity(const FVector& Location, const FRotator& Rotation, const UPotatoStructureData* Data);
	
	void UpdateGhostActorTransform();
	void UpdateGhostActorMaterials();
	void RefreshGhostActorModel();
private:
	bool bIsPlacementValid = false;
};
