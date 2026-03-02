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
#include "Components/CapsuleComponent.h"
#include "Combat/PotatoWeaponData.h"
#include "Core/PotatoDayNightCycle.h"
#include "Core/Interactable.h"

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
    if (GetController() != nullptr)
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
    if (BuildingComponent && BuildingComponent->bIsBuildMode) return;

    float CameraZoomValue = Value.Get<float>();

	if (CameraBoom && CameraZoomValue != 0.0f)
	{
		TargetCameraDistance = FMath::Clamp(TargetCameraDistance + (CameraZoomValue * CameraZoomSpeed),
		                                    MinCameraDistance, MaxCameraDistance);
	}
}

void APotatoPlayerCharacter::Attack(const FInputActionValue& Value)
{
    if (WeaponComponent)
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

    WeaponComponent->Fire();
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
    if (Value.Get<bool>())
    {
        APlayerController* PlayerController = Cast<APlayerController>(GetController());
        if (!PlayerController) return;

        int32 SlotIndex = 0;

        if (PlayerController->IsInputKeyDown(EKeys::One)   || PlayerController->IsInputKeyDown(EKeys::NumPadOne))   SlotIndex = 0;
        if (PlayerController->IsInputKeyDown(EKeys::Two)   || PlayerController->IsInputKeyDown(EKeys::NumPadTwo))   SlotIndex = 1;
        if (PlayerController->IsInputKeyDown(EKeys::Three) || PlayerController->IsInputKeyDown(EKeys::NumPadThree)) SlotIndex = 2;
        if (PlayerController->IsInputKeyDown(EKeys::Four)  || PlayerController->IsInputKeyDown(EKeys::NumPadFour))  SlotIndex = 3;

        if (WeaponComponent)
        {
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
	if (UPotatoDayNightCycle* DNC = GetWorld()->GetSubsystem<UPotatoDayNightCycle>())
	{
		if (DNC->GetCurrentPhase() == EDayPhase::Night)
		{
			if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController()))
			{
				PC->ShowHUDMessage(NSLOCTEXT("HUD", "NightAmmo", "밤에는 탄약을 교환할 수 없습니다"), 1.5f, true);
			}
			return;
		}
	}

	if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController()))
	{
		PC->ToggleAmmoPopup();
	}
}

void APotatoPlayerCharacter::OnPauseGame(const FInputActionValue& Value)
{
    if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController()))
    {
        PC->TogglePauseMenu();
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
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	
	if (DeathMontage)
	{
		float MontageDuration = PlayAnimMontage(DeathMontage) - 0.5f;
		GetWorldTimerManager().SetTimer(
		DeathTimerHandle,
		this,
		&APotatoPlayerCharacter::OnDeathFinished,
		MontageDuration,
		false);
	}
	else
	{
		OnDeathFinished();
	}
}

void APotatoPlayerCharacter::OnDeathFinished()
{
	APotatoGameMode* GameMode = Cast<APotatoGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		GameMode->EndGame(false, NSLOCTEXT("GameOver", "PlayerDead", "용감한 농부가 쓰러졌습니다..."));
	}
}

void APotatoPlayerCharacter::StartRegenCooldown()
{
    GetWorldTimerManager().ClearTimer(RegenDelayTimerHandle);
    GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
    
    if (bIsDead || CurrentHP >= MaxHP)
    {
        return;
    }

    // 반복이 아니라 딜레이만 설정함
    GetWorldTimerManager().SetTimer(RegenDelayTimerHandle, this, &APotatoPlayerCharacter::TickRegen, RegenDelay, false);
}

void APotatoPlayerCharacter::TickRegen()
{
    if (bIsDead || CurrentHP >= MaxHP)
    {
        GetWorldTimerManager().ClearTimer(RegenTickTimerHandle); // 체젠 중지
        return;
    }

    CurrentHP = FMath::Clamp(CurrentHP + RegenHPRate * RegenTickInterval, 0.0f, MaxHP);
    OnHPChanged.Broadcast(CurrentHP, MaxHP);

    // 첫 틱 이후 반복 전환 실시
    if (!GetWorldTimerManager().IsTimerActive(RegenTickTimerHandle))
    {
        // 반복 설정
        GetWorldTimerManager().SetTimer(RegenTickTimerHandle, this, &APotatoPlayerCharacter::TickRegen, RegenTickInterval, true);
    }
}

float APotatoPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
	if (ActualDamage > 0.0f)
	{
        if (bIsDead) return 0.0f; // 이미 사망한 경우 추가 피해 방지

		CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);
        StartRegenCooldown(); // 피해를 입으면 체력 재생 대기 시작
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

void APotatoPlayerCharacter::CloseAllPopups()
{
	if (APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController()))
	{
		PC->CloseAllPopups();
	}
}

void APotatoPlayerCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;

	APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController());
	if (!PC) return;

	TArray<UActorComponent*> Comps = OtherActor->GetComponentsByInterface(UInteractable::StaticClass());
	for (UActorComponent* Comp : Comps)
	{
		if (IInteractable* Interactable = Cast<IInteractable>(Comp))
		{
			Interactable->OnPlayerEnter(PC);
		}
	}
}

void APotatoPlayerCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController());
	if (!PC) return;

	TArray<UActorComponent*> Comps = OtherActor->GetComponentsByInterface(UInteractable::StaticClass());
	for (UActorComponent* Comp : Comps)
	{
		if (IInteractable* Interactable = Cast<IInteractable>(Comp))
		{
			Interactable->OnPlayerExit(PC);
		}
	}
}

void APotatoPlayerCharacter::OnBarnMode(const FInputActionValue& Value)
{
	APotatoPlayerController* PC = Cast<APotatoPlayerController>(GetController());
	if (!PC) return;

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor) continue;
		TArray<UActorComponent*> Comps = Actor->GetComponentsByInterface(UInteractable::StaticClass());
		for (UActorComponent* Comp : Comps)
		{
			if (IInteractable* Interactable = Cast<IInteractable>(Comp))
			{
 				Interactable->Interact(PC);
			}
		}
	}
}