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
	UImage* CrosshairTop;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CrosshairBottom;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CrosshairLeft;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CrosshairRight;
};
