#pragma once

#include "CoreMinimal.h"

class APotatoWeapon;

class POTATOPROJECT_API PotatoWeaponSystem
{
public:
	PotatoWeaponSystem();
	~PotatoWeaponSystem();

	TArray<APotatoWeapon*> WeaponSlots;
	int CurrentSlotIndex;
	APotatoWeapon* CurrentWeapon;

	void EquipWeapon(int SlotIndex);
	void Fire();
	void Reload();
	void SwitchWeapon(int SlotIndex);
};
