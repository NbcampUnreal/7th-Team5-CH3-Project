#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoBuildingGhost.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoBuildingGhost : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoBuildingGhost();

protected:
	virtual void BeginPlay() override;

public:	
	UStaticMeshComponent* GhostMesh;
	bool IsValidPlacement;
	FVector Location;
	FRotator Rotation;

	virtual void Tick(float DeltaTime) override;

	void UpdateLocation(FVector NewLocation);
	bool CheckValidPlacement();
	void SetGhostColor(bool IsValid);
};
