#include "PotatoWeaponComponent.h"
#include "PotatoWeapon.h"
#include "PotatoWeaponData.h"
#include "PotatoProjectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UPotatoWeaponComponent::UPotatoWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPotatoWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeAmmoMap();

	if (WeaponSlots.IsValidIndex(0))
	{
		EquipWeapon(0);
	}
}

void UPotatoWeaponComponent::AddAmmoToWeapon(UPotatoWeaponData* TargetWeapon, int32 Amount)
{
	if (!TargetWeapon || !AmmoMap.Contains(TargetWeapon))
	{
		return;
	}

	FWeaponAmmoState& State = AmmoMap[TargetWeapon];
	State.ReserveAmmo += Amount;
}

void UPotatoWeaponComponent::InitializeAmmoMap()
{
	for (UPotatoWeaponData* Data : WeaponSlots)
	{
		if (Data && !AmmoMap.Contains(Data))
		{
			FWeaponAmmoState NewState;
			NewState.CurrentAmmo = Data->MaxAmmoSize; // 탄약 가득 채우고 시작
			NewState.ReserveAmmo = 100; // 테스트를 위한 예비 탄약 100개
			AmmoMap.Add(Data, NewState);
		}
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

void UPotatoWeaponComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// ControlRotation Recoil
	if (!FMath::IsNearlyZero(TargetRecoilPitch) || !FMath::IsNearlyZero(TargetRecoilYaw))
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter && OwnerCharacter->GetController())
		{
			float PitchStep = FMath::FInterpTo(0.0f, TargetRecoilPitch, DeltaTime, CurrentWeaponData->RecoilSpeed);
			float YawStep = FMath::FInterpTo(0.0f, TargetRecoilYaw, DeltaTime, CurrentWeaponData->RecoilSpeed);
			
			OwnerCharacter->AddControllerPitchInput(-PitchStep);
			OwnerCharacter->AddControllerYawInput(-YawStep);
			
			TargetRecoilPitch -= PitchStep;
			TargetRecoilYaw -= YawStep;
		}
	}
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

	CurrentWeaponActor = GetWorld()->SpawnActor<APotatoWeapon>(NewClass, FVector::ZeroVector, FRotator::ZeroRotator,
	                                                           SpawnParams);

	if (CurrentWeaponActor)
	{
		// TODO: 추후 근접 무기 추가시 WeaponData에 Socket Name 추가
		FName AttachSocketName = TEXT("WeaponSocket");
		CurrentWeaponActor->AttachToComponent(OwnerCharacter->GetMesh(),
		                                      FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
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

	// 무기를 교체하는 경우 재장전 취소
	CancelReload();

	// 이미 동일한 무기를 들고 있다면 아무것도 하지 않음
	if (CurrentWeaponData == NewData)
	{
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

	// 필요한 경우 이전 무기를 파괴하고 새로운 무기를 스폰
	if (bIsNeedToSpawnNew && CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
	}

	if (bIsNeedToSpawnNew && NewData->WeaponActorClass)
	{
		SpawnWeapon(NewData->WeaponActorClass);
	}

	CurrentWeaponIndex = SlotIndex;
	CurrentWeaponData = NewData;
	
	// 무기 변경 브로드캐스트
	if (OnWeaponChanged.IsBound())
	{
		OnWeaponChanged.Broadcast(CurrentWeaponData);
	}
	
	// 새 무기에 대한 탄약 브로드캐스트
	BroadcastAmmoState();
	
	// 사운드 및 애니메이션 재생
	if (CurrentWeaponData->EquipSound)
	{
		float RandomPitch = GetRandomPitch(CurrentWeaponData->EquipSoundPitchRandomness);
		UGameplayStatics::PlaySoundAtLocation(this, CurrentWeaponData->EquipSound, GetMuzzleLocation(), 1.0f, RandomPitch);
	}
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}
	
	if (CurrentWeaponData->EquipMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentWeaponData->EquipMontage);
	}
		
	LastFireTime = GetWorld()->GetTimeSeconds(); // Equip 직후 타임스탬프 업데이트
}

bool UPotatoWeaponComponent::CanFire() const
{
	// 유효성 검사
	if (!CurrentWeaponData || !CurrentWeaponActor)
	{
		return false;
	}

	// 상태 확인
	if (CurrentState != EWeaponState::Idle)
	{
		return false;
	}

	// 발사 속도 쿨다운: 타임스탬프 확인
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < CurrentWeaponData->FireRate)
	{
		return false;
	}
	return true;
}

bool UPotatoWeaponComponent::IsReloading() const
{
	return CurrentState == EWeaponState::Reloading;
}

bool UPotatoWeaponComponent::IsInCombatStance() const
{
	return (GetWorld()->GetTimeSeconds() - LastFireTime) < 3.0f;
}

float UPotatoWeaponComponent::GetLastFireTime() const
{
	return LastFireTime;
}

void UPotatoWeaponComponent::Fire()
{
	if (!CanFire())
	{
		return;
	}
	
	// =================================================================
	// 탄약 로직
	// =================================================================

	if (!AmmoMap.Contains(CurrentWeaponData))
	{
		return;
	}

	FWeaponAmmoState& State = AmmoMap[CurrentWeaponData];

	if (State.CurrentAmmo <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("장전 된 탄약이 없습니다!"));
		return;
	}

	State.CurrentAmmo--;
	BroadcastAmmoState();
	LastFireTime = GetWorld()->GetTimeSeconds(); // 발사 직후 타임스탬프 업데이트

	// =================================================================
	// Game Feel
	// =================================================================

	PlayFireEffects();

	// =================================================================
	// 실제 발사 로직
	// =================================================================

	FVector TargetLocation = GetCrosshairTargetLocation();

	switch (CurrentWeaponData->FireType)
	{
	case EWeaponFireType::Projectile:
		//FireProjectileWith(TargetLocation);
        FireProjectileWithBallistics(TargetLocation);
		break;
	case EWeaponFireType::Hitscan:
		FireHitscan(TargetLocation);
		break;
	}
}

void UPotatoWeaponComponent::StartReload()
{
	if (!CurrentWeaponData || !CurrentWeaponActor)
	{
		return;
	}

	if (CurrentState != EWeaponState::Idle)
	{
		return;
	}

	FWeaponAmmoState& State = AmmoMap[CurrentWeaponData];

	// 이미 가득 차 있는지 확인
	if (State.CurrentAmmo >= CurrentWeaponData->MaxAmmoSize)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("탄창이 꽉 찼습니다!"));
		return;
	}

	// 예비 탄약이 있는지 확인
	if (State.ReserveAmmo <= 0)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("예비 탄약이 없습니다!"));
		return;
	}

	CurrentState = EWeaponState::Reloading;
	// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Reloading...(이동 속도 감소)"));
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}
	
	// 이동 속도 처리
	if (OwnerCharacter->GetCharacterMovement())
	{
		CachedWalkSpeed = OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed;
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = CachedWalkSpeed * ReloadWalkSpeedScale;
	}

	float Duration = CurrentWeaponData->ReloadTime;

	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPotatoWeaponComponent::FinishReload,
		Duration,
		false
	);
	
	if (CurrentWeaponData->ReloadMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentWeaponData->ReloadMontage);
	}
	
	if (CurrentWeaponData->ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CurrentWeaponData->ReloadSound, GetMuzzleLocation());
	}
}

void UPotatoWeaponComponent::FinishReload()
{
	if (!CurrentWeaponData || !AmmoMap.Contains(CurrentWeaponData))
	{
		CurrentState = EWeaponState::Idle;
		return;
	}

	FWeaponAmmoState& State = AmmoMap[CurrentWeaponData];

	int32 AmmoNeeded = CurrentWeaponData->MaxAmmoSize - State.CurrentAmmo;
	int32 AmmoToTransfer = FMath::Min(AmmoNeeded, State.ReserveAmmo);

	State.CurrentAmmo += AmmoToTransfer;
	State.ReserveAmmo -= AmmoToTransfer;
	
	BroadcastAmmoState();
	CancelReload();
}

void UPotatoWeaponComponent::CancelReload()
{
	// 1. 타이머 클리어
	if (GetWorld()->GetTimerManager().IsTimerActive(ReloadTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	// 2. 이동 속도 복원
	if (CurrentState == EWeaponState::Reloading)
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
		{
			OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = CachedWalkSpeed;
		}
	}

	// 3. 상태 재설정
	CurrentState = EWeaponState::Idle;
}

FVector UPotatoWeaponComponent::GetMuzzleLocation() const
{
	if (CurrentWeaponActor)
	{
		return CurrentWeaponActor->GetMuzzleLocation();
	}
	return FVector::ZeroVector;
}

FVector UPotatoWeaponComponent::GetCrosshairTargetLocation() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return FVector::ZeroVector;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlayerController)
	{
		return FVector::ZeroVector;
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
	if (bHit)
	{
		return HitResult.Location;
	}

	return TraceEnd;
}

void UPotatoWeaponComponent::UpdateCachedWalkSpeed(float NewSpeed)
{
	CachedWalkSpeed = NewSpeed;
}

void UPotatoWeaponComponent::BroadcastAmmoState()
{
	if (CurrentWeaponData && AmmoMap.Contains(CurrentWeaponData))
	{
		const FWeaponAmmoState& State = AmmoMap[CurrentWeaponData];
		if (OnAmmoChanged.IsBound())
		{
			OnAmmoChanged.Broadcast(State.CurrentAmmo, State.ReserveAmmo);
		}
	}
}

void UPotatoWeaponComponent::FireProjectile(const FVector& TargetLocation)
{
	if (!CurrentWeaponData->ProjectileClass)
	{
		return;
	}
	// 스폰 위치 및 회전 계산
	FVector MuzzleLocation = GetMuzzleLocation();
	FVector LaunchDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
	FRotator LaunchRotation = LaunchDirection.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());

	APotatoProjectile* NewProjectile = GetWorld()->SpawnActor<APotatoProjectile>(
		CurrentWeaponData->ProjectileClass,
		MuzzleLocation,
		LaunchRotation,
		SpawnParams
	);

	if (NewProjectile)
	{
		NewProjectile->InitializeProjectile(CurrentWeaponData);
	}
}

void UPotatoWeaponComponent::FireProjectileWithBallistics(const FVector& TargetLocation)
{
    if (!CurrentWeaponData->ProjectileClass)
    {
        return;
    }

    /** SuggestProjecttileVelocity로 포물선 발사각 보정을 수행하는 로직 */
    FVector SuggestedVelocity = FVector::ZeroVector;
    FVector MuzzleLocation = GetMuzzleLocation();

    // 사거리 초과 시 목표를 최대 사거리 지점으로 클램핑 수행
    FVector ActualTargetLocation = TargetLocation;
    if (CurrentWeaponData->ProjectileMaxRange > 0.0f)
    {
        FVector ToTarget = TargetLocation - MuzzleLocation;
        if (ToTarget.Size() > CurrentWeaponData->ProjectileMaxRange)
        {
            ActualTargetLocation = MuzzleLocation + ToTarget.GetSafeNormal() * CurrentWeaponData->ProjectileMaxRange;
        }
    }
    
    float GravityZ = GetWorld()->GetGravityZ() * CurrentWeaponData->ProjectileGravityScale;

    bool bSuccess = UGameplayStatics::SuggestProjectileVelocity(
        this,
        SuggestedVelocity,
        MuzzleLocation,                      // 발사 위치: 총구 기준
        ActualTargetLocation,
        CurrentWeaponData->ProjectileSpeed,
        false,
        0.0f,
        GravityZ,
        ESuggestProjVelocityTraceOption::DoNotTrace
    );

    FRotator LaunchRotation = FRotator::ZeroRotator;

    float DebugLifeTime = 2.0f;
    FColor DebugColor = bSuccess ? FColor::Green : FColor::Red;

    DrawDebugLine(GetWorld(), MuzzleLocation, ActualTargetLocation, DebugColor, false, DebugLifeTime, 0, 1.5f);

    if (bSuccess)
    {
        LaunchRotation = SuggestedVelocity.GetSafeNormal().Rotation();
    }
    else
    {
        FVector FlatDirection = (ActualTargetLocation - MuzzleLocation);
        
        float ProjectileCompensation = FMath::Clamp(FlatDirection.Size(), 0.0f, 1.0f) * 0.25;
        FVector LaunchDirection = FlatDirection.GetSafeNormal();
        LaunchDirection.Z += ProjectileCompensation;
        LaunchRotation = LaunchDirection.Rotation();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = Cast<APawn>(GetOwner());

    APotatoProjectile* NewProjectile = GetWorld()->SpawnActor<APotatoProjectile>(
        CurrentWeaponData->ProjectileClass,
        MuzzleLocation,
        LaunchRotation,
        SpawnParams
    );

    if (NewProjectile)
    {
        NewProjectile->InitializeProjectile(CurrentWeaponData);
    }
}

void UPotatoWeaponComponent::FireHitscan(const FVector& TargetLocation)
{
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

	FVector MuzzleLocation = GetMuzzleLocation();
	// Muzzle에서 Target까지의 기본 방향 계산 (시각적 정확성을 위해 카메라에서 Target까지의 방향이 아님!)
	FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeaponActor);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	for (int32 i = 0; i < CurrentWeaponData->PelletCount; ++i)
	{
		FVector SpreadDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(
			BaseDirection, CurrentWeaponData->SpreadAngle);
		FVector TraceEnd = MuzzleLocation + (SpreadDirection * CurrentWeaponData->EffectiveRange);

		FHitResult HitResult;

		bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, MuzzleLocation, TraceEnd, ObjectQueryParams,
		                                                    QueryParams);

		if (bHit)
		{
			AActor* HitActor = HitResult.GetActor();

			if (HitActor && HitActor->ActorHasTag(TEXT("Monster")))
			{
				UGameplayStatics::ApplyDamage(HitActor, CurrentWeaponData->BaseDamage, PlayerController,
				                              CurrentWeaponActor, UDamageType::StaticClass());
			}
			SpawnHitscanVisual(HitResult, SpreadDirection);
		}
	}
}

void UPotatoWeaponComponent::SpawnHitscanVisual(const FHitResult& HitResult, const FVector& ShotDirection)
{
	if (!CurrentWeaponData->HitscanActorClass)
	{
		return;
	}

	FRotator VisualRotation = ShotDirection.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* VisualActor = GetWorld()->SpawnActor<AActor>(
		CurrentWeaponData->HitscanActorClass,
		HitResult.Location,
		VisualRotation,
		SpawnParams
	);

	if (VisualActor)
	{
		VisualActor->AttachToComponent(HitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform);
		VisualActor->SetLifeSpan(10.0f);
	}
	
	if (!CurrentWeaponData)
	{
		return;
	}
	
	if (CurrentWeaponData->ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			CurrentWeaponData->ImpactEffect,
			HitResult.Location,
			FRotator::ZeroRotator,
			CurrentWeaponData->ImpactEffectScale,
			true,
			true);
	}
	
	if (CurrentWeaponData->ImpactSound)
	{
		float RandomPitch = GetRandomPitch(CurrentWeaponData->FireSoundPitchRandomness);
		UGameplayStatics::PlaySoundAtLocation(this, CurrentWeaponData->ImpactSound, HitResult.Location, 1.0f, RandomPitch);
	}
}

void UPotatoWeaponComponent::PlayFireEffects()
{
	if (!CurrentWeaponData || !CurrentWeaponActor)
	{
		return;
	}

	// 1. 사운드 재생 (랜덤 피치 적용)
	if (CurrentWeaponData->FireSound)
	{
		float RandomPitch = GetRandomPitch(CurrentWeaponData->FireSoundPitchRandomness);
		UGameplayStatics::PlaySoundAtLocation(this, CurrentWeaponData->FireSound, GetMuzzleLocation(), 1.0f,
		                                      RandomPitch);
	}

	// 2. 총구 이펙트 재생
	if (CurrentWeaponData->MuzzleFlash)
	{
		// 무기 메쉬의 Muzzle 소켓에 부착
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			CurrentWeaponData->MuzzleFlash,
			CurrentWeaponActor->WeaponMesh,
			TEXT("Muzzle"),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
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

	// 3. Camera Shake
	if (CurrentWeaponData->FireCameraShake)
	{
		PlayerController->ClientStartCameraShake(CurrentWeaponData->FireCameraShake);
	}

	
	if (CurrentWeaponData && CurrentWeaponActor)
	{
		// 4. ControlRotation Recoil: 반동 누적, 연사 시 계속 위로 올라가도록
		TargetRecoilPitch += CurrentWeaponData->RecoilPitch;
		TargetRecoilYaw += FMath::RandRange(-CurrentWeaponData->RecoilYaw, CurrentWeaponData->RecoilYaw);
		
		// 5. Procedural Weapon Kick (무기 모델 반동)
		CurrentWeaponActor->PlayKick(CurrentWeaponData->WeaponKickOffset, CurrentWeaponData->WeaponKickRotation,
								 CurrentWeaponData->WeaponKickRecoverySpeed);
	}
	
	if (CurrentWeaponData->FireMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentWeaponData->FireMontage);
	}
	
}

float UPotatoWeaponComponent::GetRandomPitch(float SoundPitchRandomness)
{
	float RandomPitch = 1.0f;
	
	if (SoundPitchRandomness > 0.0f)
	{
		float MinPitch = 1.0f - SoundPitchRandomness;
		float MaxPitch = 1.0f + SoundPitchRandomness;
		return RandomPitch = FMath::RandRange(MinPitch, MaxPitch);
	}
	
	return RandomPitch;
}
