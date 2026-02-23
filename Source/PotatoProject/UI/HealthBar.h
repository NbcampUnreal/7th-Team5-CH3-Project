// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "HealthBar.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetHealthRatio(float Ratio);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar = nullptr;
};