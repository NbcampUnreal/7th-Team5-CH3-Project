#include "PotatoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Building/PotatoPlaceableStructure.h"
#include "UI/PotatoPlayerHUD.h"
#include "UI/AnimalPopup.h"
#include "UI/NPCPopup.h"
#include "UI/AmmoPopupWidget.h"
#include "UI/PauseMenu.h"
#include "Building/PotatoAnimalManagementComp.h"
#include "Building/PotatoNPCManagementComp.h"
#include "PotatoPlayerCharacter.h"

APotatoPlayerController::APotatoPlayerController()
	: InputMappingContext(nullptr),
      JumpAction(nullptr),
      MoveAction(nullptr),
	  LookAction(nullptr),
	  SprintAction(nullptr),
	  CameraZoomAction(nullptr)
{
}

void APotatoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	if (InterGuideClass)
	{
		InterGuideWidget = CreateWidget<UUserWidget>(this, InterGuideClass);
		if (InterGuideWidget)
		{
			InterGuideWidget->AddToViewport(10);
			InterGuideWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (AnimalPopupClass)
	{
		AnimalPopupWidget = CreateWidget<UAnimalPopup>(this, AnimalPopupClass);
		if (AnimalPopupWidget)
		{
			AnimalPopupWidget->AddToViewport(10);
			AnimalPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (NPCPopupClass)
	{
		NPCPopupWidget = CreateWidget<UNPCPopup>(this, NPCPopupClass);
		if (NPCPopupWidget)
		{
			NPCPopupWidget->AddToViewport(10);
			NPCPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (AmmoPopupClass)
	{
		if (APotatoPlayerCharacter* Char = Cast<APotatoPlayerCharacter>(GetPawn()))
		{
			AmmoPopupWidget = CreateWidget<UAmmoPopupWidget>(this, AmmoPopupClass);
			if (AmmoPopupWidget)
			{
				AmmoPopupWidget->InitPopup(Char->GetWeaponComponent());
				AmmoPopupWidget->AddToViewport(10);
				AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void APotatoPlayerController::SetUIMode(bool bEnable, UUserWidget* FocusWidget)
{
    if (bEnable)
    {
        FInputModeUIOnly InputMode;
        if (FocusWidget)
        {
            TSharedPtr<SWidget> SlateWidget = FocusWidget->GetCachedWidget();
            if (SlateWidget.IsValid())
            {
                InputMode.SetWidgetToFocus(SlateWidget);
            }
        }
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
        FlushPressedKeys();
    }
    else
    {
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = false;
    }
}

void APotatoPlayerController::RegisterWarehouseHP(APotatoPlaceableStructure* Warehouse)
{
    WarehouseStructure = Warehouse;
}

void APotatoPlayerController::ShowHUDMessage(const FText& InText, float Duration, bool bPlayAnim)
{
    if (PlayerHUDWidget)
    {
        PlayerHUDWidget->ShowMessageWithDuration(InText, Duration, bPlayAnim);
    }
}

void APotatoPlayerController::ShowAnimalPopup(UPotatoAnimalManagementComp* Comp)
{
	if (AnimalPopupWidget && Comp)
	{
		AnimalPopupWidget->InitPopup(Comp);
		AnimalPopupWidget->SetVisibility(ESlateVisibility::Visible);
		SetUIMode(true, AnimalPopupWidget);
	}
}

void APotatoPlayerController::HideAnimalPopup()
{
	if (AnimalPopupWidget && AnimalPopupWidget->IsVisible())
	{
		AnimalPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		SetUIMode(false);
	}
}

void APotatoPlayerController::ToggleAnimalPopup(UPotatoAnimalManagementComp* Comp)
{
	if (AnimalPopupWidget && AnimalPopupWidget->IsVisible())
	{
		HideAnimalPopup();
	}
	else
	{
		if (AnimalPopupWidget && Comp)
		{
			AnimalPopupWidget->InitPopup(Comp);
			AnimalPopupWidget->SetVisibility(ESlateVisibility::Visible);
			SetUIMode(true, AnimalPopupWidget);
		}
	}
}

void APotatoPlayerController::ShowNPCPopup(UPotatoNPCManagementComp* Comp)
{
	if (NPCPopupWidget && Comp)
	{
		NPCPopupWidget->InitPopup(Comp);
		NPCPopupWidget->SetVisibility(ESlateVisibility::Visible);
		SetUIMode(true, NPCPopupWidget);
	}
}

void APotatoPlayerController::HideNPCPopup()
{
	if (NPCPopupWidget && NPCPopupWidget->IsVisible())
	{
		NPCPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		SetUIMode(false);
	}
}

void APotatoPlayerController::ToggleNPCPopup(UPotatoNPCManagementComp* Comp)
{
	if (NPCPopupWidget && NPCPopupWidget->IsVisible())
	{
		HideNPCPopup();
	}
	else
	{
		if (NPCPopupWidget && Comp)
		{
			NPCPopupWidget->InitPopup(Comp);
			NPCPopupWidget->SetVisibility(ESlateVisibility::Visible);
			SetUIMode(true, NPCPopupWidget);
		}
	}
}

void APotatoPlayerController::ShowInterGuide()
{
	if (InterGuideWidget && !InterGuideWidget->IsVisible())
	{
		InterGuideWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void APotatoPlayerController::HideInterGuide()
{
	if (InterGuideWidget && InterGuideWidget->IsVisible())
	{
		InterGuideWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APotatoPlayerController::CloseAllInteractionPopups()
{
	HideAnimalPopup();
	HideNPCPopup();
	HideInterGuide();
}

void APotatoPlayerController::TogglePauseMenu()
{
    if (!PauseMenuClass) return;

    // 팝업이 열려있으면 모두 닫은 후 일시정지
    CloseAllPopups();

    UPauseMenu* PauseMenu = CreateWidget<UPauseMenu>(this, PauseMenuClass);
    if (PauseMenu)
    {
        SetPause(true);
        PauseMenu->AddToViewport(20);
        SetUIMode(true, PauseMenu);
    }
}

void APotatoPlayerController::ToggleAmmoPopup()
{
	if (!AmmoPopupWidget) return;

	if (AmmoPopupWidget->IsVisible())
	{
		AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		SetUIMode(false);
	}
	else
	{
		AmmoPopupWidget->RefreshAll();
		AmmoPopupWidget->SetVisibility(ESlateVisibility::Visible);
		SetUIMode(true, AmmoPopupWidget);
	}
}

void APotatoPlayerController::CloseAllPopups()
{
	if (AmmoPopupWidget && AmmoPopupWidget->IsVisible())
	{
		AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	CloseAllInteractionPopups();
	SetUIMode(false);
}