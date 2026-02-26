// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StackItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UStackItem::SetStackData(EIconItemType InIconType, int32 InAmount, EStackItemType PlusMinus)
{
	if (ItemImage)
	{
        if (TObjectPtr<UTexture2D>* Found = IconMap.Find(InIconType))
        {
            ItemImage->SetBrushFromTexture(*Found);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UStackItem::SetStackData - Icon not found for type %d"), (int32)InIconType);
        }
	}
	SetAmount(InAmount, PlusMinus);
}

void UStackItem::SetAmount(int32 InAmount, EStackItemType PlusMinus)
{
	if (StackText)
	{
        // EStackItemType에 따라 + 또는 - 기호를 붙여서 텍스트를 설정합니다.
        FText AmountText;
        switch(PlusMinus)
        {
            case EStackItemType::Plus:
                AmountText = FText::Format(NSLOCTEXT("StackItem", "PlusAmount", "+{0}"), InAmount);
                break;
            case EStackItemType::Minus:
                AmountText = FText::Format(NSLOCTEXT("StackItem", "MinusAmount", "-{0}"), InAmount);
                break;
            default:
                AmountText = FText::AsNumber(InAmount);
                break;
        }
		
		StackText->SetText(AmountText);
	}
}
