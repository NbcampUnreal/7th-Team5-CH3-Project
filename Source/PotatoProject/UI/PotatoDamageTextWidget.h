// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PotatoDamageTextWidget.generated.h"

class UTextBlock;

UCLASS()
class POTATOPROJECT_API UPotatoDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetDamage(int32 Damage);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText = nullptr;
	
};
