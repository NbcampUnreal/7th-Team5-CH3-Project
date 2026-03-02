#include "SkillTransformResolver.h"
#include "PotatoMonsterSpecialSkillPresetRow.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

FVector FSkillTransformResolver::ResolveOrigin(
	AActor* Owner,
	AActor* Target,
	const FPotatoMonsterSpecialSkillPresetRow& Row
)
{
	if (!IsValid(Owner))
	{
		return FVector::ZeroVector;
	}

	switch (Row.TargetType)
	{
	case EMonsterSpecialTargetType::Self:
		return Owner->GetActorLocation();

	case EMonsterSpecialTargetType::Location:
		if (IsValid(Target))
		{
			return Target->GetActorLocation();
		}
		return Owner->GetActorLocation()
			+ Owner->GetActorForwardVector()
		* FMath::Max(0.f, 0.f);
			/** FMath::Max(0.f, Row.Range);*/

	default:
		return IsValid(Target)
			? Target->GetActorLocation()
			: Owner->GetActorLocation();
	}
}

FTransform FSkillTransformResolver::ResolveSpawnTransform(
	AActor* Owner,
	AActor* Target,
	const FPotatoMonsterSpecialSkillPresetRow& Row
)
{
	if (!IsValid(Owner))
	{
		return FTransform::Identity;
	}

	UWorld* World = Owner->GetWorld();
	FVector Origin = ResolveOrigin(Owner, Target, Row);

	// SnapToGround
	Origin = SnapToGroundIfNeeded(World, Origin, Row.bSnapToGround);

	// Rotation
	FRotator Rot = Owner->GetActorRotation();

	if (Row.bFaceTargetOnSpawn && IsValid(Target))
	{
		Rot = MakeFaceTargetYawOnly(Origin, Target->GetActorLocation(), true);
	}

	return FTransform(Rot, Origin);
}

FVector FSkillTransformResolver::SnapToGroundIfNeeded(
	UWorld* World,
	const FVector& InLocation,
	bool bSnapToGround
)
{
	if (!bSnapToGround || !World)
	{
		return InLocation;
	}

	FHitResult Hit;
	FVector Start = InLocation + FVector(0, 0, 500.f);
	FVector End   = InLocation - FVector(0, 0, 2000.f);

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	if (World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params))
	{
		return Hit.Location;
	}

	return InLocation;
}

FRotator FSkillTransformResolver::MakeFaceTargetYawOnly(
	const FVector& From,
	const FVector& To,
	bool bFaceTarget
)
{
	if (!bFaceTarget)
	{
		return FRotator::ZeroRotator;
	}

	FVector Dir = (To - From);
	Dir.Z = 0.f;
	Dir.Normalize();

	return Dir.Rotation();
}