#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoBuilding.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoBuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoBuilding();

protected:
	virtual void BeginPlay() override;

public:	
	EBuildingType Type;
	int WoodCost;
	int StoneCost;
	float Health; 
	float MaxHealth; 
	FVector Location;

	virtual void Tick(float DeltaTime) override;

	void Construct();
	void TakeDamage(float Damage);
	void Repair();
	void Destroy();
};
