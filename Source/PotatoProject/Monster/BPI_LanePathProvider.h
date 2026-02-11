// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BPI_LanePathProvider.generated.h"

UINTERFACE(BlueprintType)
class POTATOPROJECT_API UBPI_LanePathProvider : public UInterface
{
    GENERATED_BODY()
};

class POTATOPROJECT_API IBPI_LanePathProvider
{
    GENERATED_BODY()

public:
    // SpawnGroup(LaneId) -> Lane Points
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lane")
    void GetLanePoints(FName LaneId, TArray<AActor*>& Points) const;
};