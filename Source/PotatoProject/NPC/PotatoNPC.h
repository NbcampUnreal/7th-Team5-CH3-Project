#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoNPC.generated.h"

class APotatoBuilding;

UCLASS()
class POTATOPROJECT_API APotatoNPC : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoNPC();

protected:
	virtual void BeginPlay() override;

public:	
	ENPCType Type;
	int CropHireCost;
	int LivestockMaintenanceCost;
	float ProductionBonus;
	APotatoBuilding* AssignedBuilding;
	
	virtual void Tick(float DeltaTime) override;

	void Work();
	bool PayMaintenance();
	void Retire();
};
