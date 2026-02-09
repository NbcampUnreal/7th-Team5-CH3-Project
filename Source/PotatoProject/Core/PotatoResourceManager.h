#pragma once

#include "CoreMinimal.h"
#include "../Core/PotatoEnums.h"
#include "Subsystems/Subsystem.h"
#include "PotatoResourceManager.generated.h"

UCLASS()
class POTATOPROJECT_API UPotatoResourceManager : public USubsystem
{
	GENERATED_BODY()
public:
	int Wood;
	int Stone;
	int Crop;
	int Livestock;

	void AddResource(EResourceType Type, int Amount);
	bool RemoveResource(EResourceType Type, int Amount);
	int GetResource(EResourceType Type);
	bool HasEnoughResource(EResourceType Type, int Amount);
};
