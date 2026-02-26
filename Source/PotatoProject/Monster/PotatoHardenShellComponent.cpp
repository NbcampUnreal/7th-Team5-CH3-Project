#include "PotatoHardenShellComponent.h"

#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UPotatoHardenShellComponent::UPotatoHardenShellComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoHardenShellComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedOwner = GetOwner();
	if (CachedOwner)
	{
		CachedMesh = CachedOwner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

float UPotatoHardenShellComponent::ModifyIncomingDamage(float InDamage) const
{
	if (InDamage <= 0.f) return 0.f;
	if (!bHardened) return InDamage;

	return InDamage * FMath::Max(0.f, DamageMultiplierWhenHardened);
}

bool UPotatoHardenShellComponent::ReadHp(float& OutHP, float& OutMaxHP) const
{
	OutHP = 0.f;
	OutMaxHP = 0.f;

	if (!CachedOwner) return false;

	const UClass* C = CachedOwner->GetClass();
	if (!C) return false;

	FProperty* HpProp = C->FindPropertyByName(HpPropertyName);
	FProperty* MaxProp = C->FindPropertyByName(MaxHpPropertyName);
	if (!HpProp || !MaxProp) return false;

	FFloatProperty* HpFloat = CastField<FFloatProperty>(HpProp);
	FFloatProperty* MaxFloat = CastField<FFloatProperty>(MaxProp);
	if (!HpFloat || !MaxFloat) return false;

	OutHP = HpFloat->GetPropertyValue_InContainer(CachedOwner);
	OutMaxHP = MaxFloat->GetPropertyValue_InContainer(CachedOwner);

	return true;
}

void UPotatoHardenShellComponent::PostDamageCheck()
{
	if (bHardened) return;

	float HP = 0.f, MaxHP = 0.f;
	if (!ReadHp(HP, MaxHP)) return;
	if (MaxHP <= KINDA_SMALL_NUMBER) return;

	const float Pct = HP / MaxHP;
	if (Pct <= TriggerHpPercent)
	{
		bHardened = true;
		ApplyHardenMaterial();
	}
}

void UPotatoHardenShellComponent::ApplyHardenMaterial()
{
	if (!CachedMesh) return;

	const int32 MatNum = CachedMesh->GetNumMaterials();
	for (int32 i = 0; i < MatNum; i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Harden] Mesh=%s Mat[%d]=%s"),
	*GetNameSafe(CachedMesh), i, *GetNameSafe(CachedMesh->GetMaterial(i)));
		UMaterialInterface* M = CachedMesh->GetMaterial(i);
		if (!M) continue;

		UMaterialInstanceDynamic* MID = CachedMesh->CreateAndSetMaterialInstanceDynamic(i);
		if (!MID) continue;
		
		FLinearColor Before;
		const bool bHasBefore = MID->GetVectorParameterValue(TintParamName, Before);

		MID->SetVectorParameterValue(TintParamName, HardenTint);

		FLinearColor After;
		const bool bHasAfter = MID->GetVectorParameterValue(TintParamName, After);

		UE_LOG(LogTemp, Warning, TEXT("[Harden] Mat[%d] MID=%s HasBefore=%d Before=%s HasAfter=%d After=%s Set=%s Param=%s"),
			i,
			*GetNameSafe(MID),
			bHasBefore ? 1 : 0,
			*Before.ToString(),
			bHasAfter ? 1 : 0,
			*After.ToString(),
			*HardenTint.ToString(),
			*TintParamName.ToString());

		MID->SetVectorParameterValue(TintParamName, HardenTint);
	}
}