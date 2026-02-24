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
    if (ChargeButton)
    {
        ChargeButton->OnClicked.AddDynamic(this, &UAmmoPopupWidget::OnChargeButtonClicked);
    }

    //SelectedWeaponData = PotatoWeaponData;
    //InitPopup();
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

    //int test = SelectedWeaponData->MaxAmmoSize;
    //GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("PotatoWeaponData: %d!"), test));
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
    if (!WeaponComp)  GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("WeaponComp!")));
    if (!SelectedWeaponData)   GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("SelectedWeaponData!")));
    if (!ResourceManager)   GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("ResourceManager!")));

   
    if (!WeaponComp || !SelectedWeaponData || !ResourceManager ) return;
    if(PendingAmmoCount <= 0)
    {
        ChargeButton->SetToolTipText(FText::FromString(TEXT("교환할 자원이 없습니다.")));
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("charge!1")));
    const int32 TotalCost = PendingAmmoCount * SelectedWeaponData->AmmoCraftingCost;
    if (!ResourceManager->RemoveResource(EResourceType::Crop, TotalCost)) return;

    WeaponComp->AddAmmoToWeapon(SelectedWeaponData, PendingAmmoCount);

    if (SliderValue) SliderValue->SetValue(0.f);
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("charge!2")));
    RefreshAmmoDisplay();
    RefreshSelectionPanel();
}

// 내부 갱신

void UAmmoPopupWidget::OnSliderValueChanged(float Value)
{
    if (Slider) Slider->SetPercent(Value);
    //PendingAmmoCount = (int)(Value *100);
    //int Conval = (int)(Value*100);
    //int Proval = (int)(Value*300);
    //if(SelectedWeaponData->AmmoCraftingCost)
    //ConsumeCrop->SetText(FText::FromString(Constring));
    
    //ConsumeCrop->SetText(FText::Format(FText::FromString(TEXT("감자{0}개")), Conval)); //NowAmmo
    //ProductionAmmo->SetText(FText::Format(FText::FromString(TEXT("탄약{0}발")), Proval));
    //if (ProductionAmmo) ProductionAmmo->SetText(FText::AsNumber(PendingAmmoCount));
    //if (ConsumeCrop)    ConsumeCrop->SetText(FText::AsNumber(PendingAmmoCount * SelectedWeaponData->AmmoCraftingCost));
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
    //const FWeaponAmmoState* State = WeaponComp ? WeaponComp->AmmoMap.Find(SelectedWeaponData) : nullptr;
    //const int32 Current = State ? (State->CurrentAmmo + State->ReserveAmmo) : 0;
    //const int32 MaxCharge = FMath::Max(SelectedWeaponData->MaxAmmoSize - Current, 0);

    int MaxCharge = SelectedWeaponData->MaxAmmoSize;
    //GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("확인?: %d"), test));

    const float Ratio = SliderValue ? SliderValue->GetValue() : 0.f;
    PendingAmmoCount = FMath::RoundToInt(Ratio * MaxCharge);
    //GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("PendingAmmoCount: %.2f, %d"), Ratio, MaxCharge));
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
            if (CropAmount) CropAmount->SetText(FText::AsNumber(NewValue));   
            break;
        case EResourceType::Livestock: 
            if (LivestockAmount) LivestockAmount->SetText(FText::AsNumber(NewValue)); 
            break;
    }
}

void UAmmoPopupWidget::ChangeAmmo(int index)
{
    if (AmmoImage) AmmoImage->SetBrushFromTexture(AmmoTextures[index]);
    RefreshSelectionPanel();
    switch (index)
    {
    case 1:
        SelectedWeaponData = PotatoWeaponData;
        //if (MinusCropAmount) MinusCropAmount->SetText(FText::Format(FText::FromString(TEXT("-1"))));
        //if (PlusAmmoAmount) PlusAmmoAmount->SetText(FText::Format(FText::FromString(TEXT("+1"))));
        break;
    case 2:
        SelectedWeaponData = CornWeaponData;
        //if (MinusCropAmount) MinusCropAmount->SetText(FText::Format(FText::FromString(TEXT("-1"))));
        //if (PlusAmmoAmount) PlusAmmoAmount->SetText(FText::Format(FText::FromString(TEXT("+1"))));
        break;
    case 3:
        SelectedWeaponData = PumpkinWeaponData;
        //if (MinusCropAmount) MinusCropAmount->SetText(FText::Format(FText::FromString(TEXT("-1"))));
        //if (PlusAmmoAmount) PlusAmmoAmount->SetText(FText::Format(FText::FromString(TEXT("+3"))));
        break;
    case 4:
        SelectedWeaponData = CarrotWeaponData;
        //if (MinusCropAmount) MinusCropAmount->SetText(FText::Format(FText::FromString(TEXT("-1"))));
        //if (PlusAmmoAmount) PlusAmmoAmount->SetText(FText::Format(FText::FromString(TEXT("+1"))));
        break;
    }
    
}