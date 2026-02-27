#include "PotatoPlaceableStructure.h"
#include "PotatoStructureData.h"
#include "Core/PotatoProductionComponent.h"

APotatoPlaceableStructure::APotatoPlaceableStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	ProductionComponent = CreateDefaultSubobject<UPotatoProductionComponent>(TEXT("ProductionComponent"));
}

void APotatoPlaceableStructure::BeginPlay()
{
	Super::BeginPlay();
	
	if (StructureData)
	{
		if (StructureData->bIsDestructible)
		{
			CurrentHealth = StructureData->MaxHealth;
		}
		else
		{
			CurrentHealth = -1.0f; // 파괴 불가능
		}
		
		UE_LOG(LogTemp, Warning, TEXT("%s Initialized!"), *StructureData->DisplayName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Structure 객체가 DataAsset 없이 스폰됐습니다!"));
	}
}

void APotatoPlaceableStructure::Interact(AActor* Interactor)
{
	if (IsDestructible() && CurrentHealth <= 0.0f)
	{
		return;
	}
	
	if (OnInteractedDelegate.IsBound())
	{
		OnInteractedDelegate.Broadcast(Interactor);
	}
}

bool APotatoPlaceableStructure::IsDestructible() const
{
	if (StructureData)
	{
		return StructureData->bIsDestructible;
	}

	return false;
}

float APotatoPlaceableStructure::TakeDamage(float DamageAmount, 
											FDamageEvent const& DamageEvent,
											AController* EventInstigator, 
											AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (!StructureData)
	{
		return 0.0f;
	}
	if (bDestroyed)
	{
		return 0.0f;
	}
	if (!IsDestructible() || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}
	
	const float DamageToApply = FMath::Min(ActualDamage, CurrentHealth);
    
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0.0f, StructureData->MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		// 파괴 처리: FX 재생 등
		// OnStructureDestroyed.Broadcast();
		bDestroyed = true;
		Destroy();
	}
	return DamageToApply;
}
