#include "PotatoWeapon.h"
#include "PotatoProjectile.h"
#include "PotatoWeaponSystem.h"
#include "../Monster/PotatoMonster.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"


APotatoWeapon::APotatoWeapon()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void APotatoWeapon::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* GI = GetGameInstance())
	{

		WeaponSystem = GI->GetSubsystem<UPotatoWeaponSystem>();
		//WeaponSystem->Fire();
	}

	//CurrentAmmo[0] = 30;
	//CurrentAmmo[1] = 30;
	//CurrentAmmo[2] = 10;
	//CurrentAmmo[3] = 50;
	ChangeWeapon(1);
}

float FireAccTime = 0.0f; // 누적 시간 계산용
float FireInterval = 2.0f; // 2초마다 발사

void APotatoWeapon::Tick(float DeltaTime)
{
	//Super::Tick(DeltaTime);
	//FireAccTime += DeltaTime;
	//
	//if (FireAccTime >= FireInterval)
	//{
	//	if (WeaponSystem) {
	//	
	//		WeaponSystem->EquipWeapon(0);
	//		UE_LOG(LogTemp, Log, TEXT("UPotatoWeaponSystem::Fire() 호출됨!"));
	//		WeaponSystem->Fire();

	//	}
	//	// 시간 초기화 (남은 시간을 유지하려면 FireAccTime -= FireInterval;)
	//	FireAccTime = 0.0f;
	//}
}


void APotatoWeapon::Fire()
{
	int Weaponindex = (int)Type;
	if (CurrentAmmo[Weaponindex] == 0) {
		//UE_LOG(LogTemp, Warning, TEXT("탄환: %d / %d"), CurrentAmmo[Weaponindex], MagazineSize);
		return;
	}
	else {
		CurrentAmmo[Weaponindex]--;
		//UE_LOG(LogTemp, Warning, TEXT("탄환: %d / %d"), CurrentAmmo[Weaponindex], MagazineSize);
	
	}
		if (WeaponSystem)
		{
				FTransform WeaponTransform = GetActorTransform();
				FVector RelativeOffset = GetActorLocation().ForwardVector * +90.0f+ GetActorLocation().UpVector* Fireangle;
				FVector SpawnLocation = WeaponTransform.TransformPosition(RelativeOffset);
				FRotator SpawnRotation = FRotator::ZeroRotator;
				FVector Direction = SpawnLocation - GetActorLocation();
			if (Type != EWeaponType::Carrot) {
				FActorSpawnParameters SpawnParams;
				APotatoProjectile* NewProjectile = GetWorld()->SpawnActor<APotatoProjectile>(
					ProjectileOrigins[(int)Type],
					SpawnLocation,
					SpawnRotation,
					SpawnParams
				);
				WeaponSystem->ProjectileLimit.Add(NewProjectile);
				//UE_LOG(LogTemp, Log, TEXT("Direction %s"), *Direction.ToString());
				NewProjectile->Launch(Direction);
				WeaponSystem->LimitBullets();
			}
			else {
				FHitResult HitResult;
				FVector End = SpawnLocation + Direction * ValidDistance;//유효사거리
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(this);
				bool bHit = GetWorld()->LineTraceSingleByChannel(
					HitResult,
					SpawnLocation,
					End,
					ECC_Visibility,
					Params
				);
				//UE_LOG(LogTemp, Warning, TEXT("맞냐?"));
				if (bHit && HitResult.GetActor())
				{
					// 1. 부딪힌 액터 가져오기
					AActor* HitActor = HitResult.GetActor();
					FString a = HitActor->GetFName().ToString();
					//UE_LOG(LogTemp, Warning, TEXT("뭔가 맞았다! %s"),*a);

					// 3. 특정 클래스인지 확인 (Cast 이용)
					APotatoMonster* Monster = Cast<APotatoMonster>(HitActor);
					if (Monster)
					{
						//static mesh에만 맞는듯...?
						UGameplayStatics::ApplyDamage(
							Monster,
							Damage,
							GetInstigatorController(),
							this,
							UDamageType::StaticClass()
						);
						//UE_LOG(LogTemp, Warning, TEXT("몬스터 맞았다! %f"), Damage);
					}
				}
				else {
				
				//UE_LOG(LogTemp, Warning, TEXT("안 맞음 ? "));
				}
			}
		}	
	
}

bool APotatoWeapon::Reload()
{
	int Weaponindex = (int)Type;
	if (MagazineSize == CurrentAmmo[Weaponindex])
	{
		//UE_LOG(LogTemp, Warning, TEXT("탄환: %d / %d"), CurrentAmmo[Weaponindex], MagazineSize);
		return false;
	}
	else {
		CurrentAmmo[Weaponindex] = MagazineSize;
		//UE_LOG(LogTemp, Warning, TEXT("탄환: %d / %d"), CurrentAmmo[Weaponindex], MagazineSize);
		return true;
	}
}

bool APotatoWeapon::CanFire()
{
	return false;
}

void APotatoWeapon::ChangeWeapon(int index)
{
	int typeindex = index - 1;
	Type = EWeaponType(typeindex);
	if (index != 4) {
		APotatoProjectile* DefaultProjectile = ProjectileOrigins[typeindex]->GetDefaultObject <APotatoProjectile>();
		SetProjectileType(DefaultProjectile);
	}
	switch (typeindex)
	{
	case 0:
		Damage = 10;
		MagazineSize = 30;
		//CurrentAmmo = 30;
		CropCostPerShot = 1;
		FireRate = 0.5f;
		Fireangle = 10.0f;
		break;
	case 1:
		Damage = 8;
		MagazineSize = 30;
		//CurrentAmmo = 30;
		CropCostPerShot = 1;
		FireRate = 0.5f;
		Fireangle = 10.0f;
		break;
	case 2:
		Damage = 30;
		MagazineSize = 10;
		//CurrentAmmo = 30;
		CropCostPerShot = 3;
		FireRate = 1.0f;
		Fireangle = 10.0f;
		break;
	case 3:
		Damage = 5;
		MagazineSize = 50;
		//CurrentAmmo = 50;
		CropCostPerShot = 1;
		FireRate = 0;
		break;
	}
}

void APotatoWeapon::SetProjectileType(APotatoProjectile* aProjectile)
{
	Damage = aProjectile->Damage;
}