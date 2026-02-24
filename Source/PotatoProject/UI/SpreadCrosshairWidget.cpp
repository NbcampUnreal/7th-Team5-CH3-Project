// Fill out your copyright notice in the Description page of Project Settings.


#include "SpreadCrosshairWidget.h"

#include "Components/Image.h"

void USpreadCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	float Spread = CalculateCurrentSpread(InDeltaTime);
	
	if (CrosshairTop)
	{
		CrosshairTop->SetRenderTranslation(FVector2D(0.0f, -Spread));
	}
	if (CrosshairBottom)
	{
		CrosshairBottom->SetRenderTranslation(FVector2D(0.0f, Spread));
	}
	if (CrosshairLeft)
	{
		CrosshairLeft->SetRenderTranslation(FVector2D(-Spread, 0.0f));
	}
	if (CrosshairRight)
	{
		CrosshairRight->SetRenderTranslation(FVector2D(Spread, 0.0f));
	}
}
