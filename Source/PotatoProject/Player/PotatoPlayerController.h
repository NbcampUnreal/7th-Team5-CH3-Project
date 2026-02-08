#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PotatoPlayerController.generated.h"

class APotatoPlayerCharacter;
class PotatoBuildingSystem;
class PotatoWeaponSystem;

UCLASS()
class POTATOPROJECT_API APotatoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	APotatoPlayerCharacter* ControlledCharacter;
	PotatoBuildingSystem* BuildingSystem;
	PotatoWeaponSystem* WeaponSystem;

	void HandleMovementInput();
	void HandleCombatInput();
	void HandleBuildingInput();
	void HandleInteractionInput();
};
