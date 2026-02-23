// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PotatoDamageTextWidget.h"
#include "Components/TextBlock.h"

void UPotatoDamageTextWidget::SetDamage(int32 Damage)
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(Damage));
	}
}