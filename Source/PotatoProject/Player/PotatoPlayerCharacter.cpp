#include "PotatoPlayerCharacter.h"
#include "PotatoPlayerController.h"
#include "EnhancedinputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Building/BuildingSystemComponent.h"
#include "Combat/PotatoWeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Core/PotatoGameMode.h" 
#include "../UI/AmmoPopupWidget.h"


APotatoPlayerCharacter::APotatoPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->GravityScale = 1.5f;

	// Create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	//CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// 카메라 줌인/줌아웃: 목표 거리 초기화
	TargetCameraDistance = DefaultCameraDistance;

	// Create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Create build system component
	BuildingComponent = CreateDefaultSubobject<UBuildingSystemComponent>(TEXT("BuildingComponent"));

	// Create weapon component
	WeaponComponent = CreateDefaultSubobject<UPotatoWeaponComponent>(TEXT("WeaponComponent"));

	//빌드모드 가능여부
	IsBuildingMode = true;
	//IsAmmoProduct = false;

	

}

void APotatoPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (AmmoPopupClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Ammo! a")));
		AmmoPopupWidget = CreateWidget<UAmmoPopupWidget>(GetWorld(), AmmoPopupClass);
		if (AmmoPopupWidget)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Ammo! b")));
			AmmoPopupWidget->AddToViewport();
			AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void APotatoPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라 줌인/줌아웃: 보간
	if (CameraBoom && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetCameraDistance, 0.1f))
	{
		float NewDistance = FMath::FInterpTo(
			CameraBoom->TargetArmLength,
			TargetCameraDistance,
			DeltaTime,
			CameraZoomInterpSpeed
		);

		CameraBoom->TargetArmLength = NewDistance;
	}

	if (CurrentHP <= 0)
	{
		OnDeath();
	}
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

			// Combat
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

			if (PlayerController->ToggleBuildAction)
			{
				EnhancedInput->BindAction(
					PlayerController->ToggleBuildAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::OnToggleBuildMode
				);
			}
			if (PlayerController->AmmoAction)
			{
				EnhancedInput->BindAction(
					PlayerController->AmmoAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::OnAmmoMode
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
	if (GetController() != nullptr && AmmoPopupWidget && !AmmoPopupWidget->IsVisible())
	{
		const FVector2D LookAxisVector = Value.Get<FVector2D>();
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APotatoPlayerCharacter::StartSprint(const FInputActionValue& Value)
{
	if (!WeaponComponent)
	{
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		}
	}
	
	if (WeaponComponent->IsReloading())
	{
		return;
	}
	
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void APotatoPlayerCharacter::StopSprint(const FInputActionValue& Value)
{
	if (!WeaponComponent)
	{
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
		}
	}
	
	if (WeaponComponent->IsReloading())
	{
		WeaponComponent->UpdateCachedWalkSpeed(NormalSpeed);
		return;
	}
	
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
		TargetCameraDistance = FMath::Clamp(TargetCameraDistance + (CameraZoomValue * CameraZoomSpeed),
		                                    MinCameraDistance, MaxCameraDistance);
	}
}

void APotatoPlayerCharacter::Attack(const FInputActionValue& Value)
{
	if (WeaponComponent && AmmoPopupWidget && !AmmoPopupWidget->IsVisible())
	{
		WeaponComponent->Fire();
	}
}

void APotatoPlayerCharacter::Reload(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->StartReload();
	}
}

void APotatoPlayerCharacter::WeaponChange(const FInputActionValue& Value)
{
	if (Value.Get<bool>() && WeaponComponent)
	{
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (!PlayerController)
		{
			return;
		}
		
		int32 SlotIndex = 0;
		
		if (PlayerController->IsInputKeyDown(EKeys::One) || PlayerController->IsInputKeyDown(EKeys::NumPadOne))
		{
			SlotIndex = 0;
		}
		if (PlayerController->IsInputKeyDown(EKeys::Two) || PlayerController->IsInputKeyDown(EKeys::NumPadTwo))
		{
			SlotIndex = 1;
		}
		if (PlayerController->IsInputKeyDown(EKeys::Three) || PlayerController->IsInputKeyDown(EKeys::NumPadThree))
		{
			SlotIndex = 2;
		}
		if (PlayerController->IsInputKeyDown(EKeys::Four) || PlayerController->IsInputKeyDown(EKeys::NumPadFour))
		{
			SlotIndex = 3;
		}
		
		if (AmmoPopupWidget && AmmoPopupWidget->IsVisible()) {
			//1 2 3 4로 탄환충전할 거 선택
		}
		else {
			WeaponComponent->EquipWeapon(SlotIndex);
		}
		
	}
}

void APotatoPlayerCharacter::OnToggleBuildMode(const FInputActionValue& Value)
{
	if (BuildingComponent && IsBuildingMode)
	{
		BuildingComponent->ToggleBuildMode();
	}
}

void APotatoPlayerCharacter::SetIsBuildingMode(bool BuildingMode)
{
	IsBuildingMode = BuildingMode;

	//빌드모드 켜져있으면 끄기
	if (BuildingComponent && !IsBuildingMode)
	{
		if (BuildingComponent->bIsBuildMode) {
			BuildingComponent->ToggleBuildMode();
		}
	}
}

void APotatoPlayerCharacter::OnAmmoMode(const FInputActionValue& Value)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());


	if (AmmoPopupWidget && PlayerController)
	{
		if (AmmoPopupWidget->IsVisible()) {
			AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
			PlayerController->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
		else {
			AmmoPopupWidget->SetVisibility(ESlateVisibility::Visible);
			PlayerController->bShowMouseCursor = true;
		}
	}


}


void APotatoPlayerCharacter::OnDeath()
{
	APotatoGameMode* GameMode = Cast<APotatoGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		GameMode->EndGame(false);
	}
	//캐릭터 죽는 애니메이션 실행필요
}

float APotatoPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);

		///UE_LOG(LogTemp, Warning, TEXT("Remaining Health: %f"), CurrentHealth);

		if (CurrentHP <= 0.0f)
		{
			OnDeath();
		}
	}

	return ActualDamage;
}