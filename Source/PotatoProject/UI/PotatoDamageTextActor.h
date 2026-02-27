// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotatoDamageTextActor.generated.h"

class UWidgetComponent;
class UPotatoDamageTextWidget;

DECLARE_DELEGATE_OneParam(FPotatoDamageTextFinished, APotatoDamageTextActor*);

UCLASS()
class POTATOPROJECT_API APotatoDamageTextActor : public AActor
{
	GENERATED_BODY()
public:
	APotatoDamageTextActor();

	// DamageText 표시 시작
	void ShowDamage(
		int32 Damage,
		const FVector& WorldLocation,
		int32 StackIndex,
		FPotatoDamageTextFinished InOnFinished
	);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// -------------------------
		// UI
		// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|UI")
	TSubclassOf<UUserWidget> DamageTextWidgetClass;

	// -------------------------
	// Lifetime / Motion
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float LifeTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float BaseRiseSpeed = 60.f;

	// -------------------------
	// Billboard (정석)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Billboard")
	bool bYawOnlyBillboard = true;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Billboard")
	bool bTwoSided = true;

	// -------------------------
	// Distance Scale (정석)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Scale")
	bool bUseDistanceScale = true;

	// 이 거리(uu)에서 Scale=1.0 기준
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Scale")
	float ReferenceDistance = 800.f;

	// 0이면 고정 1.0, 1이면 거리비율 반영 강함
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Scale", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DistanceScaleStrength = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Scale")
	float MinScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Scale")
	float MaxScale = 1.35f;

	// -------------------------
	// Camera Clamp (정석)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Clamp")
	bool bClampTooClose = true;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Clamp")
	float MinEffectiveDistance = 180.f;

	// -------------------------
	// Render / Sorting (정석)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Render")
	int32 TranslucencySortPriority = 50;

	// -------------------------
	// Fade (정석)
	// -------------------------
	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Fade")
	bool bFadeOut = true;

	UPROPERTY(EditDefaultsOnly, Category = "DamageText|Fade", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.18f;

private:
	UPROPERTY(VisibleAnywhere, Category = "DamageText|UI")
	UWidgetComponent* WidgetComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "DamageText|UI")
	UPotatoDamageTextWidget* Widget = nullptr;

	FPotatoDamageTextFinished OnFinished;

	float Elapsed = 0.f;
	float CurrentRiseSpeed = 60.f;
	FVector StartLoc = FVector::ZeroVector;

	// 런타임
	float StartTime = 0.f;
	bool bShowing = false;

	void EnsureWidgetClassApplied();

	// ---- 옵션 세트 핵심 헬퍼 ----
	void UpdateBillboardYawOnly();

	// [World Space] SetWorldScale3D 기반 거리 스케일 (PP 영향 받음)
	void UpdateDistanceScaleAndClamp();

	// [Screen Space] RenderTransform 기반 거리 스케일 (PP 영향 없음)
	void UpdateDistanceScale_ScreenSpace();

	void UpdateFade(float NowTime);
	void Finish(); // LifeTime 종료 처리(숨김 + OnFinished)
};