#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PotatoPlayerCharacter.generated.h"

class UCameraComponent;
class UBuildingSystemComponent;
class USpringArmComponent;
class UPotatoWeaponComponent;

struct FInputActionValue;

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
	
private:
	float TargetCameraDistance;
	
	// Functions
public:
	APotatoPlayerCharacter();
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaTime ) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
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
	};
