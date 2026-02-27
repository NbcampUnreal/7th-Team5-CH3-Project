#include "BuildingSystemComponent.h"
#include "PotatoStructureData.h"
#include "PotatoPlaceableStructure.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GeometryTypes.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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
			EnhancedInput->BindAction(PlaceStructureAction, ETriggerEvent::Started, this,
			                          &UBuildingSystemComponent::OnPlaceStructure);
		}

		if (RotateStructureAction)
		{
			EnhancedInput->BindAction(RotateStructureAction, ETriggerEvent::Triggered, this,
			                          &UBuildingSystemComponent::OnRotateStructure);
		}

		if (CycleStructureAction)
		{
			EnhancedInput->BindAction(CycleStructureAction, ETriggerEvent::Started, this,
			                          &UBuildingSystemComponent::OnCycleStructure);
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

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		PlayerController->GetLocalPlayer());
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

			// 브로드캐스트
			OnBuildModeToggled.Broadcast(true);
			OnBuildSlotChanged.Broadcast(CurrentSlotIndex, GetSelectedData());
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
			
			// 브로드캐스트
			OnBuildModeToggled.Broadcast(false);
		}

	}
}

void UBuildingSystemComponent::OnPlaceStructure(const FInputActionValue& Value)
{
	if (!GhostActor || !bIsBuildMode)
	{
		return;
	}

	if (!bIsPlacementValid || GhostActor->IsHidden())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("이 장소에는 배치할 수 없습니다!"));
		// TODO: 오류 사운드 재생
		return;
	}

	const UPotatoStructureData* SelectedData = GetSelectedData();
	if (!SelectedData)
	{
		return;
	}

	FTransform SpawnTransform = GhostActor->GetActorTransform(); // 이미 스냅되어 있음
	// Deferred Spawn
	APotatoPlaceableStructure* NewStructure = GetWorld()->SpawnActorDeferred<APotatoPlaceableStructure>(
		SelectedData->StructureClass,
		SpawnTransform,
		GetOwner(),
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (NewStructure)
	{
		NewStructure->StructureData = SelectedData;
		UGameplayStatics::FinishSpawningActor(NewStructure, SpawnTransform);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
		                                 FString::Printf(TEXT("%s 설치 성공!"), *SelectedData->DisplayName.ToString()));

		// TODO: 리소스 차감 및 사운드, 파티클 이펙트 재생
	}
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

	CurrentRotationIndex = 0; // Structure 선택 변경 시 회전 초기화
	RefreshGhostActorModel();
	
	// 브로드캐스트
	OnBuildSlotChanged.Broadcast(CurrentSlotIndex, GetSelectedData());
}

FVector UBuildingSystemComponent::CalculateSnappedLocation(const FVector& HitLocation) const
{
	const UPotatoStructureData* SelectedData = GetSelectedData();
	if (!SelectedData)
	{
		return HitLocation;
	}

	// 1. 회전에 따른 차원 결정
	bool bIsRotated = (CurrentRotationIndex % 2 != 0); // 90도 또는 270도 회전 시(인덱스 1, 3) X,Y 스왑
	int32 SizeX, SizeY;

	if (bIsRotated)
	{
		SizeX = SelectedData->GridSize.Y;
		SizeY = SelectedData->GridSize.X;
	}
	else
	{
		SizeX = SelectedData->GridSize.X;
		SizeY = SelectedData->GridSize.Y;
	}

	float SnappedX, SnappedY;

	// 2. X축 로직
	if (SizeX % 2 == 0) // 짝수일 경우 가장 가까운 그리드 라인에 스냅 (e.g. 0, 50, 100)
	{
		SnappedX = FMath::RoundToFloat(HitLocation.X / GridUnitSize) * GridUnitSize;
	}
	else // 홀수일 경우 타일의 중앙에 스냅 (e.g. 25, 75, 125): 그리드 라인으로 내림 후 절반 크기 더하기
	{
		SnappedX = (FMath::FloorToFloat(HitLocation.X / GridUnitSize) * GridUnitSize) + (GridUnitSize * 0.5f);
	}

	// 3. Y축 로직
	if (SizeY % 2 == 0)
	{
		SnappedY = FMath::RoundToFloat(HitLocation.Y / GridUnitSize) * GridUnitSize;
	}
	else
	{
		SnappedY = (FMath::FloorToFloat(HitLocation.Y / GridUnitSize) * GridUnitSize) + (GridUnitSize * 0.5f);
	}

	return FVector(SnappedX, SnappedY, HitLocation.Z);
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
	bool bIsFloor = bIsTraceHit && (Hit.ImpactNormal.Z > 0.9f);

	if (bIsFloor)
	{
		GhostActor->SetActorHiddenInGame(false);

		// 스냅
		FVector SnappedLocation = CalculateSnappedLocation(Hit.Location);
		FRotator SnappedRotation = FRotator(0, CurrentRotationIndex * 90.0f, 0);
		GhostActor->SetActorLocationAndRotation(SnappedLocation, SnappedRotation);

		// 유효성 검사
		const UPotatoStructureData* SelectedData = GetSelectedData();
		bIsPlacementValid = CheckPlacementValidity(SnappedLocation, SnappedRotation, SelectedData);

		// 디버그: 그리드 시각화
		DrawOccupiedGrid(SnappedLocation, SnappedRotation, SelectedData);

		// 녹색 또는 빨간색
		UpdateGhostActorMaterials();
	}
	else
	{
		GhostActor->SetActorHiddenInGame(true);
		bIsPlacementValid = false;
	}
}

void UBuildingSystemComponent::UpdateGhostActorMaterials()
{
	if (!GhostActor)
	{
		return;
	}

	UMaterialInterface* NewMaterial = bIsPlacementValid ? GhostValidMaterial : GhostInvalidMaterial;

	if (!NewMaterial)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	GhostActor->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* Mesh : MeshComponents)
	{
		int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			Mesh->SetMaterial(i, NewMaterial);
		}
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
	
	FTransform SpawnTransform = FTransform::Identity;
	
	GhostActor = GetWorld()->SpawnActorDeferred<APotatoPlaceableStructure>(
		SelectedData->StructureClass,
		SpawnTransform,
		GetOwner(),
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (GhostActor)
	{
		GhostActor->StructureData = SelectedData;
		GhostActor->SetActorEnableCollision(false);
		UGameplayStatics::FinishSpawningActor(GhostActor, SpawnTransform);
		UpdateGhostActorTransform();
	}
}

void UBuildingSystemComponent::DrawOccupiedGrid(const FVector& Location, const FRotator& Rotation,
                                                const UPotatoStructureData* Data)
{
	if (!bShowOccupiedGrid || !Data)
	{
		return;
	}

	FVector BoxExtent;
	BoxExtent.X = (Data->GridSize.X * GridUnitSize) * 0.5f;
	BoxExtent.Y = (Data->GridSize.Y * GridUnitSize) * 0.5f;
	BoxExtent.Z = 1.0f;

	FVector DebugCenter = Location + FVector(0.0f, 0.0f, 2.0f);

	DrawDebugSolidBox(
		GetWorld(),
		DebugCenter,
		BoxExtent,
		Rotation.Quaternion(),
		FColor::White,
		false,
		-1.0f,
		0);
}

const UPotatoStructureData* UBuildingSystemComponent::GetSelectedData() const
{
	if (StructureSlots.IsValidIndex(CurrentSlotIndex))
	{
		return StructureSlots[CurrentSlotIndex];
	}
	return nullptr;
}

bool UBuildingSystemComponent::CheckPlacementValidity(const FVector& Location, const FRotator& Rotation,
                                                      const UPotatoStructureData* Data)
{
	if (!Data)
	{
		return false;
	}

	FVector BoxExtent;
	BoxExtent.X = (Data->GridSize.X * GridUnitSize) * 0.5f - 2.0f;
	BoxExtent.Y = (Data->GridSize.Y * GridUnitSize) * 0.5f - 2.0f;
	BoxExtent.Z = 100.0f;

	//회전 처리
	if (CurrentRotationIndex % 2 != 0)
	{
		float Temp = BoxExtent.X;
		BoxExtent.X = BoxExtent.Y;
		BoxExtent.Y = Temp;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GhostActor);

	// =================================================================
	// 1. 오버랩 체크: 배치할 위치에 다른 오브젝트가 있는지 검사
	// =================================================================

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	// 필요 시 추가

	FVector OverlapCenter = Location + FVector(0, 0, BoxExtent.Z + 20.0f);

	bool bHit = GetWorld()->OverlapAnyTestByObjectType(
		OverlapCenter,
		FQuat(Rotation),
		ObjectParams,
		FCollisionShape::MakeBox(BoxExtent),
		QueryParams
	);

	if (bHit)
	{
		return false;
	}

	// =================================================================
	// 2. 지면 유효성 체크
	// =================================================================

	FVector Center = Location;
	float X = BoxExtent.X;
	float Y = BoxExtent.Y;

	TArray<FVector> CheckPoints;
	CheckPoints.Add(Center + Rotation.RotateVector(FVector(X, Y, 20.0f)));
	CheckPoints.Add(Center + Rotation.RotateVector(FVector(X, -Y, 20.0f)));
	CheckPoints.Add(Center + Rotation.RotateVector(FVector(-X, Y, 20.0f)));
	CheckPoints.Add(Center + Rotation.RotateVector(FVector(-X, -Y, 20.0f)));

	for (const FVector& Point : CheckPoints)
	{
		FHitResult GroundHit;

		bool bGroundFound = GetWorld()->LineTraceSingleByObjectType(
			GroundHit,
			Point,
			Point - FVector(0, 0, 100.0f),
			FCollisionObjectQueryParams(ECC_WorldStatic),
			QueryParams
		);
		if (!bGroundFound)
		{
			return false;
		}
	}

	return true;
}

void UBuildingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsBuildMode && GhostActor)
	{
		UpdateGhostActorTransform();
	}
}
