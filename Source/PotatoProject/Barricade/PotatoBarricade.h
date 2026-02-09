#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "GameFramework/Actor.h"
#include "PotatoBarricade.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoBarricade : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoBarricade();

protected:
	virtual void BeginPlay() override;

public:	
	EBarricadeType Type;
	int WoodCost;
	int StoneCost;
	int CropCost;
	float Health;
	float MaxHealth;
	FVector Location;

	virtual void Tick(float DeltaTime) override;

	void TakeDamage(float Damage);
	void Repair();
	void Destroy();
};
