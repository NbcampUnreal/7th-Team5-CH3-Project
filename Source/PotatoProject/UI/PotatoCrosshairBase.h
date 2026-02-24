#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PotatoCrosshairBase.generated.h"

class APotatoPlayerCharacter;
class UPotatoWeaponComponent;

UCLASS()
class POTATOPROJECT_API UPotatoCrosshairBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	float CalculateCurrentSpread(float DeltaTime);
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float BaseSpread = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float VelocitySpreadMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float FiringSpread = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float JumpingSpread = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float FiringSpreadDuration = 0.15f;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float SpreadInterpSpeed = 15.0f;
	
	UPROPERTY()
	TObjectPtr<APotatoPlayerCharacter> PlayerCharacter;
	
	UPROPERTY()
	TObjectPtr<UPotatoWeaponComponent> WeaponComponent;
	
	float CurrentSpreadValue;
};
