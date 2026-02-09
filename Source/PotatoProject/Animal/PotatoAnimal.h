#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoAnimal.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoAnimal : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoAnimal();

protected:
	virtual void BeginPlay() override;

public:	
	EAnimalType Type;
	int CropCost;
	float CropProductionRate;
	float LivestockProductionRate;
	int RefundAmount;

	virtual void Tick(float DeltaTime) override;

	void Produce();
};
