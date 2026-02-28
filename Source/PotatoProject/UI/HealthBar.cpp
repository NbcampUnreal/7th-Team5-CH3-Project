// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HealthBar.h"
#include "Components/ProgressBar.h"


void UHealthBar::SetHealthRatio(float Ratio)
{
	if (!HealthBar) return;
	if (!IsValid(HealthBar)) return;

	HealthBar->SetPercent(FMath::Clamp(Ratio, 0.f, 1.f));
}