// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/PotatoEnums.h"
#include "PotatoGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class POTATOPROJECT_API APotatoGameStateBase : public AGameState
{
	GENERATED_BODY()
	
private:
    int32 TotalKilledEnemy = 0;

    int32 TotalKilledNormal = 0;
    int32 TotalKilledElite = 0;
    int32 TotalKilledBoss = 0;

    int32 CurrentDayKilledNormal = 0;
    int32 CurrentDayKilledElite = 0;
    int32 CurrentDayKilledBoss = 0;

public:
    void AddKill(EMonsterRank Rank);
    void ResetKillStats();

    int32 GetKilledNormal() const { return CurrentDayKilledNormal; }
    int32 GetKilledElite() const { return CurrentDayKilledElite; }
    int32 GetKilledBoss() const { return CurrentDayKilledBoss; }

    int32 GetTotalKilledNormal() const { return TotalKilledNormal; }
    int32 GetTotalKilledElite() const { return TotalKilledElite; }
    int32 GetTotalKilledBoss() const { return TotalKilledBoss; }

    int32 GetTotalKilledEnemy() const { return TotalKilledEnemy; }
};
