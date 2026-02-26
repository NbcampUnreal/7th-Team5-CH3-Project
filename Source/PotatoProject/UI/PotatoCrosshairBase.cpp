// Fill out your copyright notice in the Description page of Project Settings.


#include "PotatoCrosshairBase.h"

#include "Combat/PotatoWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/PotatoPlayerCharacter.h"

void UPotatoCrosshairBase::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacter = Cast<APotatoPlayerCharacter>(GetOwningPlayerPawn());
	if (PlayerCharacter)
	{
		WeaponComponent = PlayerCharacter->FindComponentByClass<UPotatoWeaponComponent>();
	}
	//CurrentSpreadValue = BaseSpread;
}

void UPotatoCrosshairBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

float UPotatoCrosshairBase::CalculateCurrentSpread(float DeltaTime)
{
    if (!WeaponComponent) return 0.0f;

    // WeaponComponent가 계산한 동적 각도를 픽셀로 변환
    float TargetSpreadPixels = WeaponComponent->GetCurrentSpreadAngle() * SpreadDegreesToPixels;
    

	//if (!PlayerCharacter || !WeaponComponent)
	//{
	//	return BaseSpread;
	//}

	//float TargetSpread = BaseSpread;

	//// 1. 속도에 따른 Spread
	//float PlayerSpeed = PlayerCharacter->GetVelocity().Size();
	//float VelocityFactor = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 600.0f), FVector2D(0.0f, 50.0f),
	//                                                         PlayerSpeed);
	//TargetSpread += VelocityFactor * VelocitySpreadMultiplier;
	//
	//// 2. 점프에 따른 Spread
	//if (PlayerCharacter->GetCharacterMovement()->IsFalling())
	//{
	//	TargetSpread += JumpingSpread;
	//}
	//
	//// 3. 발사에 따른 Spread
	//if (WeaponComponent)
	//{
	//	float TimeSinceFire = GetWorld()->GetTimeSeconds() - WeaponComponent->GetLastFireTime();
	//	if (TimeSinceFire < FiringSpreadDuration)
	//	{
	//		TargetSpread += FiringSpread;
	//	}
	//}
	//
	
    // 4. 보간
	//CurrentSpreadValue = FMath::FInterpTo(CurrentSpreadValue, TargetSpread, DeltaTime, SpreadInterpSpeed);
    CurrentSpreadValue = FMath::FInterpTo(CurrentSpreadValue, TargetSpreadPixels, DeltaTime, SpreadInterpSpeed);
	return CurrentSpreadValue;
}
