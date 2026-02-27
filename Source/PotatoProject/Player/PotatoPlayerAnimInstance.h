#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PotatoPlayerAnimInstance.generated.h"

class APotatoPlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class POTATOPROJECT_API UPotatoPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "References")
	APotatoPlayerCharacter* PlayerCharacter;
	
	UPROPERTY(BlueprintReadOnly, Category = "References")
	UCharacterMovementComponent* MovementComponent;
	
	// AnimBP를 위한 상태 변수
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity;
	
	/** 액터 회전 방향: -180 ~ 180, 0은 전방 */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LocomotionDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;
	
	/** 조준의 Pitch값: -90은 아래, +90은 위 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float AimPitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInCombatStance;
	
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bShouldUseCombatUpperBody;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;
};
