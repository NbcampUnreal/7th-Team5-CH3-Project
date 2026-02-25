// PotatoDamageTextActor.cpp
#include "PotatoDamageTextActor.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "PotatoDamageTextWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

APotatoDamageTextActor::APotatoDamageTextActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false); // 표시 중에만 Tick

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(RootComponent);

	//  World Space로 전환 (입체감)
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

	// 캐시(없어도 ShowDamage에서 다시 시도)
	if (WidgetComp)
	{
		Widget = Cast<UPotatoDamageTextWidget>(WidgetComp->GetUserWidgetObject());
	}

	// 옵션 반영
	if (WidgetComp)
	{
		WidgetComp->SetTwoSided(bTwoSided);
	}
}

void APotatoDamageTextActor::EnsureWidgetClassApplied()
{
	if (!WidgetComp) return;

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
	StartTime = GetWorld() ? GetWorld()->TimeSeconds : 0.f;
	bShowing = true;

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true); //  표시 중에만 Tick

	StartLoc = WorldLocation;
	SetActorLocation(StartLoc);

	CurrentRiseSpeed = BaseRiseSpeed + StackIndex * 12.f;

	EnsureWidgetClassApplied();

	if (!Widget && WidgetComp)
	{
		Widget = Cast<UPotatoDamageTextWidget>(WidgetComp->GetUserWidgetObject());
	}

	if (Widget)
	{
		Widget->SetDamage(Damage);
	}

	// 시작 시점 상태 초기화(페이드)
	if (bFadeOut && WidgetComp)
	{
		if (UUserWidget* W = WidgetComp->GetUserWidgetObject())
		{
			W->SetRenderOpacity(1.f);
		}
	}

	// 첫 프레임 삐끗 방지: 즉시 한 번 업데이트
	UpdateBillboardYawOnly();
	UpdateDistanceScaleAndClamp();
}

void APotatoDamageTextActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bShowing || IsHidden()) return;

	Elapsed += DeltaSeconds;

	// 위로 떠오르기
	SetActorLocation(StartLoc + FVector(0.f, 0.f, CurrentRiseSpeed * Elapsed));

	//  카메라 빌보드 (Yaw-only)
	UpdateBillboardYawOnly();

	//  거리 스케일 + 근접 클램프
	UpdateDistanceScaleAndClamp();

	//  FadeOut
	if (bFadeOut)
	{
		const float Now = GetWorld() ? GetWorld()->TimeSeconds : 0.f;
		UpdateFade(Now);
	}

	// 종료
	if (Elapsed >= LifeTime)
	{
		Finish();
	}
}

void APotatoDamageTextActor::Finish()
{
	//  안정 순서: 먼저 상태 정리 → 그 다음 풀 반환 콜백
	bShowing = false;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	if (OnFinished.IsBound())
	{
		OnFinished.Execute(this);
	}
}

// ------------------------------
// 옵션 세트 핵심 구현
// ------------------------------

void APotatoDamageTextActor::UpdateBillboardYawOnly()
{
	if (!WidgetComp) return;

	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam) return;

	const FVector CamLoc = Cam->GetCameraLocation();
	const FVector MyLoc = WidgetComp->GetComponentLocation();

	const FRotator LookAt = (CamLoc - MyLoc).Rotation();

	FRotator FinalRot;
	if (bYawOnlyBillboard)
	{
		FinalRot = FRotator(0.f, LookAt.Yaw, 0.f);
	}
	else
	{
		FinalRot = LookAt;
	}

	WidgetComp->SetWorldRotation(FinalRot);
}

void APotatoDamageTextActor::UpdateDistanceScaleAndClamp()
{
	if (!WidgetComp) return;
	if (!bUseDistanceScale && !bClampTooClose) return;

	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam) return;

	const FVector CamLoc = Cam->GetCameraLocation();
	const FVector MyLoc = WidgetComp->GetComponentLocation();

	float Dist = FVector::Dist(CamLoc, MyLoc);

	// 근접 클램프(튀는 현상 방지)
	if (bClampTooClose)
	{
		Dist = FMath::Max(Dist, MinEffectiveDistance);
	}

	if (!bUseDistanceScale) return;

	const float SafeRef = FMath::Max(ReferenceDistance, 1.f);

	// 멀수록 조금 커져서 읽기 쉬운 기본 세팅
	const float Raw = Dist / SafeRef;
	float Scale = FMath::Lerp(1.f, Raw, DistanceScaleStrength);
	Scale = FMath::Clamp(Scale, MinScale, MaxScale);

	WidgetComp->SetWorldScale3D(FVector(Scale));
}

void APotatoDamageTextActor::UpdateFade(float NowTime)
{
	if (!WidgetComp) return;

	UUserWidget* W = WidgetComp->GetUserWidgetObject();
	if (!W) return;

	const float FadeStart = FMath::Max(0.f, LifeTime - FadeOutDuration);
	if (Elapsed < FadeStart)
	{
		W->SetRenderOpacity(1.f);
		return;
	}

	const float T = (FadeOutDuration <= KINDA_SMALL_NUMBER)
		? 1.f
		: FMath::Clamp((Elapsed - FadeStart) / FadeOutDuration, 0.f, 1.f);

	// SmoothStep
	const float Smooth = 1.f - (T * T * (3.f - 2.f * T));
	W->SetRenderOpacity(Smooth);
}