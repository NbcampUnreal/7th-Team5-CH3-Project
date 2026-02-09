#pragma once

#include "CoreMinimal.h"
#include "PotatoEnums.h"

class POTATOPROJECT_API PotatoResourceManager
{
public:
	PotatoResourceManager();
	~PotatoResourceManager();
	
	int Wood;
	int Stone;
	int Crop;
	int Livestock;
	
	void AddResource(EResourceType Type, int Amount);
	bool RemoveResource(EResourceType Type, int Amount);
	int GetResource(EResourceType Type);
	bool HasEnoughResource(EResourceType Type, int Amount);
};
