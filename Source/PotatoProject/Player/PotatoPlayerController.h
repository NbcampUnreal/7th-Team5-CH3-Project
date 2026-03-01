#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PotatoPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APotatoPlaceableStructure;
class UPotatoPlayerHUD;
class UAnimalPopup;
class UNPCPopup;
class UAmmoPopupWidget;
class UPotatoAnimalManagementComp;
class UPotatoNPCManagementComp;

UCLASS()
class POTATOPROJECT_API APotatoPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	// Input	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* SprintAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* CameraZoomAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* AttackAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* ReloadAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* WeaponChangeAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ToggleBuildAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* AmmoAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* BarnAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction* PauseAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* NextDialogueAction;
	
	// HUD
public:
	UPROPERTY(BlueprintReadWrite, Category = "HUD|Setup")
	TObjectPtr<APotatoPlaceableStructure> WarehouseStructure;

	UPROPERTY(BlueprintReadWrite, Category = "HUD")
	TObjectPtr<UPotatoPlayerHUD> PlayerHUDWidget = nullptr;

	// 상호작용 UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Interaction")
	TSubclassOf<UUserWidget> InterGuideClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Interaction")
	TSubclassOf<UAnimalPopup> AnimalPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Interaction")
	TSubclassOf<UNPCPopup> NPCPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Ammo")
	TSubclassOf<UAmmoPopupWidget> AmmoPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Pause")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InterGuideWidget;

	UPROPERTY()
	TObjectPtr<UAnimalPopup> AnimalPopupWidget;

	UPROPERTY()
	TObjectPtr<UNPCPopup> NPCPopupWidget;

	UPROPERTY()
	TObjectPtr<UAmmoPopupWidget> AmmoPopupWidget;

	// functions
public:
	APotatoPlayerController();

	virtual void BeginPlay() override;
    void SetUIMode(bool bEnable, UUserWidget* FocusWidget = nullptr);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void RegisterWarehouseHP(APotatoPlaceableStructure* Warehouse);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowHUDMessage(const FText& InText, float Duration, bool bPlayAnim);

	// 상호작용 UI 함수
	void ShowAnimalPopup(UPotatoAnimalManagementComp* Comp);
	void HideAnimalPopup();
	void ToggleAnimalPopup(UPotatoAnimalManagementComp* Comp);

	void ShowNPCPopup(UPotatoNPCManagementComp* Comp);
	void HideNPCPopup();
	void ToggleNPCPopup(UPotatoNPCManagementComp* Comp);

	void ShowInterGuide();
	void HideInterGuide();
	void CloseAllInteractionPopups();

	void TogglePauseMenu();
	void ToggleAmmoPopup();
	void CloseAllPopups();
};
