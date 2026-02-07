#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoNPC.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoNPC : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoNPC();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
