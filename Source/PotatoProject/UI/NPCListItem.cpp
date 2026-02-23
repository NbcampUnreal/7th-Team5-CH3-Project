// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NPCListItem.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/CheckBox.h"
#include "NPC/PotatoNPC.h"
#include "Core/PotatoProductionComponent.h"

void UNPCListItem::NativeConstruct()
{
    Super::NativeConstruct();

    if (CheckBox_0)
    {
        CheckBox_0->OnCheckStateChanged.AddDynamic(this, &UNPCListItem::OnCheckBoxStateChanged);
    }

    ShowEmptyState();
}

void UNPCListItem::OnCheckBoxStateChanged(bool bIsChecked)
{
    if (bIsChecked && CachedNPC)
    {
        OnItemSelected.Broadcast(this);
    }
    else
    {
        if (CheckBox_0) CheckBox_0->SetIsChecked(false);
    }
}

void UNPCListItem::SetNPCData(APotatoNPC* InNPC)
{
    CachedNPC = InNPC;

    if (CheckBox_0) CheckBox_0->SetIsChecked(false);

    if (InNPC)
    {
        ShowNPCData(InNPC);
    }
    else
    {
        ShowEmptyState();
    }
}

void UNPCListItem::SetSelected(bool bInSelected)
{
    if (CheckBox_0) CheckBox_0->SetIsChecked(bInSelected);
}

void UNPCListItem::ShowNPCData(APotatoNPC* InNPC)
{
    if (NPCDataBox) NPCDataBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (EmptyBox)   EmptyBox->SetVisibility(ESlateVisibility::Collapsed);

    // NPC 직종별 이름 + 아이콘
    FText NPCName;
    UTexture2D* Icon = nullptr;
    switch (InNPC->Type)
    {
    case ENPCType::Lumberjack:
        NPCName = FText::FromString(TEXT("[벌목꾼] 유지비(매일 밤):"));
        Icon = LumberjackIcon;
        break;
    case ENPCType::Miner:
        NPCName = FText::FromString(TEXT("[광부] 유지비(매일 밤):"));
        Icon = MinerIcon;
        break;
    }

    if (Description_7) Description_7->SetText(NPCName);
    if (Image_99 && Icon) Image_99->SetBrushFromTexture(Icon);

    // 유지비 (Livestock 고정)
    if (ConsumeAmount)
    {
        ConsumeAmount->SetText(FText::Format(
            FText::FromString(TEXT("-{0}")),
            FText::AsNumber(InNPC->MaintenanceCostLivestock)));
    }

    // 생산량 — 0이면 아이콘+텍스트 숨김
    UPotatoProductionComponent* Prod = InNPC->ProductionComp;
    if (!Prod) return;

    SetResourceVisible(WoodBox,      WoodAmount,      Prod->GetProductionPerMinuteWood());
    SetResourceVisible(StoneBox,     StoneAmount,     Prod->GetProductionPerMinuteStone());
    SetResourceVisible(CropBox,      CropAmount,      Prod->GetProductionPerMinuteCrop());
    SetResourceVisible(LivestockBox, LivestockAmount, Prod->GetProductionPerMinuteLivestock());
}

void UNPCListItem::ShowEmptyState()
{
    if (NPCDataBox) NPCDataBox->SetVisibility(ESlateVisibility::Collapsed);
    if (EmptyBox)   EmptyBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UNPCListItem::SetResourceVisible(USizeBox* IconBox, UTextBlock* AmountText, int32 Value)
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
