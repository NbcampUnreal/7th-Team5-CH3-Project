#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoWeapon.generated.h"

class APotatoProjectile;
class UPotatoWeaponSystem;

UCLASS()
class POTATOPROJECT_API APotatoWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	APotatoWeapon();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:	
	FVector GetMuzzleLocation() const;
	void PlayKick(const FVector& KickOffset, const FRotator& KickRotation, float RecoverySpeed);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;
	
private:
	FVector TargetKickLocation;
	FRotator TargetKickRotation;
	
	FVector CurrentKickLocation;
	FRotator CurrentKickRotation;
	
	float CurrentRecoverySpeed;
};
