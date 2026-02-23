// PotatoDamageTextActor.cpp
#include "PotatoDamageTextActor.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "PotatoDamageTextWidget.h"

APotatoDamageTextActor::APotatoDamageTextActor()
{
	PrimaryActorTick.bCanEverTick = true;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	RootComponent = WidgetComp;

	// 기본 설정
	WidgetComp->SetWidgetSpace(EWidgetSpace::World);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComp->SetTwoSided(true);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void APotatoDamageTextActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureWidgetClassApplied();

	// 캐시(없어도 ShowDamage에서 다시 시도하긴 함)
	if (WidgetComp)
	{
		Widget = Cast<UPotatoDamageTextWidget>(WidgetComp->GetUserWidgetObject());
	}
}

void APotatoDamageTextActor::EnsureWidgetClassApplied()
{
	if (!WidgetComp) return;

	// 이미 인스턴스가 만들어졌는데 클래스가 다르면 다시 세팅
	if (DamageTextWidgetClass)
	{
		const TSubclassOf<UUserWidget> CurrentClass = WidgetComp->GetWidgetClass();
		if (CurrentClass != DamageTextWidgetClass)
		{
			WidgetComp->SetWidgetClass(DamageTextWidgetClass);
		}
	}
}

void APotatoDamageTextActor::ShowDamage(
	int32 Damage,
	const FVector& WorldLocation,
	int32 StackIndex,
	FPotatoDamageTextFinished InOnFinished
)
{
	OnFinished = InOnFinished;
	Elapsed = 0.f;

	SetActorHiddenInGame(false);

	StartLoc = WorldLocation;
	SetActorLocation(StartLoc);

	CurrentRiseSpeed = BaseRiseSpeed + StackIndex * 12.f;

	// 혹시 BP에서 나중에 WidgetClass 세팅했을 수도 있으니, 매번 1회 안전 적용
	EnsureWidgetClassApplied();

	if (!Widget && WidgetComp)
	{
		Widget = Cast<UPotatoDamageTextWidget>(WidgetComp->GetUserWidgetObject());
	}

	if (Widget)
	{
		Widget->SetDamage(Damage);
	}
}

void APotatoDamageTextActor::Tick(float DeltaSeconds)
{
	if (IsHidden()) return;

	Elapsed += DeltaSeconds;

	// 위로 떠오르기
	SetActorLocation(StartLoc + FVector(0.f, 0.f, CurrentRiseSpeed * Elapsed));

	if (Elapsed >= LifeTime)
	{
		SetActorHiddenInGame(true);

		if (OnFinished.IsBound())
		{
			OnFinished.Execute(this);
		}
	}
}