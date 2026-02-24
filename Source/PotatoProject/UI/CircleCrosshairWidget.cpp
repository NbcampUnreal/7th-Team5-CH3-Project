// Fill out your copyright notice in the Description page of Project Settings.


#include "CircleCrosshairWidget.h"

#include "Components/Image.h"

void UCircleCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	float Spread = CalculateCurrentSpread(InDeltaTime);
	float Scale = Spread / SpreadScaling;
	
	if (CircleImage)
	{
		CircleImage->SetRenderScale(FVector2D(Scale, Scale));
	}
}
