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

	// 몬스터/블루프린트에서 호출
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void SpawnDamageText(
		int32 Damage,
		const FVector& WorldLocation,
		int32 StackIndex
	);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 풀에서 생성할 액터 클래스
	UPROPERTY(EditAnywhere, Category = "Pool")
	TSubclassOf<APotatoDamageTextActor> DamageTextActorClass;

	// 미리 만들어둘 개수
	UPROPERTY(EditAnywhere, Category = "Pool", meta = (ClampMin = "0"))
	int32 PrewarmCount = 40;

private:
	// GC 안정성: UPROPERTY로 잡아둠
	UPROPERTY()
	TArray<TObjectPtr<APotatoDamageTextActor>> Pool;

	// Active는 중복/탐색 안정성 위해 Set 권장
	UPROPERTY()
	TSet<TObjectPtr<APotatoDamageTextActor>> Active;

	APotatoDamageTextActor* Acquire();

	// 델리게이트 바인딩을 안전하게/명확하게 하기 위해 UFUNCTION 권장
	UFUNCTION()
	void Release(APotatoDamageTextActor* Actor);

	// 내부 유틸
	void Prewarm();
	APotatoDamageTextActor* SpawnNewPooledActor();
};