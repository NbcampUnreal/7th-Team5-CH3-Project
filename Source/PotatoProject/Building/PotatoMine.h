#pragma once

#include "CoreMinimal.h"
#include "PotatoBuilding.h"
#include "PotatoMine.generated.h"

class APotatoNPC;

UCLASS()
class POTATOPROJECT_API APotatoMine : public APotatoBuilding
{
	GENERATED_BODY()
public:
	float StoneProductionRate;
	APotatoNPC* AssignedWorker;

	int ProduceStone();
	bool HireNPC();
	float GetProductionBonus();
};
