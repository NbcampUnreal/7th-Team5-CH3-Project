#pragma once

#include "CoreMinimal.h"
#include "Subsystems/Subsystem.h"
#include "PotatoDayNightCycle.generated.h"

UCLASS()
class POTATOPROJECT_API UPotatoDayNightCycle : public USubsystem
{
	GENERATED_BODY()
public:
	float PhaseTimer;
	float DayDuration;
	float NightDuration;
	bool IsDay;

	void StartDay();
	void StartNight();
	void UpdateTimer(float DeltaTime);
	float GetRemainingTime();
	void TriggerWarning();
};
