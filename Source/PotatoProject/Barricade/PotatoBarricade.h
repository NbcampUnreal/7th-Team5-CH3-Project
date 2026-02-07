#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoBarricade.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoBarricade : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoBarricade();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
