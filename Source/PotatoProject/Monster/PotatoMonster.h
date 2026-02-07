#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoMonster.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoMonster : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoMonster();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
