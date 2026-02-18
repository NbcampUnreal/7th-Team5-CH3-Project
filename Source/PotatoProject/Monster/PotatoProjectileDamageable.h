// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PotatoProjectileDamageable.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class POTATOPROJECT_API UPotatoProjectileDamageable : public UInterface
{
	GENERATED_BODY()
};

class POTATOPROJECT_API IPotatoProjectileDamageable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void SetProjectileDamage(float InDamage);
};
