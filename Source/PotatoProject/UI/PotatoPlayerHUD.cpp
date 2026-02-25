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
		// HP 바인딩
		CachedPlayer->OnHPChanged.AddDynamic(this, &UPotatoPlayerHUD::HandleHPChanged);
		HandleHPChanged(CachedPlayer->GetCurrentHP(), CachedPlayer->GetMaxHP());
		
		// Weapon 바인딩
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
	
	// 자원 바인딩
	UPotatoResourceManager* ResourceManager = GetWorld()->GetSubsystem<UPotatoResourceManager>();
	if (ResourceManager)
	{
		ResourceManager->OnResourceChanged.AddDynamic(this, &UPotatoPlayerHUD::HandleResourceChanged);
		
		HandleResourceChanged(EResourceType::Wood, ResourceManager->GetWood());
		HandleResourceChanged(EResourceType::Stone, ResourceManager->GetStone());
		HandleResourceChanged(EResourceType::Crop, ResourceManager->GetCrop());
		HandleResourceChanged(EResourceType::Livestock, ResourceManager->GetLivestock());
	}
}

void UPotatoPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshStorageHP();
	RefreshClockNeedle(InDeltaTime);
	RefreshTimeText();
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

void UPotatoPlayerHUD::HandleHPChanged(float CurrentHP, float MaxHP)
{
	if (HP)
	{
		HP->SetText(FText::Format(
			NSLOCTEXT("HUD", "HP", "{0} / {1}"),
			FMath::CeilToInt(CurrentHP),
			FMath::CeilToInt(MaxHP)));
	}
}

void UPotatoPlayerHUD::HandleResourceChanged(EResourceType Type, int32 NewValue)
{
	UPotatoResourceManager* ResourceManager = GetWorld()->GetSubsystem<UPotatoResourceManager>();
	if (!ResourceManager)
	{
		return;
	}
	
	auto FormatResourceText = [&](int32 Amount, EResourceType InType) -> FText
	{
		const int32 Rate = ResourceManager->GetTotalProductionPerMinute(InType);
		if (Rate > 0)
		{
			return FText::Format(NSLOCTEXT("HUD", "ResRate", "{0}(+{1}/min)"), Amount, Rate);
		}
		return FText::AsNumber(Amount);
	};
	
	FText NewText = FormatResourceText(NewValue, Type);
	FText AmountText = FText::AsNumber(NewValue);

	switch (Type)
	{
	case EResourceType::Wood:
		if (ResourceWood) ResourceWood->SetText(NewText);
		if (WoodAmount) WoodAmount->SetText(AmountText);
		break;
	case EResourceType::Stone:
		if (ResourceStone) ResourceStone->SetText(NewText);
		if (StoneAmount) StoneAmount->SetText(AmountText);
		break;
	case EResourceType::Crop:
		if (ResourceCrop) ResourceCrop->SetText(NewText);
		if (CropAmount) CropAmount->SetText(AmountText);
		break;
	case EResourceType::Livestock:
		if (ResourceLivestock) ResourceLivestock->SetText(NewText);
		if (LivestockAmount) LivestockAmount->SetText(AmountText);
		break;
	}
}

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
	if (!Ammo)
	{
		return;
	}
	int32 MaxAmmoSize = 0;
	UPotatoWeaponComponent* WeaponComponent = GetWeaponComp();
	if (WeaponComponent && WeaponComponent->CurrentWeaponData)
	{
		MaxAmmoSize = WeaponComponent->CurrentWeaponData->MaxAmmoSize;
	}
	FString AmmoString = FString::Printf(TEXT("%d/%d (+%d)"), CurrentAmmo, MaxAmmoSize, ReserveAmmo);
	Ammo->SetText(FText::FromString(AmmoString));
	
	if (MaxAmmoSize > 0)
	{
		float CurrentRatio = (float)CurrentAmmo / (float)MaxAmmoSize;
		if (CurrentRatio == 0)
		{
			Ammo->SetColorAndOpacity(LowAmmoColor);
		}
		else if (CurrentRatio <= LowAmmoPercentage)
		{
			Ammo->SetColorAndOpacity(LowAmmoColor);
		}
		else
		{
			Ammo->SetColorAndOpacity(NormalAmmoColor);
		}
	}
}

// ============================================================
// Internal Refresh
// ============================================================

void UPotatoPlayerHUD::RefreshClockNeedle(float DeltaTime)
{
	if (!DayClockNeedle) return;

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
