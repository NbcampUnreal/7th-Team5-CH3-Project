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

protected:
	virtual void BeginPlay() override;

public:	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;
	
	FVector GetMuzzleLocation() const;
};
