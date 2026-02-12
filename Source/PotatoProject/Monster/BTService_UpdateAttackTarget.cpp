#include "BTService_UpdateAttackTarget.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Components/PrimitiveComponent.h"
#include "PotatoMonster.h"

UBTService_UpdateAttackTarget::UBTService_UpdateAttackTarget()
{
	NodeName = TEXT("Update AttackTarget (Warehouse)");
	Interval = 0.2f;

	AttackTargetKey.SelectedKeyName = TEXT("CurrentTarget");
	InAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");

	CollisionCheckPadding = 200.f;
}

uint16 UBTService_UpdateAttackTarget::GetInstanceMemorySize() const
{
	return sizeof(FUpdateAttackTargetMemory);
}

static UPrimitiveComponent* FindFirstCollisionPrimitive(AActor* Target)
{
	if (!Target) return nullptr;

	// 1) 루트가 Primitive면 우선
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		if (RootPrim->IsCollisionEnabled()) return RootPrim;
	}

	// 2) 아니면 컴포넌트 중 Collision enabled인 Primitive 하나 찾기
	TArray<UPrimitiveComponent*> Prims;
	Target->GetComponents<UPrimitiveComponent>(Prims);

	for (UPrimitiveComponent* C : Prims)
	{
		if (!C) continue;
		if (!C->IsCollisionEnabled()) continue;
		return C;
	}

	return nullptr;
}

void UBTService_UpdateAttackTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC) return;

	APotatoMonster* M = Cast<APotatoMonster>(AIC->GetPawn());
	if (!M || M->bIsDead) return;

	AActor* Target = M->WarehouseActor.Get();
	BB->SetValueAsObject(AttackTargetKey.SelectedKeyName, Target);

	bool bInRange = false;

	// Target 없으면 false 유지
	if (!Target)
	{
		BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		return;
	}

	const float Range = M->FinalStats.AttackRange;
	const FVector From = M->GetActorLocation();

	// 1) 바운즈 기반 "표면 근사 거리" 계산 (매우 저렴)
	FVector TargetOrigin, TargetExt;
	Target->GetActorBounds(true, TargetOrigin, TargetExt);
	const float TargetExtent2D = FVector2D(TargetExt.X, TargetExt.Y).Size();

	FVector OwnerOrigin, OwnerExt;
	M->GetActorBounds(true, OwnerOrigin, OwnerExt);
	const float OwnerExtent2D = FVector2D(OwnerExt.X, OwnerExt.Y).Size();

	const float PivotDist2D = FVector::Dist2D(From, Target->GetActorLocation());

	// 표면까지 근사 거리(0보다 작으면 이미 겹치거나 붙어있는 상태로 취급)
	const float ApproxSurfaceDist2D = FMath::Max(0.f, PivotDist2D - TargetExtent2D - OwnerExtent2D);

	// 2) 1차 컷: 근사 거리로 컷 (피벗 컷 금지!)
	const float Threshold = Range + FMath::Max(0.f, CollisionCheckPadding);
	if (ApproxSurfaceDist2D > Threshold)
	{
		BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		return;
	}

	// 3) 가능하면 콜리전 기반 정확 거리(가까울 때만)
	float FinalDist2D = ApproxSurfaceDist2D;

	UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Target->GetRootComponent());
	if (!Prim || !Prim->IsCollisionEnabled())
	{
		// 루트가 프리미티브가 아니면 콜리전 컴포넌트 하나 찾기
		TArray<UPrimitiveComponent*> Prims;
		Target->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* C : Prims)
		{
			if (C && C->IsCollisionEnabled())
			{
				Prim = C;
				break;
			}
		}
	}

	if (Prim)
	{
		FVector Closest;
		const float Dist3D = Prim->GetDistanceToCollision(From, Closest);
		if (Dist3D >= 0.f)
		{
			FinalDist2D = FVector::Dist2D(From, Closest);
		}
	}

	bInRange = (FinalDist2D <= Range);
	BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, bInRange);
}
