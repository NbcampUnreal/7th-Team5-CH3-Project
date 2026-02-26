#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/DamageType.h"
#include "PotatoDamageLibrary.generated.h"

/**
 * 공용 AoE 판정/적용 유틸
 * - Circle: Radius Overlap
 * - Cone: Overlap 후 방향/각도 필터
 * - Line: Capsule Sweep (Start->End)
 *
 * Blueprint 호환: HitOnce는 TArray<AActor*>로 관리
 */
UCLASS()
class POTATOPROJECT_API UPotatoDamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Potato|Damage")
	static int32 ApplyAoE_Circle(
		UObject* WorldContextObject,
		AActor* DamageCauser,
		AController* InstigatorController,
		const FVector& Center,
		float Radius,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass,
		const TArray<AActor*>& IgnoreActors,
		bool bHitOncePerTarget,
		TArray<AActor*>& InOutHitOnceList
	);

	UFUNCTION(BlueprintCallable, Category="Potato|Damage")
	static int32 ApplyAoE_Cone(
		UObject* WorldContextObject,
		AActor* DamageCauser,
		AController* InstigatorController,
		const FVector& Origin,
		const FVector& Forward,
		float Radius,
		float HalfAngleDeg,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass,
		const TArray<AActor*>& IgnoreActors,
		bool bHitOncePerTarget,
		TArray<AActor*>& InOutHitOnceList
	);

	UFUNCTION(BlueprintCallable, Category="Potato|Damage")
	static int32 ApplyAoE_Line(
		UObject* WorldContextObject,
		AActor* DamageCauser,
		AController* InstigatorController,
		const FVector& Start,
		const FVector& End,
		float ThicknessRadius,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass,
		const TArray<AActor*>& IgnoreActors,
		bool bHitOncePerTarget,
		TArray<AActor*>& InOutHitOnceList
	);

	/** 공용: 대상 필터 + ApplyDamage */
	static bool TryApplyDamageOnce(
		AActor* Victim,
		AActor* DamageCauser,
		AController* InstigatorController,
		float Damage,
		TSubclassOf<UDamageType> DamageTypeClass,
		bool bHitOncePerTarget,
		TArray<AActor*>& InOutHitOnceList
	);
};