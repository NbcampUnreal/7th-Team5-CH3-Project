#pragma once

#include "CoreMinimal.h"
#include "PotatoCrosshairBase.h"
#include "SpreadCrosshairWidget.generated.h"

class UImage;

UCLASS()
class POTATOPROJECT_API USpreadCrosshairWidget : public UPotatoCrosshairBase
{
	GENERATED_BODY()
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
protected:
	// TODO: WBP_SpreadCrosshair에서 바인딩 필요
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairTop;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairBottom;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairLeft;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairRight;
};
