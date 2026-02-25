// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AnimalListItem.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/CheckBox.h"
#include "Animal/PotatoAnimal.h"
#include "Core/PotatoProductionComponent.h"

void UAnimalListItem::NativeConstruct()
{
    Super::NativeConstruct();

    if (CheckBox_106)
    {
        CheckBox_106->OnCheckStateChanged.AddDynamic(this, &UAnimalListItem::OnCheckBoxStateChanged);
    }

    ShowEmptyState();
}

void UAnimalListItem::OnCheckBoxStateChanged(bool bIsChecked)
{
    if (bIsChecked && CachedAnimal)
    {
        OnItemSelected.Broadcast(this);
    }
    else
    {
        // 빈 슬롯이거나 강제 해제 — Unchecked 유지
        if (CheckBox_106) CheckBox_106->SetIsChecked(false);
    }
}

void UAnimalListItem::SetAnimalData(APotatoAnimal* InAnimal)
{
    CachedAnimal = InAnimal;

    if (CheckBox_106) CheckBox_106->SetIsChecked(false);

    if (InAnimal)
    {
        ShowAnimalData(InAnimal);
    }
    else
    {
        ShowEmptyState();
    }
}

void UAnimalListItem::SetSelected(bool bInSelected)
{
    if (CheckBox_106) CheckBox_106->SetIsChecked(bInSelected);
}

void UAnimalListItem::ShowAnimalData(APotatoAnimal* InAnimal)
{
    if (AnimalDataBox) AnimalDataBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (EmptyBox)      EmptyBox->SetVisibility(ESlateVisibility::Collapsed);

    UTexture2D* Icon = nullptr;
    FText AnimalName;
    switch (InAnimal->AnimalType)
    {
    case EAnimalType::Cow:
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("소")));
        AnimalName = FText::FromString(TEXT("[소]"));
        Icon = CowIcon;
        break;
    case EAnimalType::Pig:
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("돼지")));
        AnimalName = FText::FromString(TEXT("[돼지]"));
        Icon = PigIcon;
        break;
    case EAnimalType::Chicken:
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("닭")));
        AnimalName = FText::FromString(TEXT("[닭]"));
        Icon = ChickenIcon;
        break;
    }

    if (AnimalNameText) AnimalNameText->SetText(AnimalName);
    if (AnimalImage && Icon) AnimalImage->SetBrushFromTexture(Icon);

    UPotatoProductionComponent* Prod = InAnimal->ProductionComp;
    if (!Prod) return;

    SetResourceVisible(WoodBox,      WoodAmount,      Prod->GetProductionPerMinuteWood());
    SetResourceVisible(StoneBox,     StoneAmount,     Prod->GetProductionPerMinuteStone());
    SetResourceVisible(CropBox,      CropAmount,      Prod->GetProductionPerMinuteCrop());
    SetResourceVisible(LivestockBox, LivestockAmount, Prod->GetProductionPerMinuteLivestock());
}

void UAnimalListItem::ShowEmptyState()
{
    if (AnimalDataBox) AnimalDataBox->SetVisibility(ESlateVisibility::Collapsed);
    if (EmptyBox)      EmptyBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UAnimalListItem::SetResourceVisible(USizeBox* IconBox, UTextBlock* AmountText, int32 Value)
{
    const ESlateVisibility Vis = (Value > 0)
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Collapsed;

    if (IconBox)    IconBox->SetVisibility(Vis);
    if (AmountText)
    {
        AmountText->SetVisibility(Vis);
        if (Value > 0) AmountText->SetText(FText::AsNumber(Value));
    }
}
