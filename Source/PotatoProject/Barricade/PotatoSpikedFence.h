#pragma once

#include "CoreMinimal.h"
#include "PotatoBarricade.h"
#include "PotatoSpikedFence.generated.h"

class APotatoMonster;

UCLASS()
class POTATOPROJECT_API APotatoSpikedFence : public APotatoBarricade
{
	GENERATED_BODY()
	
public:
	float ContactDamage;
	void DamageMonster(APotatoMonster* Target);
};
