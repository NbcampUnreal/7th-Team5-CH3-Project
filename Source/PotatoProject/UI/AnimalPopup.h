// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "AnimalPopup.generated.h"

class UTextBlock;
class UCheckBox;
class UScrollBox;
class UButton;
class USizeBox;
class UAnimalListItem;
class UPotatoAnimalManagementComp;
class UPotatoResourceManager;
class APotatoAnimal;

UCLASS()
class POTATOPROJECT_API UAnimalPopup : public UUserWidget
{
    GENERATED_BODY()

public:
    // 팝업 열 때 외부에서 AnimalManagementComp 주입
    UFUNCTION(BlueprintCallable, Category = "AnimalPopup")
    void InitPopup(UPotatoAnimalManagementComp* InManagementComp);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // 타이틀
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> RemainingTime;

    // 동물 선택 CheckBox (Radio 방식)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCheckBox> CowBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCheckBox> PigBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UCheckBox> ChickenBox;

    // 보유 마릿수
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> CattleAmount;

    // 구매 비용 (Crop)
    // BindWidget 이름이 WBP에서 변경 예정 (현재 EmployeeCharge)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> EmployeeCharge;

    // 생산량 증가분 (선택된 동물 종류 기준)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> WoodBox_1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> AddWood;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> StoneBox_1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> AddStone;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> CropBox_1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> AddCrop;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> LivestockBox_1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> AddLiveStock;

    // 전체 총 생산량
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> WoodBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> TotalWoodAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> StoneBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> TotalStoneAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> CropBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> TotalCropAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USizeBox> LivestockBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> TotalLivestockAmount;

    // 동물 목록 ScrollBox
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UScrollBox> AnimalList;

    // 버튼
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> BuyButton;      // 구매

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> SlaughterButton;    // 도축

    //instance할 class 원본
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UAnimalListItem> AnimalListItemClass;
private:
    // 이벤트 핸들러
    UFUNCTION()
    void OnCowBoxChanged(bool bIsChecked);
    UFUNCTION()
    void OnPigBoxChanged(bool bIsChecked);
    UFUNCTION()
    void OnChickenBoxChanged(bool bIsChecked);

    UFUNCTION()
    void OnBuyButtonClicked();
    UFUNCTION()
    void OnSlaughterButtonClicked();
    UFUNCTION()
    void OnCloseButtonClicked();

    UFUNCTION()
    void OnAnimalItemSelected(UAnimalListItem* SelectedItem);

    // 내부 갱신 함수
    void SelectAnimalType(EAnimalType NewType);
    void RefreshAnimalList();
    void RefreshBuyCostPanel();
    void RefreshTotalProduction();
    void RefreshCattleAmount();

    // 런타임 상태
    UPROPERTY()
    TObjectPtr<UPotatoAnimalManagementComp> ManagementComp;

    UPROPERTY()
    TObjectPtr<UPotatoResourceManager> ResourceManager;

    UPROPERTY()
    TObjectPtr<UAnimalListItem> SelectedListItem;

    EAnimalType SelectedAnimalType = EAnimalType::Cow;
};
