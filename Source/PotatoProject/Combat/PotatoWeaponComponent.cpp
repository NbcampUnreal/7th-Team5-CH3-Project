#include "PotatoWeaponComponent.h"
#include "PotatoWeapon.h"
#include "PotatoWeaponData.h"
#include "PotatoProjectile.h"
#include "GameFramework/Character.h"

UPotatoWeaponComponent::UPotatoWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (WeaponSlots.IsValidIndex(0))
	{
		EquipWeapon(0);
	}
}

void UPotatoWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

void UPotatoWeaponComponent::SpawnWeapon(TSubclassOf<APotatoWeapon> NewClass)
{
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !NewClass)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	CurrentWeaponActor = GetWorld()->SpawnActor<APotatoWeapon>(NewClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (CurrentWeaponActor)
	{
		// TODO: 추후 근접 무기 추가시 WeaponData에 Socket Name 추가
		FName AttachSocketName = TEXT("WeaponSocket");
		CurrentWeaponActor->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
	}
}

void UPotatoWeaponComponent::EquipWeapon(int32 SlotIndex)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	UPotatoWeaponData* NewData = WeaponSlots[SlotIndex];
	if (!NewData)
	{
		return;
	}
	
	// 이미 동일한 무기를 들고 있다면 아무것도 하지 않음
	if (CurrentWeaponData == NewData)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("이미 동일한 무기를 장착하고 있습니다!")));
		return;
	}
	
	bool bIsNeedToSpawnNew = true;
	
	if (CurrentWeaponActor && NewData->WeaponActorClass)
	{
		// 액터 클래스가 동일한 경우(e.g. 대포의 탄약 종류를 변경) 액터를 파괴하지 않음
		if (CurrentWeaponActor->GetClass() == NewData->WeaponActorClass)
		{
			bIsNeedToSpawnNew = false;
		}
	}
	
	// 필요한 경우 이전 무기를 파괴
	if (bIsNeedToSpawnNew && CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
	}
	
	// 필요한 경우 새로운 무기를 스폰
	if (bIsNeedToSpawnNew && NewData->WeaponActorClass)
	{
		SpawnWeapon(NewData->WeaponActorClass);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("새로운 무기 외형을 스폰합니다.")));
	}
	
	CurrentWeaponIndex = SlotIndex;
	CurrentWeaponData = NewData;
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("EquipWeapon 성공! 현재 무기: %s, 인덱스 번호: %d"), *NewData->WeaponName.ToString(), SlotIndex ));
}

void UPotatoWeaponComponent::Fire()
{
	if (!CurrentWeaponData || !CurrentWeaponActor)
	{
		return;
	}
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlayerController)
	{
		return;
	}
	
	// 크로스헤어 위치 얻기
	FVector CameraLoc;
	FRotator CameraRot;
	PlayerController->GetPlayerViewPoint(CameraLoc, CameraRot);
	
	FVector TraceStart = CameraLoc;
	FVector TraceEnd = CameraLoc + (CameraRot.Vector() * 10000.0f);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeaponActor);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
	
	// 아무것도 맞히지 못했다면 (e.g. 하늘) 트레이스의 끝 지점
	FVector TargetLocation = bHit ? HitResult.Location : TraceEnd;
	
	// 스폰 위치 및 회전 계산
	FVector MuzzleLocation = GetMuzzleLocation();
	FVector LaunchDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
	FRotator LaunchRotation = LaunchDirection.Rotation();
	
	// =================================================================
	// 투사체 발사 처리
	// =================================================================
	
	if (CurrentWeaponData->FireType == EWeaponFireType::Projectile)
	{
		if (CurrentWeaponData->ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = OwnerCharacter;
			SpawnParams.Instigator = OwnerCharacter;
			
			APotatoProjectile* NewProjectile = GetWorld()->SpawnActor<APotatoProjectile>(
				CurrentWeaponData->ProjectileClass,
				MuzzleLocation,
				LaunchRotation,
				SpawnParams
			);
			
			if (NewProjectile)
			{
				NewProjectile->InitializeProjectile(
					CurrentWeaponData->ProjectileSpeed,
					CurrentWeaponData->ProjectileGravityScale
				);
			}
		}
	}
	// =================================================================
	// 히트스캔 발사 처리
	// =================================================================
	else if (CurrentWeaponData->FireType == EWeaponFireType::Hitscan)
	{
		// TODO: 히트스캔(당근) 로직 구현
		DrawDebugLine(GetWorld(), MuzzleLocation, TargetLocation, FColor::Red, false, 1.0f, 0, 1.0f);
	}
}

FVector UPotatoWeaponComponent::GetMuzzleLocation() const
{
	if (CurrentWeaponActor)
	{
		return CurrentWeaponActor->GetMuzzleLocation();
	}
	return FVector::ZeroVector;
}
