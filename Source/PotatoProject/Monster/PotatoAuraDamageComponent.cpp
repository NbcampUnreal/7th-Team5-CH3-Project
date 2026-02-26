#include "PotatoAuraDamageComponent.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UPotatoAuraDamageComponent::UPotatoAuraDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPotatoAuraDamageComponent::Configure(float InRadius, float InDps, float InTickInterval)
{
	Radius = FMath::Max(0.f, InRadius);
	Dps = FMath::Max(0.f, InDps);
	TickInterval = FMath::Max(0.01f, InTickInterval);

	if (Sphere)
	{
		Sphere->SetSphereRadius(Radius);
	}
}

void UPotatoAuraDamageComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedOwner = GetOwner();
	if (!CachedOwner) return;

	// Owner에 SphereComponent가 이미 있으면 찾아 쓰고, 없으면 생성
	Sphere = CachedOwner->FindComponentByClass<USphereComponent>();
	if (!Sphere)
	{
		Sphere = NewObject<USphereComponent>(CachedOwner, USphereComponent::StaticClass(), TEXT("PotatoAuraSphere"));
		if (Sphere)
		{
			Sphere->RegisterComponent();
			Sphere->AttachToComponent(CachedOwner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (!Sphere) return;

	Sphere->SetSphereRadius(Radius);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &UPotatoAuraDamageComponent::HandleBeginOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &UPotatoAuraDamageComponent::HandleEndOverlap);

	// 틱 시작
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(AuraTickTH, this, &UPotatoAuraDamageComponent::TickAura, TickInterval, true);
	}
}

void UPotatoAuraDamageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AuraTickTH);
	}

	OverlappingTargets.Reset();

	Super::EndPlay(EndPlayReason);
}

bool UPotatoAuraDamageComponent::IsValidTarget(AActor* A) const
{
	return IsValid(A) && !A->IsActorBeingDestroyed() && (RequiredTargetTag.IsNone() || A->ActorHasTag(RequiredTargetTag));
}

void UPotatoAuraDamageComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidTarget(OtherActor)) return;
	if (OtherActor == CachedOwner) return;

	OverlappingTargets.Add(OtherActor);
}

void UPotatoAuraDamageComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;
	OverlappingTargets.Remove(OtherActor);
}

void UPotatoAuraDamageComponent::TickAura()
{
	if (!CachedOwner || !GetWorld()) return;
	if (Dps <= 0.f) return;

	const float DamageThisTick = Dps * TickInterval;
	if (DamageThisTick <= 0.f) return;

	// Instigator
	AController* InstigatorController = nullptr;
	if (APawn* P = Cast<APawn>(CachedOwner))
	{
		InstigatorController = P->GetController();
	}

	// 순회 중 invalid 제거
	TArray<TWeakObjectPtr<AActor>> ToRemove;

	for (const TWeakObjectPtr<AActor>& W : OverlappingTargets)
	{
		AActor* T = W.Get();
		if (!IsValidTarget(T))
		{
			ToRemove.Add(W);
			continue;
		}

		UGameplayStatics::ApplyDamage(
			T,
			DamageThisTick,
			InstigatorController,
			CachedOwner,
			DamageTypeClass ? *DamageTypeClass : UDamageType::StaticClass()
		);
	}

	for (const auto& R : ToRemove)
	{
		OverlappingTargets.Remove(R);
	}
}