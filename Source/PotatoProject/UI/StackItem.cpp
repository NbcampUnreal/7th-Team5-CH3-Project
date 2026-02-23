// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StackItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UStackItem::SetStackData(UTexture2D* InIcon, int32 InAmount)
{
	if (ItemImage && InIcon)
	{
		ItemImage->SetBrushFromTexture(InIcon);
	}
	SetAmount(InAmount);
}

void UStackItem::SetAmount(int32 InAmount)
{
	if (StackText)
	{
		// 양수면 "+N", 음수면 "-N" 형태로 표시
		const FText AmountText = InAmount >= 0
			? FText::Format(NSLOCTEXT("StackItem", "Plus", "+{0}"), InAmount)
			: FText::Format(NSLOCTEXT("StackItem", "Minus", "-{0}"), InAmount);
		StackText->SetText(AmountText);
	}
}
