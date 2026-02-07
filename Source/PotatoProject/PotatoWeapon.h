#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoWeapon.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoWeapon();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
