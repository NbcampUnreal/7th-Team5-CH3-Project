#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PotatoGameMode.generated.h"

class PotatoDayNightCycle;

UCLASS()
class POTATOPROJECT_API APotatoGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	PotatoDayNightCycle* CurrentCycle;
	int CurrentWave;
	bool IsNightPhase;

	APotatoGameMode();
	void StartGame();
	void StartDayPhase();
	void StartNightPhase();
	void EndGame();
	void CheckVictoryCondition();
};
