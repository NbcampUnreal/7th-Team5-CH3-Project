#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoTypes.h"



class POTATOPROJECT_API PotatoMonsterSpawner
{
public:
	TArray<FWaveData> Waves;
	int CurrentWaveIndex;
	float SpawnTimer;

	PotatoMonsterSpawner();
	~PotatoMonsterSpawner();

	void SpawnWave();
	void SpawnMonster(EMonsterType Type, FVector Location);
};
