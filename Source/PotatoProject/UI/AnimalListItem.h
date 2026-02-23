// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "AnimalListItem.generated.h"

class UTextBlock;
class UImage;
class UHorizontalBox;
class USizeBox;
class UCheckBox;
class APotatoAnimal;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimalItemSelected, UAnimalListItem*, SelectedItem);

UCLASS()
class POTATOPROJECT_API UAnimalListItem : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetAnimalData(APotatoAnimal* InAnimal);

    // 선택/해제 상태 — CheckBox_106 Checked 상태를 직접 제어
    UFUNCTION(BlueprintCallable, Category = "AnimalListItem")
    void SetSelected(bool bInSelected);

    UFUNCTION(BlueprintPure, Category = "AnimalListItem")
    APotatoAnimal* GetAnimal() const { return CachedAnimal; }

    UFUNCTION(BlueprintPure, Category = "AnimalListItem")
    bool HasAnimal() const { return CachedAnimal != nullptr; }

    // AnimalPopup에서 구독
    UPROPERTY(BlueprintAssignable, Category = "AnimalListItem")
    FOnAnimalItemSelected OnItemSelected;

protected:
    virtual void NativeConstruct() override;

    // == 루트 CheckBox (ToggleButton, WBP 전체 배경) ===========
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCheckBox> CheckBox_106;

    // == 데이터 있을 때 표시 ====================================
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UHorizontalBox> AnimalDataBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UImage> AnimalImage;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> AnimalNameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description_6;

    // 아이콘 SizeBox — 값이 0이면 함께 Collapsed
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

    // == 데이터 없을 때 표시 ====================================
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UHorizontalBox> EmptyBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description;

    // == 동물 아이콘 텍스처 (WBP Detail에서 할당) ==============
    UPROPERTY(EditDefaultsOnly, Category = "AnimalListItem|Icons")
    TObjectPtr<UTexture2D> CowIcon;

    UPROPERTY(EditDefaultsOnly, Category = "AnimalListItem|Icons")
    TObjectPtr<UTexture2D> PigIcon;

    UPROPERTY(EditDefaultsOnly, Category = "AnimalListItem|Icons")
    TObjectPtr<UTexture2D> ChickenIcon;

private:
    UFUNCTION()
    void OnCheckBoxStateChanged(bool bIsChecked);

    void ShowAnimalData(APotatoAnimal* InAnimal);
    void ShowEmptyState();

    // 아이콘+텍스트 쌍을 값에 따라 표시/숨김
    void SetResourceVisible(USizeBox* IconBox, UTextBlock* AmountText, int32 Value);

    UPROPERTY()
    TObjectPtr<APotatoAnimal> CachedAnimal = nullptr;
};
