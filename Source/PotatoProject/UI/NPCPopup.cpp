// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NPCPopup.h"
#include "UI/NPCListItem.h"
#include "Building/PotatoNPCManagementComp.h"
#include "NPC/PotatoNPC.h"
#include "Core/PotatoProductionComponent.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"

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
    if (ScrollBox_79)
    {
        for (int32 i = 0; i < ScrollBox_79->GetChildrenCount(); ++i)
        {
            if (UNPCListItem* Item = Cast<UNPCListItem>(ScrollBox_79->GetChildAt(i)))
            {
                Item->OnItemSelected.AddDynamic(this, &UNPCPopup::OnNPCItemSelected);
            }
        }
    }
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

    // 건물에 등록된 NPCClassMap의 첫 번째 타입을 자동 결정
    // LumberMill -> Lumberjack, Mine -> Miner 등 건물 BP에서 설정한 값 사용
    if (ManagementComp && ManagementComp->NPCClassMap.Num() > 0)
    {
        TArray<ENPCType> Keys;
        ManagementComp->NPCClassMap.GetKeys(Keys);
        CachedNPCType = Keys[0];
    }

    RefreshNPCList();
    RefreshHireCostPanel();
    RefreshTotalCost();
    RefreshEmployeeAmount();
}

// ============================================================
// Button Callbacks
// ============================================================

void UNPCPopup::OnHireButtonClicked()
{
    if (!ManagementComp) return;

    ManagementComp->HireNPC(CachedNPCType);

    RefreshNPCList();
    RefreshHireCostPanel();
    RefreshTotalCost();
    RefreshEmployeeAmount();
}

void UNPCPopup::OnCloseButtonClicked()
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

    for (int32 i = 0; i < ScrollBox_79->GetChildrenCount(); ++i)
    {
        UNPCListItem* Item = Cast<UNPCListItem>(ScrollBox_79->GetChildAt(i));
        if (!Item) continue;

        APotatoNPC* NPC = NPCs.IsValidIndex(i) ? NPCs[i].Get() : nullptr;
        Item->SetNPCData(NPC);
        Item->SetSelected(false);
    }

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
