#include "PotatoPlayerCharacter.h"

APotatoPlayerCharacter::APotatoPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APotatoPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APotatoPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotatoPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void APotatoPlayerCharacter::Move(FVector Direction)
{

}

void APotatoPlayerCharacter::Jump()
{

}

void APotatoPlayerCharacter::Sprint()
{

}

void APotatoPlayerCharacter::TakeDamage(float Damage)
{

}

void APotatoPlayerCharacter::Die()
{

}