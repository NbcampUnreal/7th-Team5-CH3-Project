#pragma once

#include "CoreMinimal.h"

class POTATOPROJECT_API PotatoDayNightCycle
{
public:
	float PhaseTimer;
	float DayDuration;
	float NightDuration;
	bool IsDay;

	PotatoDayNightCycle();
	~PotatoDayNightCycle();

	void StartDay();
	void StartNight();
	void UpdateTimer(float DeltaTime);
	float GetRemainingTime();
	void TriggerWarning();
};
