// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "NPCPopup.generated.h"

class UTextBlock;
class UScrollBox;
class UButton;
class UNPCListItem;
class UPotatoNPCManagementComp;
class APotatoNPC;
class UNPCListItem;
class UImage;

/**
 * WBP_NPCPopup
 *
 * [WBP 위젯 구조]
 * CanvasPanel_22
 * ├── Border_100            (반투명 배경)
 * ├── Backpanel             (나무 프레임, bIsVariable=False)
 * ├── TitlePanel            (타이틀 이미지, bIsVariable=False)
 * ├── RemainingTime         (TextBlock, "직원 관리")
 * ├── CloseButton           (닫기 버튼)
 * ├── Image                 (종이 테두리, bIsVariable=False)
 * ├── EmployeeAmount        (TextBlock, "고용 현황: 2명")
 * ├── HorizontalBox_284     ("총 일일 유지비:" + 아이콘 + TotalCost)
 * ├── HorizontalBox_283     ("고용 비용:" + 아이콘 + EmployeeCharge)
 * ├── HorizontalBox         ("생산량 증가:" + 아이콘 + ProductionAdd)
 * ├── HorizontalBox_1       ("유지비(밤마다 소모):" + 아이콘 + ConsumeCost)
 * ├── Description_5         (경고 텍스트, bIsVariable=True)
 * ├── Button_GoToMainMenu   ("고용" 버튼)
 * ├── SizeBox_4 → Image_1   (NPC 초상화, bIsVariable=False)
 * └── ScrollBox_79          (NPC 리스트 ×5개 WBP_NPCListItem)
 *
 * [기능]
 * - InitPopup(ManagementComp)으로 건물별 NPC 관리 진입
 * - 건물의 NPCClassMap 첫 번째 키를 CachedNPCType으로 사용
 * - 고용 버튼 클릭 → ManagementComp->HireNPC()
 * - ScrollBox 내 NPCListItem 선택 → 해당 NPC 정보 표시
 */
UCLASS()
class POTATOPROJECT_API UNPCPopup : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 건물의 NPC 관리 컴포넌트로 팝업을 초기화합니다. */
    UFUNCTION(BlueprintCallable, Category = "NPCPopup")
    void InitPopup(UPotatoNPCManagementComp* InManagementComp);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ================================================================
    // BindWidgets — WBP에서 bIsVariable=True인 위젯들
    // ================================================================

    /** 타이틀 텍스트 ("직원 관리") */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> RemainingTime;

    /** 고용 현황 ("고용 현황: N명") */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> EmployeeAmount;

    /** 고용 비용 숫자 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> EmployeeCharge;

    /** 생산량 증가분 숫자 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> ProductionAdd;

    /** 유지비 숫자 (NPC 1인, 밤마다 소모) */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> ConsumeCost;

    /** 총 일일 유지비 숫자 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> TotalCost;

    /** NPC 목록 ScrollBox (5개 WBP_NPCListItem 슬롯) */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UScrollBox> ScrollBox_79;

    /** 닫기 버튼 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> CloseButton;

    /** 고용 버튼 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> Button_GoToMainMenu;

    /* 초상화 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UImage> Image_Portrait;
    
    /* 생산량 증가 표시 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UImage> Image_Product;

    /** 경고/안내 텍스트 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> WarningMessage;

    //instance할 class 원본
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UNPCListItem> NPCListItemClass;

    // 직종별 아이콘
    UPROPERTY(EditDefaultsOnly, Category = "Icons")
    TObjectPtr<UTexture2D> MinerIcon;

    UPROPERTY(EditDefaultsOnly, Category = "Icons")
    TObjectPtr<UTexture2D> LumberjackIcon;

    UPROPERTY(EditDefaultsOnly, Category = "Icons")
    TObjectPtr<UTexture2D> RockIcon;

    UPROPERTY(EditDefaultsOnly, Category = "Icons")
    TObjectPtr<UTexture2D> WoodIcon;

    // ================================================================
    // Internal
    // ================================================================
private:
    UFUNCTION()
    void OnHireButtonClicked();

    UFUNCTION()
    void OnCloseButtonClicked();

    UFUNCTION()
    void OnNPCItemSelected(UNPCListItem* ClickedItem);

    /** ScrollBox 내 NPCListItem들에 NPC 데이터를 세팅합니다. */
    void RefreshNPCList();

    /** 고용 비용 / 생산량 증가 / 유지비 패널을 갱신합니다. */
    void RefreshHireCostPanel();

    /** 전체 NPC의 총 유지비를 갱신합니다. */
    void RefreshTotalCost();

    /** 고용 현황 텍스트를 갱신합니다. */
    void RefreshEmployeeAmount();

    /** 경고 메시지를 표시합니다. */
    void ShowWarning(const FText& Message);

    /** 기본 경고 메시지를 표시합니다. */
    void ShowDefaultWarning();

    // ---- 캐시 ----
    UPROPERTY()
    TObjectPtr<UPotatoNPCManagementComp> ManagementComp;

    UPROPERTY()
    TObjectPtr<UNPCListItem> SelectedListItem;

    /** InitPopup 시 건물의 NPCClassMap에서 자동 결정됩니다. */
    ENPCType CachedNPCType = ENPCType::Lumberjack;

    FTimerHandle WarningResetTimer;
};

