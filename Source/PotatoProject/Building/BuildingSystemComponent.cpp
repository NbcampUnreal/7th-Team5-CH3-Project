#include "BuildingSystemComponent.h"
#include "PotatoStructureData.h"
#include "PotatoPlaceableStructure.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UBuildingSystemComponent::UBuildingSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBuildingSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	SetupInputBindings();
}

void UBuildingSystemComponent::SetupInputBindings()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	
	if (!PlayerController)
	{
		return;
	}
	
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(OwnerPawn->InputComponent))
	{

		if (PlaceStructureAction)
		{
			EnhancedInput->BindAction(PlaceStructureAction, ETriggerEvent::Started, this, &UBuildingSystemComponent::OnPlaceStructure);
		}

		if (RotateStructureAction)
		{
			EnhancedInput->BindAction(RotateStructureAction, ETriggerEvent::Triggered, this, &UBuildingSystemComponent::OnRotateStructure);
		}
		
		if (CycleStructureAction)
		{
			EnhancedInput->BindAction(CycleStructureAction, ETriggerEvent::Started, this, &UBuildingSystemComponent::OnCycleStructure);
		}
	}
}

void UBuildingSystemComponent::ToggleBuildMode()
{
	bIsBuildMode = !bIsBuildMode;
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PlayerController)
	{
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}
	
	if (bIsBuildMode)
	{
		if (BuildIMC)
		{
			Subsystem->AddMappingContext(BuildIMC, 1);
			SetComponentTickEnabled(true);
			CurrentRotationIndex = 0;
			RefreshGhostActorModel();
			
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Build Mode: ON"));
		}
	}
	else
	{
		if (BuildIMC)
		{
			Subsystem->RemoveMappingContext(BuildIMC);
			SetComponentTickEnabled(false);
			
			if (GhostActor)
			{
				GhostActor->Destroy();
				GhostActor = nullptr;
			}
		}
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Build Mode: OFF"));
	}
}

void UBuildingSystemComponent::OnPlaceStructure(const FInputActionValue& Value)
{
}

void UBuildingSystemComponent::OnRotateStructure(const FInputActionValue& Value)
{
	if (!bIsBuildMode)
	{
		return;
	}
	
	float Input = Value.Get<float>();
	
	if (Input > 0)
	{
		CurrentRotationIndex = (CurrentRotationIndex + 1) % 4;
	}
	else
	{
		CurrentRotationIndex = (CurrentRotationIndex - 1 + 4) % 4;
	}
	
	if (GhostActor)
	{
		UpdateGhostActorTransform();
	}
}

void UBuildingSystemComponent::OnCycleStructure(const FInputActionValue& Value)
{
	if (!bIsBuildMode || StructureSlots.Num() == 0)
	{
		return;
	}
	
	float Input = Value.Get<float>();
	
	if (Input > 0)
	{
		CurrentSlotIndex = (CurrentSlotIndex + 1) % StructureSlots.Num();
	}
	else
	{
		CurrentSlotIndex = (CurrentSlotIndex - 1 + StructureSlots.Num()) % StructureSlots.Num();
	}
	
	RefreshGhostActorModel();
}

FVector UBuildingSystemComponent::CalculateSnappedLocation(const FVector& HitLocation) const
{
	return FVector::ZeroVector;
}

void UBuildingSystemComponent::UpdateGhostActorTransform()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner()->GetInstigatorController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}
	
	FVector CamLoc = PlayerController->PlayerCameraManager->GetCameraLocation();
	FVector CamRot = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
	
	FVector TraceEnd = CamLoc + (CamRot * MaxBuildDistance);
	
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(GhostActor);
	
	bool bIsTraceHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, QueryParams);
	
	if (bIsTraceHit)
	{
		FVector TargetLocation = Hit.Location;
		FRotator TargetRotation = FRotator(0.0f, CurrentRotationIndex * 90.0f, 0.0f);
		
		GhostActor->SetActorLocationAndRotation(TargetLocation, TargetRotation);
		GhostActor->SetActorHiddenInGame(false);
	}
	else
	{
		GhostActor->SetActorHiddenInGame(true);
	}
	
}

void UBuildingSystemComponent::RefreshGhostActorModel()
{
	if (GhostActor)
	{
		GhostActor->Destroy();
		GhostActor = nullptr;
	}
	
	const UPotatoStructureData* SelectedData = GetSelectedData();
	
	if (!SelectedData || !SelectedData->StructureClass)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	GhostActor = GetWorld()->SpawnActor<APotatoPlaceableStructure>(SelectedData->StructureClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (GhostActor)
	{
		GhostActor->StructureData = SelectedData;
		GhostActor->SetActorEnableCollision(false);
		UpdateGhostActorTransform();
		// TODO: 머티리얼 변경
		
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("GhostActor %s Created"),*SelectedData->DisplayName.ToString()));
	}

}

const UPotatoStructureData* UBuildingSystemComponent::GetSelectedData() const
{
	if (StructureSlots.IsValidIndex(CurrentSlotIndex))
	{
		return StructureSlots[CurrentSlotIndex];
	}
	return nullptr;
}

void UBuildingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsBuildMode && GhostActor)
	{
		UpdateGhostActorTransform();
	}
}
