// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PotatoEnums.h"
#include "AmmoPopupWidget.generated.h"

class UTextBlock;
class UProgressBar;
class USlider;
class UButton;
class UImage;
class UPotatoWeaponData;
class UPotatoWeaponComponent;
class UPotatoResourceManager;

UCLASS()
class POTATOPROJECT_API UAmmoPopupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // WeaponComponent 캐시 및 초기 UI 구성
    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void InitPopup(UPotatoWeaponComponent* InWeaponComp);

    // 탄환 변경
    void ChangeAmmo(int index);
    //bool IsSetActive;
    void RefreshAll();
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // 헤더 
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> RemainingTime;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description;

    // 충전 수량 표시
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> ProductionAmmo;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> ConsumeCrop;

    // 보유 자원 
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> WoodAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> StoneAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> CropAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> LivestockAmount;

    // 탄약 선택 버튼 내 현재/최대 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> PotatoAmmoAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> CornAmmoAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> PumpkinAmmoAmount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> CarrotAmmoAmount;

    // 교환 비용 라인
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> Description_6;

    // 교환 시 소비 농작물 수 (e.g. "-1")
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> MinusCropAmount;

    // 교환으로 얻는 탄약 수 (e.g. "+3")
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> PlusAmmoAmount;

    // 진행 바 & 슬라이더
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UProgressBar> Slider;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<USlider> SliderValue;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> CloseButton;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UButton> ChargeButton;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UImage> AmmoImage;

    /** 경고/안내 텍스트 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    TObjectPtr<UTextBlock> WarningMessage;

    UPROPERTY(EditAnywhere, Category = "UI")
    TArray<TObjectPtr<UTexture2D>> AmmoTextures; // 바꿀 텍스트를 에디터에서 할당

    // 탄약 종류 선택 버튼 핸들러 - BP 할당
    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnPotatoButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnCornButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnPumpkinButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnCarrotButtonClicked();

    // 닫기 버튼
    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnCloseButtonClicked();

    // "탄약 충전" 버튼
    UFUNCTION(BlueprintCallable, Category = "AmmoPopup")
    void OnChargeButtonClicked();

private:
    UFUNCTION()
    void OnSliderValueChanged(float Value);

    UFUNCTION()
    void OnResourceChanged(EResourceType Type, int32 NewValue);

    void RefreshSelectionPanel();
    void RefreshResourceDisplay();
    void RefreshAmmoDisplay();
    void ShowWarning(const FText& Message);
    void ShowDefaultWarning();

    // 에디터에서 설정할 무기 DataAsset 레퍼런스
    UPROPERTY(EditDefaultsOnly, Category = "AmmoPopup|Setup")
    TObjectPtr<UPotatoWeaponData> PotatoWeaponData;

    UPROPERTY(EditDefaultsOnly, Category = "AmmoPopup|Setup")
    TObjectPtr<UPotatoWeaponData> CornWeaponData;

    UPROPERTY(EditDefaultsOnly, Category = "AmmoPopup|Setup")
    TObjectPtr<UPotatoWeaponData> PumpkinWeaponData;

    UPROPERTY(EditDefaultsOnly, Category = "AmmoPopup|Setup")
    TObjectPtr<UPotatoWeaponData> CarrotWeaponData;

    // 런타임 캐시
    UPROPERTY()
    TObjectPtr<UPotatoWeaponComponent> WeaponComp;

    UPROPERTY()
    TObjectPtr<UPotatoWeaponData> SelectedWeaponData;

    UPROPERTY()
    TObjectPtr<UPotatoResourceManager> ResourceManager;

    int32 PendingAmmoCount = 0;

    FTimerHandle WarningResetTimer;
};
