// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PotatoPlayerHUD.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"

#include "Core/PotatoDayNightCycle.h"
#include "Core/PotatoResourceManager.h"
#include "Core/PotatoGameMode.h"
#include "Core/PotatoEnums.h"
#include "Building/BuildingSystemComponent.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "Combat/PotatoWeaponComponent.h"
#include "Player/PotatoPlayerCharacter.h"

// ============================================================
// Lifecycle
// ============================================================

void UPotatoPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// BuildSlot Border 배열 초기화 (WBP 슬롯 순서와 동일)
	BuildSlotBorders = {Border_2, Border_3, Border_4, Border_5};

	// 플레이어 캐시
	CachedPlayer = Cast<APotatoPlayerCharacter>(GetOwningPlayerPawn());

	// 창고 자동 탐색 (직접 할당된 경우 스킵)
	if (!WarehouseStructure)
	{
		// TODO: 실제 창고 BP 태그 또는 클래스로 탐색하세요.
		// 예시: UGameplayStatics::GetActorOfClass(this, APotatoWarehouse::StaticClass())
		// WarehouseStructure = Cast<APotatoPlaceableStructure>(
		//     UGameplayStatics::GetActorOfClass(this, APotatoPlaceableStructure::StaticClass()));
	}

	// BuildMode 패널 초기 상태: 숨김
	if (Border_0)
	{
		Border_0->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 초기 갱신
	RefreshResourceText();
	RefreshStorageHP();
	RefreshClockNeedle(0.0f);
	
	// 크로스헤어 위젯 생성
	if (CrosshairContainer)
	{
		auto CreateAndAdd = [&](TSubclassOf<UPotatoCrosshairBase> Class, ECrosshairType Type)
		{
			if (Class)
			{
				UPotatoCrosshairBase* Widget = CreateWidget<UPotatoCrosshairBase>(this, Class);
				if (Widget)
				{
					CrosshairContainer->AddChild(Widget);
					Widget->SetVisibility(ESlateVisibility::Hidden);
					CrosshairIntances.Add(Type, Widget);
				}
			}
		};
		CreateAndAdd(ArcSpreadCrosshairClass, ECrosshairType::ArcSpread);
		CreateAndAdd(LineSpreadCrosshairClass, ECrosshairType::LineSpread);
		CreateAndAdd(CircleCrosshairClass, ECrosshairType::Circle);
		CreateAndAdd(StaticCrosshairClass, ECrosshairType::Static);
	}
	
	// 델리게이트 바인딩
	if (CachedPlayer)
	{
		if (UPotatoWeaponComponent* WeaponComp = CachedPlayer->FindComponentByClass<UPotatoWeaponComponent>())
		{
			WeaponComp->OnWeaponChanged.AddUObject(this, &UPotatoPlayerHUD::HandleWeaponChanged);
			WeaponComp->OnAmmoChanged.AddUObject(this, &UPotatoPlayerHUD::HandleAmmoChanged);
			
			if (WeaponComp->CurrentWeaponData)
			{
				HandleWeaponChanged(WeaponComp->CurrentWeaponData);
				WeaponComp->BroadcastAmmoState();
			}
		}
	}
}

void UPotatoPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshResourceText();
	RefreshStorageHP();
	RefreshClockNeedle(InDeltaTime);
	RefreshTimeText();
	RefreshHPText();
	RefreshBuildModePanel();
}

// ============================================================
// Public API
// ============================================================

void UPotatoPlayerHUD::SetMessageText(const FText& InText, bool bVisible)
{
	if (MessageText)
	{
		MessageText->SetText(InText);
		MessageText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPotatoPlayerHUD::RefreshStorageHP()
{
	if (!StorageHPBar || !WarehouseStructure) return;

	const UPotatoStructureData* Data = WarehouseStructure->StructureData;
	if (!Data) return;

	const float MaxHP = Data->MaxHealth;
	const float CurHP = WarehouseStructure->CurrentHealth;
	StorageHPBar->SetPercent((MaxHP > 0.f) ? FMath::Clamp(CurHP / MaxHP, 0.f, 1.f) : 0.f);
}

// ============================================================
// Event Handler
// ============================================================

void UPotatoPlayerHUD::HandleWeaponChanged(const UPotatoWeaponData* NewWeaponData)
{
	if (!NewWeaponData)
	{
		return;
	}

	// 슬롯 UI 하이라이트 업데이트
	UPotatoWeaponComponent* WeaponComp = GetWeaponComp();
	if (!WeaponComp)
	{
		return;
	}

	// 빌드 모드 중에는 건물 슬롯이 동일 Border를 점유하므로 무기 강조를 적용하지 않습니다.
	UBuildingSystemComponent* BuildComp = GetBuildComp();
	if (BuildComp && BuildComp->bIsBuildMode)
	{
		return;
	}

	const int32 WeaponIdx = WeaponComp->CurrentWeaponIndex;

	// WBP에서 각 Border의 배경색 RGB는 이미 칠해져 있습니다 (감자=황색, 옥수수=녹색 등).
	// 여기서는 알파 값만 조정하여 선택/비선택 상태를 표현합니다.
	// GetBrushColor()로 현재 색을 읽고, A만 덮어써서 다시 SetBrushColor()합니다.
	for (int32 i = 0; i < BuildSlotBorders.Num(); ++i)
	{
		UBorder* Border = BuildSlotBorders[i];
		if (!Border)
		{
			continue;
		}

		FLinearColor CurrentColor = Border->GetBrushColor();
		CurrentColor.A = (i == WeaponIdx) ? WeaponSlotSelectedAlpha : WeaponSlotDefaultAlpha;
		Border->SetBrushColor(CurrentColor);
	}
	
	// 크로스헤어 전환 로직
	ECrosshairType TargetType = NewWeaponData->CrosshairType;
	for (auto& Instance : CrosshairIntances)
	{
		if (Instance.Value)
		{
			Instance.Value->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (CrosshairIntances.Contains(TargetType))
	{
		CrosshairIntances[TargetType]->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPotatoPlayerHUD::HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo)
{
	if (Ammo)
	{
		Ammo->SetText(FText::Format(NSLOCTEXT("HUD", "Ammo", "{0} / {1}"), CurrentAmmo, ReserveAmmo));
	}
}

// ============================================================
// Internal Refresh
// ============================================================

void UPotatoPlayerHUD::RefreshResourceText()
{
	UPotatoResourceManager* RM = GetWorld()->GetSubsystem<UPotatoResourceManager>();
	if (!RM) return;

	auto MakeText = [&](EResourceType Type) -> FText
	{
		const int32 Amount = RM->GetResource(Type);
		const int32 Rate = RM->GetTotalProductionPerMinute(Type);
		if (Rate > 0)
		{
			return FText::Format(NSLOCTEXT("HUD", "ResRate", "{0}(+{1}/분)"), Amount, Rate);
		}
		return FText::AsNumber(Amount);
	};

	if (ResourceWood) ResourceWood->SetText(MakeText(EResourceType::Wood));
	if (ResourceStone) ResourceStone->SetText(MakeText(EResourceType::Stone));
	if (ResourceCrop) ResourceCrop->SetText(MakeText(EResourceType::Crop));
	if (ResourceLivestock) ResourceLivestock->SetText(MakeText(EResourceType::Livestock));

	// 미니 자원 행
	if (WoodAmount) WoodAmount->SetText(FText::AsNumber(RM->GetResource(EResourceType::Wood)));
	if (StoneAmount) StoneAmount->SetText(FText::AsNumber(RM->GetResource(EResourceType::Stone)));
	if (CropAmount) CropAmount->SetText(FText::AsNumber(RM->GetResource(EResourceType::Crop)));
	if (LivestockAmount) LivestockAmount->SetText(FText::AsNumber(RM->GetResource(EResourceType::Livestock)));
}

void UPotatoPlayerHUD::RefreshClockNeedle(float DeltaTime)
{
	if (!DayClockNeedle) return;

    if (GetWorld()->IsPaused()) return; // 게임 일시정지 시 바늘 멈춤

	UPotatoDayNightCycle* DNC = GetWorld()->GetSubsystem<UPotatoDayNightCycle>();
	if (!DNC || !DNC->IsSystemStarted()) return;

	APotatoGameMode* GM = GetWorld()->GetAuthGameMode<APotatoGameMode>();
	if (!GM) return;

	const float TotalDuration = GM->GetTotalCycleDuration();
	if (TotalDuration <= 0.f) return;

	// DayNightCycle의 기준값에 DeltaTime을 더해 프레임마다 부드럽게 진행
	const float ServerElapsed = DNC->GetTotalElapsedTime();
	if (SmoothElapsedTime < 0.f || FMath::Abs(SmoothElapsedTime - ServerElapsed) > 2.0f)
	{
		// 초기화 또는 오차가 클 때 즉시 동기화
		SmoothElapsedTime = ServerElapsed;
	}
	else
	{
		SmoothElapsedTime += DeltaTime;
		// 서버 기준값과 미세하게 동기화 (드리프트 방지)
		SmoothElapsedTime = FMath::Lerp(SmoothElapsedTime, ServerElapsed + DeltaTime * 0.5f, DeltaTime * 0.5f);
	}

	const float Progress = FMath::Clamp(SmoothElapsedTime / TotalDuration, 0.f, 1.f);
	const float TargetAngle = FMath::Lerp(ClockNeedleMinAngle, ClockNeedleMaxAngle, Progress);

	FWidgetTransform Transform = DayClockNeedle->GetRenderTransform();
	Transform.Angle = TargetAngle;
	DayClockNeedle->SetRenderTransform(Transform);
}

void UPotatoPlayerHUD::RefreshBuildModePanel()
{
	UBuildingSystemComponent* BuildComp = GetBuildComp();

	const bool bInBuildMode = BuildComp && BuildComp->bIsBuildMode;

	// 패널 표시 / 숨김
	if (Border_0)
	{
		Border_0->SetVisibility(bInBuildMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!bInBuildMode) return;

	// 슬롯 강조
	const int32 SelectedIdx = BuildComp->CurrentSlotIndex;
	for (int32 i = 0; i < BuildSlotBorders.Num(); ++i)
	{
		SetBorderColor(BuildSlotBorders[i],
		               (i == SelectedIdx) ? BuildSlotSelectedColor : BuildSlotDefaultColor);
	}
}

void UPotatoPlayerHUD::RefreshTimeText()
{
	UPotatoDayNightCycle* DNC = GetWorld()->GetSubsystem<UPotatoDayNightCycle>();
	if (!DNC) return;

	// 남은 시간 (MM:SS)
	if (RemainingTime)
	{
		const float Remaining = DNC->GetRemainingDayTime();
		const int32 Minutes = FMath::FloorToInt(Remaining / 60.f);
		const int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.f));
		RemainingTime->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
	}

	// Day N
	if (CurrentDay)
	{
		if (APotatoGameMode* GM = GetWorld()->GetAuthGameMode<APotatoGameMode>())
		{
			CurrentDay->SetText(
				FText::Format(NSLOCTEXT("HUD", "Day", "Day {0}"), GM->GetCurrentDay()));
		}
	}
}

void UPotatoPlayerHUD::RefreshHPText()
{
	if (!HP || !CachedPlayer) return;

	HP->SetText(FText::Format(
		NSLOCTEXT("HUD", "HP", "{0} / {1}"),
		FMath::CeilToInt(CachedPlayer->GetCurrentHP()),
		FMath::CeilToInt(CachedPlayer->GetMaxHP())));
}

// ============================================================
// Helpers
// ============================================================

void UPotatoPlayerHUD::SetBorderColor(UBorder* InBorder, const FLinearColor& InColor)
{
	if (!InBorder) return;
	InBorder->SetBrushColor(InColor);
}

UBuildingSystemComponent* UPotatoPlayerHUD::GetBuildComp() const
{
	if (!CachedPlayer) return nullptr;
	return CachedPlayer->FindComponentByClass<UBuildingSystemComponent>();
}

UPotatoWeaponComponent* UPotatoPlayerHUD::GetWeaponComp() const
{
	if (!CachedPlayer) return nullptr;
	return CachedPlayer->FindComponentByClass<UPotatoWeaponComponent>();
}
