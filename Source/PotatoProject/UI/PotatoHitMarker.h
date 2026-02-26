// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PotatoHitMarker.generated.h"

class UImage;
class UWidgetAnimation;
/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UPotatoHitMarker : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 타격시 호출할 함수    
    UFUNCTION(BlueprintCallable, Category = "HitMarker")
    void PlayHitMarker(bool bIsKill);

protected:
    // 블루프린트 이미지 바인딩

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> HitMarkerNW;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> HitMarkerSE;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> HitMarkerSW;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> HitMarkerNE;

    // 블루프린트의 Show 애니메이션 바인딩
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    TObjectPtr<UWidgetAnimation> ShowAnim;

private:
    FTimerHandle HideHitMarkerTimer;

    // 0.25초 후에 위젯 숨김
    void HideHitMarker();
};
