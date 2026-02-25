// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PotatoGameStateBase.h"

void APotatoGameStateBase::AddKill(EMonsterRank Rank)
{
    switch (Rank)
    {
        case EMonsterRank::Normal:
            CurrentDayKilledNormal++;
            TotalKilledNormal++;
            TotalKilledEnemy++;
            break;
        case EMonsterRank::Elite:
            CurrentDayKilledElite++;
            TotalKilledElite++;
            TotalKilledEnemy++;
            break;
        case EMonsterRank::Boss:
            CurrentDayKilledBoss++;
            TotalKilledBoss++;
            TotalKilledEnemy++;
            break;
        default:
            break;
    }
}

void APotatoGameStateBase::ResetKillStats()
{
    CurrentDayKilledBoss = 0;
    CurrentDayKilledElite = 0;
    CurrentDayKilledBoss = 0;
}
