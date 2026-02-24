#pragma once

#include "CoreMinimal.h"
#include "PotatoCrosshairBase.h"
#include "CircleCrosshairWidget.generated.h"

class UImage;

UCLASS()
class POTATOPROJECT_API UCircleCrosshairWidget : public UPotatoCrosshairBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// TODO: WBP_CircleCrosshair에서 바인딩 필요
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CircleImage;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CircleMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float RadiusScaleFactor = 0.005f;
};
