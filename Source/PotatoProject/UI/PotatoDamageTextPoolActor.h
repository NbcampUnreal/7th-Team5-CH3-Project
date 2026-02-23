// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoDamageTextPoolActor.generated.h"


class APotatoDamageTextActor;

UCLASS()
class POTATOPROJECT_API APotatoDamageTextPoolActor : public AActor
{
	GENERATED_BODY()
	
public:
	APotatoDamageTextPoolActor();

	UFUNCTION(BlueprintCallable)
	void SpawnDamageText(
		int32 Damage,
		const FVector& WorldLocation,
		int32 StackIndex
	);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Pool")
	TSubclassOf<APotatoDamageTextActor> DamageTextActorClass;

	UPROPERTY(EditAnywhere, Category = "Pool")
	int32 PrewarmCount = 40;

private:
	UPROPERTY()
	TArray<APotatoDamageTextActor*> Pool;

	UPROPERTY()
	TArray<APotatoDamageTextActor*> Active;

	APotatoDamageTextActor* Acquire();
	void Release(APotatoDamageTextActor* Actor);

};
