#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "PotatoBuilding.h"
#include "PotatoRanch.generated.h"

class APotatoAnimal;

UCLASS()
class POTATOPROJECT_API APotatoRanch : public APotatoBuilding
{
	GENERATED_BODY()
	
public:
	TArray<APotatoAnimal*> Animals;
	int MaxAnimalCount;

	bool PlaceAnimal(EAnimalType Type);
	int RemoveAnimal(APotatoAnimal* Target);
	void ReplaceAnimal(APotatoAnimal* Old, EAnimalType New);
	int ProduceLivestock();
};
