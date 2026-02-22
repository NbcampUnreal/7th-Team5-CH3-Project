// Fill out your copyright notice in the Description page of Project Settings.


#include "PotatoPlayerAnimInstance.h"
#include "PotatoPlayerCharacter.h"
#include "Combat/PotatoWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPotatoPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	PlayerCharacter = Cast<APotatoPlayerCharacter>(TryGetPawnOwner());
	
	if (PlayerCharacter)
	{
		MovementComponent = PlayerCharacter->GetCharacterMovement();
	}
	
}

void UPotatoPlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	
	if (!PlayerCharacter || !MovementComponent)
	{
		return;
	}
	
	// 1. 속도
	Velocity = MovementComponent->Velocity;
	FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	GroundSpeed = LateralVelocity.Size();
	
	// 2. 낙하
	bIsFalling = MovementComponent->IsFalling();
	
	// 3. 방향
	if (GroundSpeed > 3.0f)
	{
		FRotator CurrentRotation = PlayerCharacter->GetActorRotation();
		LocomotionDirection = CalculateDirection(Velocity, CurrentRotation);
	}
	else
	{
		LocomotionDirection = 0.0f;
	}
	
	// 4. Aim Offset
	FRotator ControlRotation = PlayerCharacter->GetControlRotation();
	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	FRotator DeltaRotation = ControlRotation - ActorRotation;
	DeltaRotation.Normalize();
	AimPitch = DeltaRotation.Pitch;
	
	// 5. Combat Stance
	UPotatoWeaponComponent* WeaponComponent = PlayerCharacter->FindComponentByClass<UPotatoWeaponComponent>();
	if (WeaponComponent)
	{
		bIsInCombatStance = WeaponComponent->IsInCombatStance();
	}
	
}
