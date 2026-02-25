// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AnimalPopup.h"
#include "UI/AnimalListItem.h"
#include "Building/PotatoAnimalManagementComp.h"
#include "Animal/PotatoAnimal.h"
#include "Core/PotatoProductionComponent.h"
#include "Core/PotatoResourceManager.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"

void UAnimalPopup::NativeConstruct()
{
    Super::NativeConstruct();

    ResourceManager = GetWorld()->GetSubsystem<UPotatoResourceManager>();

    if (CowBox)     CowBox->OnCheckStateChanged.AddDynamic(this, &UAnimalPopup::OnCowBoxChanged);
    if (PigBox)     PigBox->OnCheckStateChanged.AddDynamic(this, &UAnimalPopup::OnPigBoxChanged);
    if (ChickenBox) ChickenBox->OnCheckStateChanged.AddDynamic(this, &UAnimalPopup::OnChickenBoxChanged);

    if (BuyButton)   BuyButton->OnClicked.AddDynamic(this, &UAnimalPopup::OnBuyButtonClicked);
    if (SlaughterButton) SlaughterButton->OnClicked.AddDynamic(this, &UAnimalPopup::OnSlaughterButtonClicked);
    if (CloseButton)           CloseButton->OnClicked.AddDynamic(this, &UAnimalPopup::OnCloseButtonClicked);

    
}

void UAnimalPopup::NativeDestruct()
{
    if (CowBox)     CowBox->OnCheckStateChanged.RemoveDynamic(this, &UAnimalPopup::OnCowBoxChanged);
    if (PigBox)     PigBox->OnCheckStateChanged.RemoveDynamic(this, &UAnimalPopup::OnPigBoxChanged);
    if (ChickenBox) ChickenBox->OnCheckStateChanged.RemoveDynamic(this, &UAnimalPopup::OnChickenBoxChanged);

    if (BuyButton)   BuyButton->OnClicked.RemoveDynamic(this, &UAnimalPopup::OnBuyButtonClicked);
    if (SlaughterButton) SlaughterButton->OnClicked.RemoveDynamic(this, &UAnimalPopup::OnSlaughterButtonClicked);
    if (CloseButton)           CloseButton->OnClicked.RemoveDynamic(this, &UAnimalPopup::OnCloseButtonClicked);

    Super::NativeDestruct();
}

void UAnimalPopup::InitPopup(UPotatoAnimalManagementComp* InManagementComp)
{
    

    // ScrollBox 안의 AnimalListItem 자식들에 델리게이트 바인딩
    /*if (AnimalList)
    {
        AnimalList->ClearChildren();
        for (int32 i = 0; i < AnimalList->GetChildrenCount(); ++i)
        {
            if (UAnimalListItem* Item = Cast<UAnimalListItem>(AnimalList->GetChildAt(i)))
            {
                Item->OnItemSelected.AddDynamic(this, &UAnimalPopup::OnAnimalItemSelected);
            }
        }
    }*/
    ManagementComp = InManagementComp;
    SelectedListItem = nullptr;

    // 초기 선택: 소
    SelectAnimalType(EAnimalType::Cow);

    RefreshAnimalList();
    RefreshTotalProduction();
    RefreshCattleAmount();
}

// CheckBox (Radio 방식)

void UAnimalPopup::OnCowBoxChanged(bool bIsChecked)
{
    if (bIsChecked) SelectAnimalType(EAnimalType::Cow);
    else if (CowBox) CowBox->SetIsChecked(true); // 강제 유지 (다른 쪽이 선택돼야만 해제)
}

void UAnimalPopup::OnPigBoxChanged(bool bIsChecked)
{
    if (bIsChecked) SelectAnimalType(EAnimalType::Pig);
    else if (PigBox) PigBox->SetIsChecked(true);
}

void UAnimalPopup::OnChickenBoxChanged(bool bIsChecked)
{
    if (bIsChecked) SelectAnimalType(EAnimalType::Chicken);
    else if (ChickenBox) ChickenBox->SetIsChecked(true);
}

void UAnimalPopup::SelectAnimalType(EAnimalType NewType)
{
    SelectedAnimalType = NewType;

    // Radio 방식: 선택된 것만 Checked
    if (CowBox)     CowBox->SetIsChecked(NewType == EAnimalType::Cow);
    if (PigBox)     PigBox->SetIsChecked(NewType == EAnimalType::Pig);
    if (ChickenBox) ChickenBox->SetIsChecked(NewType == EAnimalType::Chicken);

    RefreshBuyCostPanel();
}

// 구매 

void UAnimalPopup::OnBuyButtonClicked()
{
    if (!ManagementComp) return;

    ManagementComp->SpawnAnimal(SelectedAnimalType);

    RefreshAnimalList();
    RefreshTotalProduction();
    RefreshCattleAmount();
    RefreshBuyCostPanel(); // 자원 변동 반영
}

// 도축

void UAnimalPopup::OnSlaughterButtonClicked()
{
    if (!ManagementComp || !SelectedListItem || !SelectedListItem->HasAnimal()) return;

    ManagementComp->RemoveAnimal(SelectedListItem->GetAnimal());
    SelectedListItem = nullptr;

    RefreshAnimalList();
    RefreshTotalProduction();
    RefreshCattleAmount();
}

// 닫기

void UAnimalPopup::OnCloseButtonClicked()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetWorld()->GetFirstPlayerController());
    if (PlayerController)
    {
        SetVisibility(ESlateVisibility::Hidden);
        PlayerController->bShowMouseCursor = false;
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
    }
    //RemoveFromParent();
}

// 리스트 아이템 선택

void UAnimalPopup::OnAnimalItemSelected(UAnimalListItem* ClickedItem)
{
    // 이전 선택 해제
    if (SelectedListItem && SelectedListItem != ClickedItem)
    {
        SelectedListItem->SetSelected(false);
    }

    // 같은 아이템 재클릭 시 선택 해제 토글
    if (SelectedListItem == ClickedItem)
    {
        SelectedListItem->SetSelected(false);
        SelectedListItem = nullptr;
    }
    else
    {
        SelectedListItem = ClickedItem;
        SelectedListItem->SetSelected(true);
    }
}

// 내부 갱신

void UAnimalPopup::RefreshAnimalList()
{
    if (!AnimalList || !ManagementComp) return;

    const TArray<TObjectPtr<APotatoAnimal>>& Animals = ManagementComp->AssignedAnimals;
    if (AnimalListItemClass)
    {
        AnimalList->ClearChildren();
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("AnimalListItemClass"));
        int Animalnum = ManagementComp->AssignedAnimals.Num();
        for (int i = 0; i < Animalnum; i++)
        {
            //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Animalnum: %d"), Animalnum));
            UAnimalListItem* NewItem = CreateWidget<UAnimalListItem>(GetWorld(), AnimalListItemClass);
            AnimalList->AddChild(NewItem);
            APotatoAnimal* animal = ManagementComp->AssignedAnimals[i].Get();
            if (animal)
            {
                NewItem->SetAnimalData(animal);
                NewItem->OnItemSelected.AddDynamic(this, &UAnimalPopup::OnAnimalItemSelected);
                NewItem->SetSelected(false);
            }
            
        }
    }

 

    SelectedListItem = nullptr;
}

void UAnimalPopup::RefreshBuyCostPanel()
{
    if (!ManagementComp) return;

    // 선택된 동물 종류의 DefaultObject에서 구매 비용 읽기
    TSubclassOf<APotatoAnimal>* FoundClass = ManagementComp->AnimalClassMap.Find(SelectedAnimalType);
    if (!FoundClass || !*FoundClass) return;

    APotatoAnimal* Default = (*FoundClass)->GetDefaultObject<APotatoAnimal>();
    if (!Default || !Default->ProductionComp) return;

    UPotatoProductionComponent* Prod = Default->ProductionComp;

    // 현재 WBP에는 Crop 아이콘만 있으므로 Crop 비용만 표시.
    // 추후 Wood/Stone/Livestock 비용 위젯 추가 시 아래에 동일 패턴으로 확장
    if (EmployeeCharge)
    {
        EmployeeCharge->SetText(FText::AsNumber(Prod->GetBuyCostCrop()));
    }

    // 생산량 증가분 - 0이면 아이콘+텍스트 숨김
    auto SetPair = [](USizeBox* Box, UTextBlock* Text, int32 Val)
    {
        const ESlateVisibility Vis = (Val > 0)
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed;
        if (Box)  Box->SetVisibility(Vis);
        if (Text) { Text->SetVisibility(Vis); if (Val > 0) Text->SetText(FText::AsNumber(Val)); }
    };

    SetPair(WoodBox_1,      AddWood,      Prod->GetProductionPerMinuteWood());
    SetPair(StoneBox_1,     AddStone,     Prod->GetProductionPerMinuteStone());
    SetPair(CropBox_1,      AddCrop,      Prod->GetProductionPerMinuteCrop());
    SetPair(LivestockBox_1, AddLiveStock, Prod->GetProductionPerMinuteLivestock());
}

void UAnimalPopup::RefreshTotalProduction()
{
    if (!ManagementComp) return;

    int32 TotalWood = 0, TotalStone = 0, TotalCrop = 0, TotalLivestock = 0;

    for (APotatoAnimal* Animal : ManagementComp->AssignedAnimals)
    {
        if (!Animal || !Animal->ProductionComp) continue;
        UPotatoProductionComponent* Prod = Animal->ProductionComp;
        TotalWood      += Prod->GetProductionPerMinuteWood();
        TotalStone     += Prod->GetProductionPerMinuteStone();
        TotalCrop      += Prod->GetProductionPerMinuteCrop();
        TotalLivestock += Prod->GetProductionPerMinuteLivestock();
    }

    // 0이면 아이콘+텍스트 숨김
    auto SetPair = [](USizeBox* Box, UTextBlock* Text, int32 Val)
    {
        const ESlateVisibility Vis = (Val > 0)
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed;
        if (Box)  Box->SetVisibility(Vis);
        if (Text) { Text->SetVisibility(Vis); if (Val > 0) Text->SetText(FText::AsNumber(Val)); }
    };

    SetPair(WoodBox,      TotalWoodAmount,      TotalWood);
    SetPair(StoneBox,     TotalStoneAmount,      TotalStone);
    SetPair(CropBox,      TotalCropAmount,       TotalCrop);
    SetPair(LivestockBox, TotalLivestockAmount,  TotalLivestock);
}

void UAnimalPopup::RefreshCattleAmount()
{
    if (!ManagementComp || !CattleAmount) return;

    const int32 Count = ManagementComp->AssignedAnimals.Num();
    CattleAmount->SetText(FText::Format(
        FText::FromString(TEXT("보유: {0}마리")), FText::AsNumber(Count)));
}
