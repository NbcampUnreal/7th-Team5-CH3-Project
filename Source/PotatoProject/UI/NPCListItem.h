// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "NPCListItem.generated.h"

class UTextBlock;
class UImage;
class UHorizontalBox;
class USizeBox;
class UCheckBox;
class APotatoNPC;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCItemSelected, class UNPCListItem*, SelectedItem);

UCLASS()
class POTATOPROJECT_API UNPCListItem : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetNPCData(APotatoNPC* InNPC);
    void SetSelected(bool bInSelected);
    APotatoNPC* GetNPC() const { return CachedNPC; }
    bool HasNPC() const { return CachedNPC != nullptr; }

    UPROPERTY(BlueprintAssignable, Category = "NPCListItem")
    FOnNPCItemSelected OnItemSelected;

protected:
    virtual void NativeConstruct() override;

    // CheckBox ToggleButton — 선택 시각 처리
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCheckBox> CheckBox_0;

    // NPC 데이터 표시 패널
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UHorizontalBox> NPCDataBox;

    // NPC 이미지
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UImage> Image_99;

    // NPC 이름 + 유지비 설명 ("벌목꾼 유지비(매일 밤):")
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description_7;

    // 유지비 아이콘 SizeBox
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> SizeBox_1;

    // 유지비 수량
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> ConsumeAmount;

    // 생산량 행 라벨 ("생산량(1분):")
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description_6;

    // 생산량 아이콘+수량 — 0이면 Collapsed
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> WoodBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> WoodAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> StoneBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> StoneAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> CropBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> CropAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> LivestockBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> LivestockAmount;

    // 빈 슬롯 패널 ("[없음]")
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UHorizontalBox> EmptyBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description;

    // NPC 직종별 아이콘 (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category = "NPCListItem|Icons")
    TObjectPtr<UTexture2D> LumberjackIcon;

    UPROPERTY(EditDefaultsOnly, Category = "NPCListItem|Icons")
    TObjectPtr<UTexture2D> MinerIcon;

private:
    UFUNCTION()
    void OnCheckBoxStateChanged(bool bIsChecked);

    void ShowNPCData(APotatoNPC* InNPC);
    void ShowEmptyState();
    void SetResourceVisible(USizeBox* IconBox, UTextBlock* AmountText, int32 Value);

    APotatoNPC* CachedNPC = nullptr;
};
