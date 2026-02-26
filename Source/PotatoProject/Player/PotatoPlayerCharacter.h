#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PotatoPlayerCharacter.generated.h"

class UCameraComponent;
class UBuildingSystemComponent;
class USpringArmComponent;
class UPotatoWeaponComponent;
class UAmmoPopupWidget;
class UAnimalPopup;
class UNPCPopup;
class UPotatoAnimalManagementComp;

struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHPChanged, float, CurrentHP, float, MaxHP);

UCLASS()
class POTATOPROJECT_API APotatoPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	// Components
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBuildingSystemComponent* BuildingComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UPotatoWeaponComponent* WeaponComponent;
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	//UPotatoAnimalManagementComp* AnimalManagementComp;

protected:
	// Camera Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DefaultCameraDistance = 400.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MinCameraDistance = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxCameraDistance = 800.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraZoomSpeed = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraZoomInterpSpeed = 10.0f;
	
	// Walk & Sprint Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float NormalSpeed = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.0f;

	//최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.0f;
	
	//현재 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CurrentHP = 100.0f;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    bool bIsDead = false;

	/** 피격 반응 쿨다운 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float HitReactionCooldown = 2.0f;
	
	// 피격 반응
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Audio")
	TArray<USoundBase*> PainSounds;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	UAnimMontage* DeathMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Camera")
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> AmmoPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> AnimalPopupClass;

	// 생성된 위젯을 참조할 변수
	UPROPERTY()
	UAmmoPopupWidget* AmmoPopupWidget;
	UPROPERTY()
	UAnimalPopup* AnimalPopupWidget;
	//UPROPERTY()
	//UNPCPopup* NPCPopupWidget;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuClass;

    UPROPERTY()
    TObjectPtr<UUserWidget> PauseMenuWidget;

private:
	float TargetCameraDistance;
	bool IsBuildingMode;

	bool IsBarnMode;
	// Functions
	float LastHitReactionTime = -10.0f;
	//bool IsAmmoProduct;
	
	// Functions
public:
	APotatoPlayerCharacter();
	
	void SetIsBuildingMode(bool BuildingMode);

	float GetCurrentHP() const { return CurrentHP; }
	float GetMaxHP() const { return MaxHP; }
	
	// Delegate Instances
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerHPChanged OnHPChanged;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaTime ) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		
	// Input Handlers
	UFUNCTION()
	void Move(const FInputActionValue& Value);
	UFUNCTION()
	void StartJump(const FInputActionValue& Value);
	UFUNCTION()
	void StopJump(const FInputActionValue& Value);
	UFUNCTION()
	void Look(const FInputActionValue& Value);
	UFUNCTION()
	void StartSprint(const FInputActionValue& Value);
	UFUNCTION()
	void StopSprint(const FInputActionValue& Value);
	UFUNCTION()
	void CameraZoom(const FInputActionValue& Value);

	UFUNCTION()
	void Attack(const FInputActionValue& Value);
	UFUNCTION()
	void Reload(const FInputActionValue& Value);
	UFUNCTION()
	void WeaponChange(const FInputActionValue& Value);
	
	UFUNCTION()
	void OnToggleBuildMode(const FInputActionValue& Value);
	UFUNCTION()
	void OnAmmoMode(const FInputActionValue& Value);
	UFUNCTION()
	void OnBarnMode(const FInputActionValue& Value);

    UFUNCTION()
    void OnPauseGame(const FInputActionValue& Value);

	UFUNCTION()
	void OnDeath();
};
