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

    float SpreadAngle = WeaponComponent->GetCurrentSpreadAngle(); // degree 단위의 분산 각도
    float TargetSpreadPixels = 0.0f;

    APlayerController* PlayerController = GetOwningPlayer();
    if (PlayerController && PlayerController->PlayerCameraManager)
    {
        float FOV = PlayerController->PlayerCameraManager->GetFOVAngle(); // 카메라의 FOV

        int32 ViewportW = 0, ViewportH = 0;
        PlayerController->GetViewportSize(ViewportW, ViewportH);

        // ScreenPixels = ViewportWidth × tan(SpreadAngle) / tan(FOV / 2) 
        // - https://forums.unrealengine.com/t/help-on-calculating-crosshair-spread-w-fov/583683에서 확인된 FOV 반영 공식
        // Tan(spreadangle) / Tan(FOV/2) * (ViewportWidth/2) = SpreadPixels
        float TanSpread = FMath::Tan(FMath::DegreesToRadians(SpreadAngle));
        float TanHalfFOV = FMath::Tan(FMath::DegreesToRadians(FOV / 2.0f));

        if (TanHalfFOV > 0.0f)
        {
            TargetSpreadPixels = (TanSpread / TanHalfFOV) * (ViewportW * 0.5f);
        }
    }


    // WeaponComponent가 계산한 동적 각도를 픽셀로 변환
    //float TargetSpreadPixels = WeaponComponent->GetCurrentSpreadAngle() * SpreadDegreesToPixels;
  
    // 4. 보간
	//CurrentSpreadValue = FMath::FInterpTo(CurrentSpreadValue, TargetSpread, DeltaTime, SpreadInterpSpeed);
    CurrentSpreadValue = FMath::FInterpTo(CurrentSpreadValue, TargetSpreadPixels, DeltaTime, SpreadInterpSpeed);
	return CurrentSpreadValue;
}
