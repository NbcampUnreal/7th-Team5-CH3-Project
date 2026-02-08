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
	float Health;
	float MaxHealth;
	FVector Location;
	FRotator Rotation;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(FVector Direction);
	void Jump();
	void Sprint();
	void TakeDamage(float Damage);
	void Die();
};
