#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"

class APotatoBuilding;
class APotatoBuildingGhost;

class POTATOPROJECT_API PotatoBuildingSystem
{
public:
	bool IsBuildMode;
	APotatoBuildingGhost* CurrentGhost;
	TArray<APotatoBuilding*> PlacedBuildings;

	PotatoBuildingSystem();
	~PotatoBuildingSystem();

	void EnterBuildMode();
	void ExitBuildMode();
	void SelectBuilding(EBuildingType Type);
	void RotateGhost(float Angle);
	void PlaceBuilding();
	void RemoveBuilding(APotatoBuilding* Target);
};
