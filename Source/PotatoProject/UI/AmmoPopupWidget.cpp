// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AmmoPopupWidget.h"
#include "Combat/PotatoWeaponComponent.h"
#include "Combat/PotatoWeaponData.h"
#include "Core/PotatoResourceManager.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/Button.h"
#include "Components/Image.h"



void UAmmoPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SliderValue)
    {
        SliderValue->OnValueChanged.AddDynamic(this, &UAmmoPopupWidget::OnSliderValueChanged);
    }

    if (UWorld* World = GetWorld())
    {
        ResourceManager = World->GetSubsystem<UPotatoResourceManager>();
        if (ResourceManager)
        {
            ResourceManager->OnResourceChanged.AddDynamic(this, &UAmmoPopupWidget::OnResourceChanged);
        }
    }
    if (CloseButton) {
        //IsSetActive = false;
        CloseButton->OnClicked.AddDynamic(this, &UAmmoPopupWidget::OnCloseButtonClicked);
    }
}

void UAmmoPopupWidget::NativeDestruct()
{
    if (ResourceManager)
    {
        ResourceManager->OnResourceChanged.RemoveDynamic(this, &UAmmoPopupWidget::OnResourceChanged);
    }
    Super::NativeDestruct();
}

void UAmmoPopupWidget::InitPopup(UPotatoWeaponComponent* InWeaponComp)
{
    WeaponComp = InWeaponComp;

    // 기본 선택: 감자
    SelectedWeaponData = PotatoWeaponData;

    RefreshResourceDisplay();
    RefreshAmmoDisplay();
    RefreshSelectionPanel();
}

// 탄약 선택 버튼 

void UAmmoPopupWidget::OnPotatoButtonClicked()
{
    SelectedWeaponData = PotatoWeaponData;
    if (SliderValue) SliderValue->SetValue(0.f);
    RefreshSelectionPanel();
}

void UAmmoPopupWidget::OnCornButtonClicked()
{
    SelectedWeaponData = CornWeaponData;
    if (SliderValue) SliderValue->SetValue(0.f);
    RefreshSelectionPanel();
}

void UAmmoPopupWidget::OnPumpkinButtonClicked()
{
    SelectedWeaponData = PumpkinWeaponData;
    if (SliderValue) SliderValue->SetValue(0.f);
    RefreshSelectionPanel();
}

void UAmmoPopupWidget::OnCarrotButtonClicked()
{
    SelectedWeaponData = CarrotWeaponData;
    if (SliderValue) SliderValue->SetValue(0.f);
    RefreshSelectionPanel();
}

// 버튼 액션 

void UAmmoPopupWidget::OnCloseButtonClicked()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetWorld()->GetFirstPlayerController());
    if (PlayerController)
    {
        SetVisibility(ESlateVisibility::Hidden);
        PlayerController->bShowMouseCursor = false;
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        //RemoveFromParent();
    }
}

void UAmmoPopupWidget::OnChargeButtonClicked()
{
    if (!WeaponComp || !SelectedWeaponData || !ResourceManager || PendingAmmoCount <= 0) return;

    const int32 TotalCost = PendingAmmoCount * SelectedWeaponData->AmmoCraftingCost;
    if (!ResourceManager->RemoveResource(EResourceType::Crop, TotalCost)) return;

    WeaponComp->AddAmmoToWeapon(SelectedWeaponData, PendingAmmoCount);

    if (SliderValue) SliderValue->SetValue(0.f);
    RefreshAmmoDisplay();
    RefreshSelectionPanel();
}

// 내부 갱신

void UAmmoPopupWidget::OnSliderValueChanged(float Value)
{
    if (Slider) Slider->SetPercent(Value);
    int Conval = (int)(Value*1000);
    int Proval = (int)(Value*3000);
    FString Constring = FString(TEXT("감자 %d개"), Value);
    //ConsumeCrop->SetText(FText::FromString(Constring));
    ConsumeCrop->SetText(FText::Format(FText::FromString(TEXT("감자{0}개")), Conval));
    //FString Prostring = FString(TEXT("탄약 %d발"), Value);
    //ProductionAmmo->SetText(FText::FromString(Prostring));
    ProductionAmmo->SetText(FText::Format(FText::FromString(TEXT("탄약{0}발")), Proval));
    RefreshSelectionPanel();
}

void UAmmoPopupWidget::RefreshSelectionPanel()
{
    if (!SelectedWeaponData)
    {
        PendingAmmoCount = 0;
        return;
    }

    // 선택 무기 기준 빈 슬롯 계산
    const FWeaponAmmoState* State = WeaponComp ? WeaponComp->AmmoMap.Find(SelectedWeaponData) : nullptr;
    const int32 Current = State ? (State->CurrentAmmo + State->ReserveAmmo) : 0;
    const int32 MaxCharge = FMath::Max(SelectedWeaponData->MaxAmmoSize - Current, 0);

    const float Ratio = SliderValue ? SliderValue->GetValue() : 0.f;
    PendingAmmoCount = FMath::RoundToInt(Ratio * MaxCharge);

    // 충전량 / 비용
    if (ProductionAmmo) ProductionAmmo->SetText(FText::AsNumber(PendingAmmoCount));
    if (ConsumeCrop)    ConsumeCrop->SetText(FText::AsNumber(PendingAmmoCount * SelectedWeaponData->AmmoCraftingCost));

    // 교환 비용 1회 기준 표시
    if (MinusCropAmount)
    {
        MinusCropAmount->SetText(FText::Format(
            FText::FromString(TEXT("-{0}")),
            FText::AsNumber(SelectedWeaponData->AmmoCraftingCost)));
    }
    if (PlusAmmoAmount) PlusAmmoAmount->SetText(FText::FromString(TEXT("+1")));

    // Description: WeaponData의 WeaponName 사용
    if (Description) Description->SetText(SelectedWeaponData->WeaponName);
}

void UAmmoPopupWidget::RefreshResourceDisplay()
{
    if (!ResourceManager) return;

    if (WoodAmount)      WoodAmount->SetText(FText::AsNumber(ResourceManager->GetWood()));
    if (StoneAmount)     StoneAmount->SetText(FText::AsNumber(ResourceManager->GetStone()));
    if (CropAmount)      CropAmount->SetText(FText::AsNumber(ResourceManager->GetCrop()));
    if (LivestockAmount) LivestockAmount->SetText(FText::AsNumber(ResourceManager->GetLivestock()));
}

void UAmmoPopupWidget::RefreshAmmoDisplay()
{
    if (!WeaponComp) return;

    auto SetAmmoText = [&](UTextBlock* TB, UPotatoWeaponData* Data)
    {
        if (!TB || !Data) return;
        const FWeaponAmmoState* State = WeaponComp->AmmoMap.Find(Data);
        const int32 Current = State ? (State->CurrentAmmo + State->ReserveAmmo) : 0;
        TB->SetText(FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(Current),
            FText::AsNumber(Data->MaxAmmoSize)));
    };

    SetAmmoText(PotatoAmmoAmount,  PotatoWeaponData);
    SetAmmoText(CornAmmoAmount,    CornWeaponData);
    SetAmmoText(PumpkinAmmoAmount, PumpkinWeaponData);
    SetAmmoText(CarrotAmmoAmount,  CarrotWeaponData);
}

void UAmmoPopupWidget::OnResourceChanged(EResourceType Type, int32 NewValue)
{
    //GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Type!")));

    switch (Type)
    {
        case EResourceType::Wood:      
            if (WoodAmount) WoodAmount->SetText(FText::AsNumber(NewValue));   
            //if(Ammo)
            break;
        case EResourceType::Stone:     
            if (StoneAmount) StoneAmount->SetText(FText::AsNumber(NewValue));     
            break;
        case EResourceType::Crop:     
            if (CropAmount) 
                CropAmount->SetText(FText::AsNumber(NewValue));           
            break;
        case EResourceType::Livestock: 
            if (LivestockAmount) 
                LivestockAmount->SetText(FText::AsNumber(NewValue)); 
            break;
    }
}
