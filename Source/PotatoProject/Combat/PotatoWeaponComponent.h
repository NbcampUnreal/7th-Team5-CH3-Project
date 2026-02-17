#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PotatoWeaponComponent.generated.h"

class UPotatoWeaponData;
class APotatoWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UPotatoWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPotatoWeaponComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TObjectPtr<UPotatoWeaponData>> WeaponSlots;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentWeaponIndex = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UPotatoWeaponData> CurrentWeaponData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<APotatoWeapon> CurrentWeaponActor;
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Fire();
	
	FVector GetMuzzleLocation() const;
	
private:
	void SpawnWeapon(TSubclassOf<APotatoWeapon> NewClass);
};
