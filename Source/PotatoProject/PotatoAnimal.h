#pragma once

#include "CoreMinimal.h"
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
	virtual void Tick(float DeltaTime) override;

};
