#include "PotatoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"

APotatoPlayerController::APotatoPlayerController()
	: InputMappingContext(nullptr),
	  MoveAction(nullptr),
	  JumpAction(nullptr),
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
            InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
        }

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