// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StackItem.generated.h"

class UTextBlock;
class UImage;

/**
 * WBP_StackItem
 * 75×75 아이템 아이콘 + 우하단 수량 텍스트(StackText)
 */
UCLASS()
class POTATOPROJECT_API UStackItem : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 아이콘 텍스처와 수량 텍스트를 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "StackItem")
	void SetStackData(UTexture2D* InIcon, int32 InAmount);

	/** 수량 텍스트만 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "StackItem")
	void SetAmount(int32 InAmount);

	// ---- BindWidgets ----
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UTextBlock> StackText;
};
