// Fill out your copyright notice in the Description page of Project Settings.


#include "CircleCrosshairWidget.h"

#include "Components/Image.h"
#include "DSP/MidiNoteQuantizer.h"
#include "DynamicMesh/MeshTransforms.h"
#include "Widgets/Notifications/SProgressBar.h"

void UCircleCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (CircleImage)
	{
		CircleMaterial = CircleImage->GetDynamicMaterial();
	}
}

void UCircleCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	float CurrentSpread = CalculateCurrentSpread(InDeltaTime);
	float TargetRadius = CurrentSpread * RadiusScaleFactor;
	
	if (CircleMaterial)
	{
		CircleMaterial->SetScalarParameterValue(FName("Radius"), TargetRadius);
	}
}
