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
#include "../UI/AnimalPopup.h"
#include "UI/PauseMenu.h"
#include "Components/CapsuleComponent.h"
#include "../Building/PotatoAnimalManagementComp.h"
#include "../UI/NPCPopup.h"
#include "../Building/PotatoNPCManagementComp.h"
#include "Combat/PotatoWeaponData.h"

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

	//AnimalManagementComp = CreateDefaultSubobject<UPotatoAnimalManagementComp>(TEXT("AnimalComponent"));;
	//빌드모드 가능여부
	IsBuildingMode = true;

	// 동물관리 가능 여부
	IsBarnMode = false;
	// NPC 관리 가능 여부
	IsNPCMode = false;
	//IsAmmoProduct = false;

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &APotatoPlayerCharacter::OnOverlapBegin);
		GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &APotatoPlayerCharacter::OnOverlapEnd);
	}

	// 초기 HP 브로드캐스트
	if (OnHPChanged.IsBound())
	{
		OnHPChanged.Broadcast(CurrentHP, MaxHP);
	}
}

void APotatoPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (AmmoPopupClass)
	{
		AmmoPopupWidget = CreateWidget<UAmmoPopupWidget>(GetWorld(), AmmoPopupClass);
		AmmoPopupWidget->InitPopup(WeaponComponent);
		if (AmmoPopupWidget)
		{
			AmmoPopupWidget->AddToViewport();
			AmmoPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (AnimalPopupClass)
	{
		AnimalPopupWidget = CreateWidget<UAnimalPopup>(GetWorld(), AnimalPopupClass);
		/*if(AnimalManagementComp)
		{
			AnimalPopupWidget->InitPopup(AnimalManagementComp);
		}*/
		if (AnimalPopupWidget)
		{
			AnimalPopupWidget->AddToViewport();
			AnimalPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (NPCPopupClass)
	{
		NPCPopupWidget = CreateWidget<UNPCPopup>(GetWorld(), NPCPopupClass);
		if (NPCPopupWidget)
		{
			NPCPopupWidget->AddToViewport();
			NPCPopupWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

    if (PauseMenuClass)
    {
        PauseMenuWidget = CreateWidget<UUserWidget>(GetWorld(), PauseMenuClass);
        if (PauseMenuWidget)
        {
            PauseMenuWidget->AddToViewport();
            PauseMenuWidget->SetVisibility(ESlateVisibility::Hidden);
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

                EnhancedInput->BindAction(
                    PlayerController->AttackAction,
                    ETriggerEvent::Triggered,
                    this,
                    &APotatoPlayerCharacter::AttackHeld
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
			if (PlayerController->BarnAction)
			{
				EnhancedInput->BindAction(
					PlayerController->BarnAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::OnBarnMode
				);
			}
            if (PlayerController->PauseAction)
            {
                EnhancedInput->BindAction(
                    PlayerController->PauseAction,
                    ETriggerEvent::Started,
                    this,
                    &APotatoPlayerCharacter::OnPauseGame
                );
            }
			if (PlayerController->NextDialogueAction)
			{
				EnhancedInput->BindAction(
					PlayerController->NextDialogueAction,
					ETriggerEvent::Started,
					this,
					&APotatoPlayerCharacter::OnNextDialogue
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
	bool IsAmmoWidget = AmmoPopupWidget && !AmmoPopupWidget->IsVisible();
	bool IsAnimalWidget= AnimalPopupWidget && !AnimalPopupWidget->IsVisible();
	bool IsNPCWidget = NPCPopupWidget && !NPCPopupWidget->IsVisible();
	if (GetController() != nullptr && IsAmmoWidget && IsAnimalWidget && IsNPCWidget)
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
	bool IsAmmoWidget = AmmoPopupWidget && !AmmoPopupWidget->IsVisible();
	bool IsAnimalWidget = AnimalPopupWidget && !AnimalPopupWidget->IsVisible();
	bool IsNPCWidget = NPCPopupWidget && !NPCPopupWidget->IsVisible();
	if (WeaponComponent && IsAmmoWidget && IsAnimalWidget && IsNPCWidget)
	{
		WeaponComponent->Fire();
	}
}

void APotatoPlayerCharacter::AttackHeld(const FInputActionValue& Value)
{
    if (!WeaponComponent || !WeaponComponent->CurrentWeaponData || !WeaponComponent->CurrentWeaponData->bAutoFire)
    {
        return;
    }

    bool IsAmmoWidget = AmmoPopupWidget && !AmmoPopupWidget->IsVisible();
    bool IsAnimalWidget = AnimalPopupWidget && !AnimalPopupWidget->IsVisible();
    bool IsNPCWidget = NPCPopupWidget && !NPCPopupWidget->IsVisible();
    if (IsAmmoWidget && IsAnimalWidget && IsNPCWidget)
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
	if (Value.Get<bool>() )
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
		
		if (AmmoPopupWidget ) { //&& AmmoPopupWidget->IsVisible()
			//0 1 2 3 탄환충전할 거 선택
			AmmoPopupWidget->ChangeAmmo(SlotIndex);
		}
		if(WeaponComponent){
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
			AmmoPopupWidget->RefreshAll();
		}
	}


}

void APotatoPlayerCharacter::OnPauseGame(const FInputActionValue& Value)
{
    APotatoPlayerController* PlayerController = Cast<APotatoPlayerController>(GetController());
    if (!PlayerController && !PauseMenuClass)
    {
        return;
    }

    UPauseMenu* PauseMenu = CreateWidget<UPauseMenu>(GetWorld(), PauseMenuClass);
    if (PauseMenu)
    {
        PlayerController->SetPause(true);
        PauseMenu->AddToViewport();
        PlayerController->SetUIMode(true, PauseMenu);
    }
}

void APotatoPlayerCharacter::OnNextDialogue(const FInputActionValue& Value)
{
	if (OnNextDialoguePressed.IsBound())
	{
		OnNextDialoguePressed.Broadcast();
	}
}


void APotatoPlayerCharacter::OnDeath()
{
	APotatoGameMode* GameMode = Cast<APotatoGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
        // TODO: 사망 애니메이션 추가
        if (DeathMontage)
        {
            PlayAnimMontage(DeathMontage);
        }

		GameMode->EndGame(false, NSLOCTEXT("GameOver", "PlayerDead", "용감한 농부가 쓰러졌습니다..."));
	}
}

float APotatoPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
	if (ActualDamage > 0.0f)
	{
        if (bIsDead) return 0.0f; // 이미 사망한 경우 추가 피해 방지

		CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);
		//UE_LOG(LogTemp, Warning, TEXT("Remaining Health: %f"), CurrentHealth);
		
		if (OnHPChanged.IsBound())
		{
			OnHPChanged.Broadcast(CurrentHP, MaxHP);
		}
		
		if (CurrentHP <= 0.0f)
		{
            bIsDead = true; // 사망 상태로 설정
			OnDeath();
		}
		
		// 피격 사운드 및 애니메이션 재생
		float CurrentTime = GetWorld()->GetTimeSeconds();
		
		if (CurrentTime - LastHitReactionTime >= HitReactionCooldown)
		{
			// 1. 사운드 재생
			if (PainSounds.Num() > 0)
			{
				int32 RandomIndex = FMath::RandRange(0, PainSounds.Num() - 1);
				USoundBase* SelectedSound = PainSounds[RandomIndex];
				
				if (SelectedSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, SelectedSound, GetActorLocation());
				}
			}
			
			// 2. 애니메이션 재생
			if (HitReactMontage)
			{
				PlayAnimMontage(HitReactMontage);
			}
			
			// 3. Camera Shake
			if (HitCameraShakeClass)
			{
				APlayerController* PlayerController = Cast<APlayerController>(GetController());
				if (PlayerController)
				{
					PlayerController->ClientStartCameraShake(HitCameraShakeClass);
				}
			}
			
			LastHitReactionTime = CurrentTime;
		}
	}
	return ActualDamage;
}

void APotatoPlayerCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{

	bool ActorCheck = OtherActor && (OtherActor != this);
	if (ActorCheck && OtherActor->GetName().Contains(TEXT("Barn")))
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("BP_Barn")));
		UPotatoAnimalManagementComp* ManagementComp = OtherActor->FindComponentByClass<UPotatoAnimalManagementComp>();
		if (ManagementComp)
		{
			AnimalPopupWidget->InitPopup(ManagementComp);
		}
		IsBarnMode = true;
	}
	if (ActorCheck && ( OtherActor->GetName().Contains(TEXT("LumberMill"))
	   || OtherActor->GetName().Contains(TEXT("Mine")) )
		)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("Barn")));
		UPotatoNPCManagementComp* ManagementComp = OtherActor->FindComponentByClass<UPotatoNPCManagementComp>();
		if (ManagementComp) {
			NPCPopupWidget->InitPopup(ManagementComp);
		}
		IsNPCMode = true;
	}
}

void APotatoPlayerCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->GetName().Contains(TEXT("Barn")))
	{
		IsBarnMode = false;
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (AnimalPopupWidget && PlayerController)
		{
			if (AnimalPopupWidget->IsVisible()) {
				AnimalPopupWidget->SetVisibility(ESlateVisibility::Hidden);
				PlayerController->bShowMouseCursor = false;
				FInputModeGameOnly InputMode;
				PlayerController->SetInputMode(InputMode);
			}
		}
	}
	if (OtherActor && (OtherActor->GetName().Contains(TEXT("LumberMill"))
		|| OtherActor->GetName().Contains(TEXT("Mine")) ) 
		)
	{
		IsNPCMode = false;
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (NPCPopupWidget && PlayerController)
		{

			if (NPCPopupWidget->IsVisible()) {
				NPCPopupWidget->SetVisibility(ESlateVisibility::Hidden);
				PlayerController->bShowMouseCursor = false;
				FInputModeGameOnly InputMode;
				PlayerController->SetInputMode(InputMode);
			}
		}
	}
}

void APotatoPlayerCharacter::OnBarnMode(const FInputActionValue& Value)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsBarnMode){
		if (AnimalPopupWidget && PlayerController)
		{
			if (AnimalPopupWidget->IsVisible()) {
				AnimalPopupWidget->SetVisibility(ESlateVisibility::Hidden);
				PlayerController->bShowMouseCursor = false;
				FInputModeGameOnly InputMode;
				PlayerController->SetInputMode(InputMode);
			}
			else {
				AnimalPopupWidget->SetVisibility(ESlateVisibility::Visible);
				PlayerController->bShowMouseCursor = true;
				//AnimalPopupWidget->RefreshAll();
			}
		}
	}
	if (IsNPCMode)
	{
		if (NPCPopupWidget && PlayerController)
		{
			if (NPCPopupWidget->IsVisible()) {
				NPCPopupWidget->SetVisibility(ESlateVisibility::Hidden);
				PlayerController->bShowMouseCursor = false;
				FInputModeGameOnly InputMode;
				PlayerController->SetInputMode(InputMode);
			}
			else {
				NPCPopupWidget->SetVisibility(ESlateVisibility::Visible);
				PlayerController->bShowMouseCursor = true;
			}
		}
	}
}