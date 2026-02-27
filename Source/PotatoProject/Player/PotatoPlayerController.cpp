#include "PotatoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Building/PotatoPlaceableStructure.h"

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
}

void APotatoPlayerController::SetUIMode(bool bEnable, UUserWidget* FocusWidget)
{
    if (bEnable)
    {
        FInputModeUIOnly InputMode;
        if (FocusWidget)
        {
            // TakeWidget()은 위젯이 완전히 초기화되기 전에 호출 시 Non-Focusable 경고 발생
            TSharedPtr<SWidget> SlateWidget = FocusWidget->GetCachedWidget();
            if (SlateWidget.IsValid())
            {
                InputMode.SetWidgetToFocus(SlateWidget);
            }
        }
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
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
