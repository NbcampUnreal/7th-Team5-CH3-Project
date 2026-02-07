#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoBuilding.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoBuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoBuilding();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
