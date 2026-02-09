#pragma once

#include "CoreMinimal.h"
#include "PotatoBarricade.h"
#include "PotatoStoneWall.generated.h"

class APotatoMonster;

UCLASS()
class POTATOPROJECT_API APotatoStoneWall : public APotatoBarricade
{
	GENERATED_BODY()
public:
	float SlowEffect;
	void ApplySlowToMonster(APotatoMonster* Target);
};
