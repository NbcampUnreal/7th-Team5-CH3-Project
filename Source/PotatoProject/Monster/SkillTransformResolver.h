#pragma once

#include "CoreMinimal.h"

struct FPotatoMonsterSpecialSkillPresetRow;

/**
 * Skill Transform Resolver
 * - Row 기반 Origin 계산
 * - Spawn Transform 계산
 * - SnapToGround / FaceTarget 처리
 *
 * Presentation / Execution 공용
 */
struct POTATOPROJECT_API FSkillTransformResolver
{
	// =========================
	// Origin 계산
	// =========================
	static FVector ResolveOrigin(
		AActor* Owner,
		AActor* Target,
		const FPotatoMonsterSpecialSkillPresetRow& Row
	);

	// =========================
	// Spawn Transform 계산
	// =========================
	static FTransform ResolveSpawnTransform(
		AActor* Owner,
		AActor* Target,
		const FPotatoMonsterSpecialSkillPresetRow& Row
	);

private:
	static FVector SnapToGroundIfNeeded(
		UWorld* World,
		const FVector& InLocation,
		bool bSnapToGround
	);

	static FRotator MakeFaceTargetYawOnly(
		const FVector& From,
		const FVector& To,
		bool bFaceTarget
	);
};