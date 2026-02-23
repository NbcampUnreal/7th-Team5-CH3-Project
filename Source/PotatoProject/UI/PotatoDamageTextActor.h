// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoDamageTextActor.generated.h"

class UWidgetComponent;
class UPotatoDamageTextWidget;

DECLARE_DELEGATE_OneParam(FPotatoDamageTextFinished, APotatoDamageTextActor*);

UCLASS()
class POTATOPROJECT_API APotatoDamageTextActor : public AActor
{
	GENERATED_BODY()
public:
	APotatoDamageTextActor();

	// DamageText 표시 시작
	void ShowDamage(
		int32 Damage,
		const FVector& WorldLocation,
		int32 StackIndex,
		FPotatoDamageTextFinished InOnFinished
	);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|UI")
	TSubclassOf<UUserWidget> DamageTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float LifeTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float BaseRiseSpeed = 60.f;

private:
	UPROPERTY()
	UWidgetComponent* WidgetComp = nullptr;

	UPROPERTY()
	UPotatoDamageTextWidget* Widget = nullptr;

	FPotatoDamageTextFinished OnFinished;

	float Elapsed = 0.f;
	float CurrentRiseSpeed = 60.f;

	FVector StartLoc = FVector::ZeroVector;

	void EnsureWidgetClassApplied();

};
