#include "PotatoPlayerCharacter.h"
#include "PotatoPlayerController.h"
#include "EnhancedinputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "../Combat/PotatoWeapon.h"

APotatoPlayerCharacter::APotatoPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	
	// 카메라 줌인/줌아웃: 목표 거리 초기화
	TargetCameraDistance = DefaultCameraDistance;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//
	
}

void APotatoPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	WeaponEqiup(true);
}

void APotatoPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 카메라 줌인/줌아웃: 보간
	if(CameraBoom && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetCameraDistance, 0.1f))
	{
		float NewDistance = FMath::FInterpTo(
			CameraBoom->TargetArmLength,
			TargetCameraDistance,
			DeltaTime,
			CameraZoomInterpSpeed
		);
		
		CameraBoom->TargetArmLength = NewDistance;
	}

	WeaponRotate();
}

void APotatoPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetController()))
		{
			// Character Movement
			if (PlayerController->MoveAction)
			{
				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&APotatoPlayerCharacter::Move
				);
			}

			if (PlayerController->JumpAction)
			{
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::StartJump
				);
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Completed,
					this,
					&APotatoPlayerCharacter::StopJump
				);
			}

			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&APotatoPlayerCharacter::Look
				);
			}

			if (PlayerController->SprintAction)
			{
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Triggered,
					this,
					&APotatoPlayerCharacter::StartSprint);

				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Completed,
					this,
					&APotatoPlayerCharacter::StopSprint);
			}
			
			// Camera
			if (PlayerController->CameraZoomAction)
			{
				EnhancedInput->BindAction(
					PlayerController->CameraZoomAction,
					ETriggerEvent::Triggered,
					this,
					&APotatoPlayerCharacter::CameraZoom
				);
			}


			if (PlayerController->AttackAction)
			{
				EnhancedInput->BindAction(
					PlayerController->AttackAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::Attack
				);
			}

			if (PlayerController->ReloadAction)
			{
				EnhancedInput->BindAction(
					PlayerController->ReloadAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::Reload
				);
			}
			if (PlayerController->WeaponChangeAction)
			{
				EnhancedInput->BindAction(
					PlayerController->WeaponChangeAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::WeaponChange
				);
			}
		}
	}
}

void APotatoPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (GetController() != nullptr)
	{
		const FVector2D MovementVector = Value.Get<FVector2D>();
		
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		AddMovementInput(ForwardDirection, MovementVector.X);
		AddMovementInput(RightDirection, MovementVector.Y);
	}
}

void APotatoPlayerCharacter::StartJump(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		Jump();
	}
}

void APotatoPlayerCharacter::StopJump(const FInputActionValue& Value)
{
	if (!Value.Get<bool>())
	{
		StopJumping();
	}
}

void APotatoPlayerCharacter::Look(const FInputActionValue& Value)
{
	if (GetController() != nullptr)
	{
		const FVector2D LookAxisVector = Value.Get<FVector2D>();
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APotatoPlayerCharacter::StartSprint(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void APotatoPlayerCharacter::StopSprint(const FInputActionValue& Value)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	}
}

void APotatoPlayerCharacter::CameraZoom(const FInputActionValue& Value)
{
	float CameraZoomValue = Value.Get<float>();
	
	if (CameraBoom && CameraZoomValue != 0.0f)
	{
		TargetCameraDistance = FMath::Clamp(TargetCameraDistance + (CameraZoomValue * CameraZoomSpeed), MinCameraDistance, MaxCameraDistance);
	}
}

void APotatoPlayerCharacter::Attack(const FInputActionValue& Value)
{

	if (Value.Get<bool>())
	{
	//UE_LOG(LogTemp, Log, TEXT("isAttack1!"));
		if (Weapon)
		{
			//UE_LOG(LogTemp, Log, TEXT("isAttack2"));
			Weapon->Fire();
		}
	}
}
void APotatoPlayerCharacter::Reload(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		if (Weapon)
		{
			Weapon->Reload();
		}

	}
}
void APotatoPlayerCharacter::WeaponChange(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) {
	
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;
		int32 WeaponIndex = 1;
		if (PC->IsInputKeyDown(EKeys::One) || PC->IsInputKeyDown(EKeys::NumPadOne))
		{
			WeaponIndex = 1;
		}
		if (PC->IsInputKeyDown(EKeys::Two) || PC->IsInputKeyDown(EKeys::NumPadTwo))
		{
			WeaponIndex = 2;
		}
		if (PC->IsInputKeyDown(EKeys::Three) || PC->IsInputKeyDown(EKeys::NumPadThree))
		{
			WeaponIndex = 3;
		}
		if (PC->IsInputKeyDown(EKeys::Four) || PC->IsInputKeyDown(EKeys::NumPadFour))
		{
			WeaponIndex = 4;
		}
		Weapon->ChangeWeapon(WeaponIndex);
	}

}

void APotatoPlayerCharacter::WeaponEqiup(bool isEquip)
{
	if (isEquip) {
		if (WeaponOrigin)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			//SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (!Weapon) {
				Weapon = GetWorld()->SpawnActor<APotatoWeapon>(WeaponOrigin, GetActorLocation(), GetActorRotation(), SpawnParams);
				FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, // 위치는 소켓에 붙임
					EAttachmentRule::KeepWorld,    // 회전은 방금 설정한 월드값 유지
					EAttachmentRule::KeepWorld,    // 스케일 유지
					true);
				Weapon->AttachToComponent(GetMesh(), AttachRules, TEXT("WeaponSocket"));
				//Weapon->GetRootComponent()->SetUsingAbsoluteRotation(true);
				//Weapon->SetActorRotation(FRotator(0.f, 90.f, 0.f));
			}
		}
	}
	else {
		if (Weapon)
		{
			Weapon->Destroy();
		}
	}
}

void APotatoPlayerCharacter::WeaponRotate()
{
	if (Weapon && FollowCamera)
	{
		Weapon->SetActorRotation(FollowCamera->GetComponentRotation());
	}
	//
}