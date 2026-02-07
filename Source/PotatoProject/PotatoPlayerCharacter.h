#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PotatoPlayerCharacter.generated.h"

UCLASS()
class POTATOPROJECT_API APotatoPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APotatoPlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
