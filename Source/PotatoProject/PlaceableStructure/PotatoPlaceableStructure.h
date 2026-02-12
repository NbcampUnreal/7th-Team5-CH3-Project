#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Interactable.h"
#include "PotatoPlaceableStructure.generated.h"

class UPotatoStructureData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStructureInteracted, AActor*, Interactor);

UCLASS()
class POTATOPROJECT_API APotatoPlaceableStructure : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	APotatoPlaceableStructure();

protected:
	virtual void BeginPlay() override;

public:	
	// Data Asset
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<const UPotatoStructureData> StructureData;
	
	// Stats: 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;
	
	// Interaction: 상호작용
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnStructureInteracted OnInteractedDelegate;
	
	virtual void Interact(AActor* Interactor) override;
	
	// 헬퍼 함수
	/** 이 구조물이 파괴될 수 있으면 true를 반환 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	bool IsDestructible() const;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual float TakeDamage(float DamageAmount, 
						struct FDamageEvent const& DamageEvent,
						AController* EventInstigator, 
						AActor* DamageCauser) override;
};
