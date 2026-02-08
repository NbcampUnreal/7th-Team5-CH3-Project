#pragma once

#include "CoreMinimal.h"
#include "PotatoBuilding.h"
#include "PotatoLumberMill.generated.h"

class APotatoNPC;

UCLASS()
class POTATOPROJECT_API APotatoLumberMill : public APotatoBuilding
{
	GENERATED_BODY()
public:
	float WoodProductionRate;
	APotatoNPC* AssignedWorker;

	int ProduceWood();
	bool HireNPC();
	float GetProductionBonus();
};
