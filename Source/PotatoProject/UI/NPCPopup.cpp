// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NPCPopup.h"
#include "UI/NPCListItem.h"
#include "Building/PotatoNPCManagementComp.h"
#include "NPC/PotatoNPC.h"
#include "Core/PotatoProductionComponent.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/Image.h"

// ============================================================
// Lifecycle
// ============================================================

void UNPCPopup::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_GoToMainMenu)
    {
        Button_GoToMainMenu->OnClicked.AddDynamic(this, &UNPCPopup::OnHireButtonClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UNPCPopup::OnCloseButtonClicked);
    }

    // ScrollBox 내 NPCListItem 자식들에 선택 델리게이트 바인딩
    /*if (ScrollBox_79)
    {
        for (int32 i = 0; i < ScrollBox_79->GetChildrenCount(); ++i)
        {
            if (UNPCListItem* Item = Cast<UNPCListItem>(ScrollBox_79->GetChildAt(i)))
            {
                Item->OnItemSelected.AddDynamic(this, &UNPCPopup::OnNPCItemSelected);
            }
        }
    }*/
}

void UNPCPopup::NativeDestruct()
{
    if (Button_GoToMainMenu)
    {
        Button_GoToMainMenu->OnClicked.RemoveDynamic(this, &UNPCPopup::OnHireButtonClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UNPCPopup::OnCloseButtonClicked);
    }

    Super::NativeDestruct();
}

// ============================================================
// Public API
// ============================================================

void UNPCPopup::InitPopup(UPotatoNPCManagementComp* InManagementComp)
{
    ManagementComp = InManagementComp;
    SelectedListItem = nullptr;

    if (ManagementComp && ManagementComp->NPCClassMap.Num() > 0)
    {
        TArray<ENPCType> Keys;
        ManagementComp->NPCClassMap.GetKeys(Keys);
        CachedNPCType = Keys[0];
        if (CachedNPCType == ENPCType::Miner && MinerIcon && RockIcon) {
            Image_Portrait->SetBrushFromTexture(MinerIcon);
            Image_Product->SetBrushFromTexture(RockIcon);
        }
        if (CachedNPCType == ENPCType::Lumberjack && LumberjackIcon && WoodIcon)
        {
            Image_Portrait->SetBrushFromTexture(LumberjackIcon);
            Image_Product->SetBrushFromTexture(WoodIcon);
        }
    }

    RefreshNPCList();
    RefreshHireCostPanel();
    RefreshTotalCost();
    RefreshEmployeeAmount();
    ShowDefaultWarning();
}

// ============================================================
// Button Callbacks
// ============================================================

void UNPCPopup::OnHireButtonClicked()
{
    if (!ManagementComp) return;

    if (!ManagementComp->HireNPC(CachedNPCType))
    {
        ShowWarning(NSLOCTEXT("NPCPopup", "NotEnoughRes", "자원이 부족합니다."));
        return;
    }

    RefreshNPCList();
    RefreshHireCostPanel();
    RefreshTotalCost();
    RefreshEmployeeAmount();
    ShowDefaultWarning();
}

void UNPCPopup::OnCloseButtonClicked()
{
    SetVisibility(ESlateVisibility::Hidden);
    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }
    //RemoveFromParent();
}

// ============================================================
// List Item Selection
// ============================================================

void UNPCPopup::OnNPCItemSelected(UNPCListItem* ClickedItem)
{
    // 이전 선택 해제
    if (SelectedListItem && SelectedListItem != ClickedItem)
    {
        SelectedListItem->SetSelected(false);
    }

    // 같은 아이템을 다시 클릭하면 선택 해제 (토글)
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

    // 선택된 NPC 정보로 패널 갱신
    RefreshHireCostPanel();
}

// ============================================================
// Internal Refresh
// ============================================================

void UNPCPopup::RefreshNPCList()
{
    if (!ScrollBox_79 || !ManagementComp) return;

    const TArray<TObjectPtr<APotatoNPC>>& NPCs = ManagementComp->AssignedNPCs;
    if (NPCListItemClass)
    {
        //바인딩 먼저 해제
        TArray<UWidget*> AllChildren = ScrollBox_79->GetAllChildren();
        for (UWidget* Child : AllChildren)
        {
            if (UNPCListItem* Item = Cast<UNPCListItem>(Child))
            {
                Item->OnItemSelected.RemoveDynamic(this, &UNPCPopup::OnNPCItemSelected);
            }
        }
        ScrollBox_79->ClearChildren();
        int NPCnum = ManagementComp->AssignedNPCs.Num();
        for (int i = 0; i < NPCnum; i++)
        {
            UNPCListItem* NewItem = CreateWidget<UNPCListItem>(GetWorld(), NPCListItemClass);
            ScrollBox_79->AddChild(NewItem);
            APotatoNPC* NPC = ManagementComp->AssignedNPCs[i].Get();
            if (NPC)
            {
                NewItem->SetNPCData(NPC);
                NewItem->OnItemSelected.AddDynamic(this, &UNPCPopup::OnNPCItemSelected);
                NewItem->SetSelected(false);
            }
        }
    }

    /*for (int32 i = 0; i < ScrollBox_79->GetChildrenCount(); ++i)
    {
        UNPCListItem* Item = Cast<UNPCListItem>(ScrollBox_79->GetChildAt(i));
        if (!Item) continue;

        APotatoNPC* NPC = NPCs.IsValidIndex(i) ? NPCs[i].Get() : nullptr;
        Item->SetNPCData(NPC);
        Item->SetSelected(false);
    }*/

    SelectedListItem = nullptr;
}

void UNPCPopup::RefreshHireCostPanel()
{
    if (!ManagementComp) return;

    // 선택된 NPC가 있으면 해당 NPC 정보, 없으면 건물 종류에 맞는 DefaultObject
    APotatoNPC* SourceNPC = nullptr;

    if (SelectedListItem && SelectedListItem->HasNPC())
    {
        SourceNPC = SelectedListItem->GetNPC();
    }
    else
    {
        TSubclassOf<APotatoNPC>* FoundClass = ManagementComp->NPCClassMap.Find(CachedNPCType);
        if (FoundClass && *FoundClass)
        {
            SourceNPC = (*FoundClass)->GetDefaultObject<APotatoNPC>();
        }
    }

    if (!SourceNPC || !SourceNPC->ProductionComp) return;

    UPotatoProductionComponent* Prod = SourceNPC->ProductionComp;

    // 고용 비용
    if (EmployeeCharge)
    {
        EmployeeCharge->SetText(FText::AsNumber(Prod->GetBuyCostCrop()));
    }

    // 생산량 증가분 — 4개 자원 중 가장 높은 값 표시
    if (ProductionAdd)
    {
        const int32 MainProduction = FMath::Max(
            FMath::Max(Prod->GetProductionPerMinuteWood(), Prod->GetProductionPerMinuteStone()),
            FMath::Max(Prod->GetProductionPerMinuteCrop(), Prod->GetProductionPerMinuteLivestock())
        );
        ProductionAdd->SetText(FText::AsNumber(MainProduction));
    }

    // 유지비 (Livestock 고정)
    if (ConsumeCost)
    {
        ConsumeCost->SetText(FText::Format(
            FText::FromString(TEXT("-{0}")),
            FText::AsNumber(SourceNPC->MaintenanceCostLivestock)));
    }
}

void UNPCPopup::RefreshTotalCost()
{
    if (!ManagementComp) return;

    int32 Total = 0;
    for (APotatoNPC* NPC : ManagementComp->AssignedNPCs)
    {
        if (NPC) Total += NPC->MaintenanceCostLivestock;
    }

    if (TotalCost)
    {
        TotalCost->SetText(FText::AsNumber(Total));
    }
}

void UNPCPopup::RefreshEmployeeAmount()
{
    if (!ManagementComp || !EmployeeAmount) return;

    const int32 Count = ManagementComp->AssignedNPCs.Num();
    EmployeeAmount->SetText(FText::Format(
        FText::FromString(TEXT("고용 현황: {0}명")), FText::AsNumber(Count)));
}

void UNPCPopup::ShowWarning(const FText& Message)
{
    if (WarningMessage)
    {
        WarningMessage->SetText(Message);
        WarningMessage->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WarningResetTimer);
        World->GetTimerManager().SetTimer(WarningResetTimer, [this]()
        {
            ShowDefaultWarning();
        }, 2.0f, false);
    }
}

void UNPCPopup::ShowDefaultWarning()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WarningResetTimer);
    }
    if (WarningMessage)
    {
        WarningMessage->SetText(NSLOCTEXT("NPCPopup", "DefaultWarning", "주의 - 유지비 미지급 시 NPC가 떠납니다."));
        WarningMessage->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
