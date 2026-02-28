#pragma once

#include "CoreMinimal.h"

class AActor;
class UPrimitiveComponent;

class APotatoMonster;

// 1) Target에서 충돌 가능한 Primitive 하나를 찾는다(안정화)
UPrimitiveComponent* FindFirstCollisionPrimitive(AActor* Target);

// 2) Target bounds를 안전하게 얻는다 (Extent 0 fallback 포함)
void GetTargetBoundsSafe(AActor* Target, FVector& OutOrigin, FVector& OutExtent);

// 3) Target에서 From 기준 가장 가까운 점(2D)을 얻는다(충돌->bounds fallback)
bool GetClosestPoint2DOnTarget(AActor* Target, const FVector& From, FVector& OutClosest2D);

// 4) Monster가 Target에 접근해야 할 "접근 지점"(2D) 계산(충돌->bounds fallback)
bool ComputeApproachPoint2D(APotatoMonster* M, AActor* Target, float ExtraOffset, FVector& OutPoint);