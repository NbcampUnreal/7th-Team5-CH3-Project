// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PotatoHitMarker.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"

void UPotatoHitMarker::PlayHitMarker(bool bIsKill)
{
    // 색상 설정
    FLinearColor HitColor = bIsKill ? FLinearColor::Red : FLinearColor::White;

    if (HitMarkerNW)         HitMarkerNW->SetColorAndOpacity(HitColor);
    if (HitMarkerSE)         HitMarkerSE->SetColorAndOpacity(HitColor);
    if (HitMarkerSW)         HitMarkerSW->SetColorAndOpacity(HitColor);
    if (HitMarkerNE)         HitMarkerNE->SetColorAndOpacity(HitColor);

    // 위젯을 보이게 설정
    SetVisibility(ESlateVisibility::HitTestInvisible);

    // 애니메이션 재생
    if (ShowAnim)
    {
        PlayAnimation(ShowAnim, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
    }

    GetWorld()->GetTimerManager().ClearTimer(HideHitMarkerTimer);
    GetWorld()->GetTimerManager().SetTimer(HideHitMarkerTimer, this, &UPotatoHitMarker::HideHitMarker, 0.25f, false);
}

void UPotatoHitMarker::HideHitMarker()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
