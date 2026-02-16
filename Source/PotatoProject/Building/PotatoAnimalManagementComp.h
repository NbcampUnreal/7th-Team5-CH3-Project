// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/PotatoEnums.h"
#include "PotatoAnimalManagementComp.generated.h"

class UBoxComponent;
class APotatoAnimal;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class POTATOPROJECT_API UPotatoAnimalManagementComp : public UActorComponent
{
	GENERATED_BODY()

public:
    UPotatoAnimalManagementComp();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, Category = "Config")
    int32 MaxAnimalCount = 5;

    /** Enum -> BP 클래스 매핑 (에디터에서 설정) */
    UPROPERTY(EditAnywhere, Category = "Config")
    TMap<EAnimalType, TSubclassOf<APotatoAnimal>> AnimalClassMap;

    UPROPERTY(VisibleInstanceOnly, Category = "State")
    TArray<TObjectPtr<APotatoAnimal>> AssignedAnimals;

private:
    /** BeginPlay에서 Owner의 BoxComponent를 자동 캐싱 */
    UPROPERTY()
    TObjectPtr<UBoxComponent> CachedBoxComp;

public:
    // API
    UFUNCTION(BlueprintCallable)
    bool SpawnAnimal(EAnimalType AnimalType);

    UFUNCTION(BlueprintCallable)
    void RemoveAnimal(APotatoAnimal* Animal);
};
