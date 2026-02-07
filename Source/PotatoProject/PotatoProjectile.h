#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoProjectile.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
