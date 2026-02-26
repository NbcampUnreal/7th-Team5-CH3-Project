// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PotatoCrosshairBase.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "Combat/PotatoWeaponData.h"
#include "PotatoPlayerHUD.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UBorder;
class APotatoPlaceableStructure;
class APotatoPlayerCharacter;
class APotatoGameMode;
class UBuildingSystemComponent;
class UPotatoCrosshairBase;
class UPotatoWeaponComponent;
class UPotatoWeaponData;
class UPotatoDayNightCycle;
class UPotatoDialogueWidget;
class UPotatoResourceManager;

/**
 * WBP_PlayerHUD
 *
 * [기능 요약]
 * 1. DayClockNeedle: -80~80도 회전으로 하루 전체 진행도 표시
 * 2. StorageHPBar:   맵 중앙 창고 BP의 HP 반영
 * 3. BuildMode 패널(Border_0): 빌드 모드 진입 시 표시, 선택 건물 Border 강조(초록)
 * 4. WeaponSelectBox(HorizontalBox_0, Border_2~5): 선택 무기 Border 강조(초록)
 */
UCLASS()
class POTATOPROJECT_API UPotatoPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

	// ================================================================
	// Lifecycle
	// ================================================================
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ================================================================
	// BindWidgets
	// ================================================================
public:
	// ---- 자원 텍스트 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> ResourceWood;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> ResourceStone;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> ResourceCrop;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> ResourceLivestock;

	// ---- 미니 자원 아이콘 행 (HorizontalBox_282) ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> WoodAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> StoneAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> CropAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> LivestockAmount;

	// ---- 시간 텍스트 ----
	/** 남은 시간 카운트다운 (MM:SS) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> RemainingTime;

	/** Day N */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> CurrentDay;

	/** 탄약 "현재/예비" */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> Ammo;

	/** 플레이어 체력 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> HP;

	// ---- 안내 메시지 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> MessageText;

	// ---- HP 바 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UProgressBar> StorageHPBar;

	// ---- 시계 바늘 ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> DayClockNeedle;

	// ---- 빌드 모드 패널 (건물 선택 슬롯) ----
	/** 빌드 모드 전체 컨테이너 Border */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> Border_0;

	/** 건물 슬롯 Border 배열 (순서: LumberMill/Mine/Farm/Barn/Fence/Barrel/SpikeFence/Wagon/StoneFence) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> Border_2; // 슬롯 0 – LumberMill

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> Border_3; // 슬롯 1 – Mine

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> Border_4; // 슬롯 2 – Farm

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UBorder> Border_5; // 슬롯 3 – Barn

	// ---- 무기 선택 박스 ----
	/** 무기 슬롯 Border 배열 (HorizontalBox_0 내부: 감자/옥수수/호박/당근) */
	// (HorizontalBox_0 자체는 bIsVariable=False 이므로 BindWidget 불필요)

	// ================================================================
	// 외부 연결용 레퍼런스
	// ================================================================

	/** 맵 중앙 창고 BP. BeginPlay 시 자동 탐색하거나 외부에서 직접 할당하세요. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Setup")
	TObjectPtr<APotatoPlaceableStructure> WarehouseStructure;

	// ================================================================
	// 외형 설정 (Blueprint에서 조절 가능)
	// ================================================================

	// ---- 빌드 모드 슬롯 강조 ----
	/** 빌드 모드: 선택된 슬롯 Border 강조 색 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Build")
	FLinearColor BuildSlotSelectedColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.7f);

	/** 빌드 모드: 비선택 슬롯 Border 기본 색 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Build")
	FLinearColor BuildSlotDefaultColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.15f);

protected:
	// TODO: WBP_PlayerHUD 내부에 자식으로 추가하기
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "UI")
	TObjectPtr<UPotatoDialogueWidget> DialogueWidget;
	
	// Dialogue 데이터 테이블 참조
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDataTable> DialogueDataTable;
	
	// ---- 무기 선택 슬롯 강조 (알파 채널만 조정) ----
	/**
	 * 무기 슬롯: 선택된 Border의 알파 값 (0~1).
	 * WBP에서 칠해 놓은 배경 색의 RGB는 그대로 유지되고 알파만 이 값으로 덮어씁니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Weapon",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WeaponSlotSelectedAlpha = 1.0f;

	/**
	 * 무기 슬롯: 비선택 Border의 알파 값 (0~1).
	 * 낮은 값으로 설정하면 선택되지 않은 슬롯이 반투명하게 보입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Weapon",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WeaponSlotDefaultAlpha = 0.35f;

	/** 시계 바늘 최소 각도(낮 시작) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Clock")
	float ClockNeedleMinAngle = -80.0f;

	/** 시계 바늘 최대 각도(밤 끝) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Clock")
	float ClockNeedleMaxAngle = 80.0f;
	
	// ================================================================
	// Crosshair
	// ================================================================
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Crosshairs")
	TSubclassOf<UPotatoCrosshairBase> ArcSpreadCrosshairClass; // 감자
	
	UPROPERTY(EditDefaultsOnly, Category = "Crosshairs")
	TSubclassOf<UPotatoCrosshairBase> LineSpreadCrosshairClass; // 옥수수
	
	UPROPERTY(EditDefaultsOnly, Category = "Crosshairs")
	TSubclassOf<UPotatoCrosshairBase> CircleCrosshairClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crosshairs")
	TSubclassOf<UPotatoCrosshairBase> StaticCrosshairClass;
	
	TMap<ECrosshairType, TObjectPtr<UPotatoCrosshairBase>> CrosshairIntances;
	
	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* CrosshairContainer;
	
	// ================================================================
	// Ammo Visual Settings
	// ================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Ammo")
	FLinearColor NormalAmmoColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Ammo")
	FLinearColor LowAmmoColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Style|Ammo")
	float LowAmmoPercentage = 0.25f;

	// ================================================================
	// Public API
	// ================================================================
public:
	/** 안내 메시지 텍스트를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetMessageText(const FText& InText, bool bVisible = true);

	/** 창고 HP 바를 수동으로 갱신합니다. (Tick에서 자동 갱신되지만 즉각 호출도 가능) */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RefreshStorageHP();
	
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void PlayDialogue(FName RowName);

	// ================================================================
	// Event Handler & Internal
	// ================================================================
protected:
	UFUNCTION()
	void HandleHPChanged(float CurrentHP, float MaxHP);
	
	UFUNCTION()
	void HandleResourceChanged(EResourceType Type, int32 NewValue);
	
	UFUNCTION()
	void HandleNextDialogueInput();
	
	void HandleWeaponChanged(const UPotatoWeaponData* NewWeaponData);
	void HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo);

private:
	/** 시계 바늘 각도 갱신 */
	void RefreshClockNeedle(float DeltaTime);

	/** 빌드 모드 패널 표시/선택 슬롯 강조 갱신 */
	void RefreshBuildModePanel();

	/** 남은 시간 텍스트 갱신 */
	void RefreshTimeText();
	
	/** Border의 BrushColor를 설정합니다. */
	void SetBorderColor(UBorder* InBorder, const FLinearColor& InColor);

	/** BuildingSystemComponent 반환 (캐시) */
	UBuildingSystemComponent* GetBuildComp() const;

	/** WeaponComponent 반환 (캐시) */
	UPotatoWeaponComponent* GetWeaponComp() const;

	// ---- 캐시 ----
	UPROPERTY()
	TObjectPtr<APotatoPlayerCharacter> CachedPlayer;

	/** 빌드 슬롯 Border 배열 (인덱스 순서: Border_2, Border_3, Border_4, Border_5) */
	TArray<UBorder*> BuildSlotBorders;

	/** 시계 바늘 부드러운 이징용 경과 시간 (-1 = 미초기화) */
	float SmoothElapsedTime = -1.f;
};
