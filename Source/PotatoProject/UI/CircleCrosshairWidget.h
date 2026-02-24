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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// TODO: WBP_CircleCrosshair에서 바인딩 필요
	UPROPERTY(meta = (BindWidget))
	UImage* CircleImage;
	
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float SpreadScaling = 15.0f;
};
