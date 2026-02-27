// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PotatoPlayerHUD.h"

#include "PotatoDialogueWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"

#include "Core/PotatoDayNightCycle.h"
#include "Core/PotatoResourceManager.h"
#include "Core/PotatoGameMode.h"
#include "Core/PotatoEnums.h"
#include "Core/PotatoProductionComponent.h"
#include "Building/BuildingSystemComponent.h"
#include "Building/PotatoPlaceableStructure.h"
#include "Building/PotatoStructureData.h"
#include "Combat/PotatoWeaponComponent.h"
#include "Player/PotatoPlayerCharacter.h"

#include "PotatoDialogueData.h"
#include "PotatoDialogueWidget.h"
#include "PotatoHitMarker.h"

// ============================================================
// Lifecycle
// ============================================================

void UPotatoPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// BuildSlot Border 배열 초기화 (WBP 슬롯 순서와 동일)
	BuildSlotBorders = {
		BuildBorder_0, BuildBorder_1, BuildBorder_2, BuildBorder_3, BuildBorder_4, BuildBorder_5, BuildBorder_6,
		BuildBorder_7, BuildBorder_8
	};

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

		if (HitMarkerClass)
		{
			HitMarkerInstance = CreateWidget<UPotatoHitMarker>(this, HitMarkerClass);
			if (HitMarkerInstance)
			{
				CrosshairContainer->AddChild(HitMarkerInstance);
				HitMarkerInstance->SetVisibility(ESlateVisibility::Hidden);
			}
		}
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
			WeaponComp->OnEnemyHit.AddUObject(this, &UPotatoPlayerHUD::HandleEnemyHit);

			if (WeaponComp->CurrentWeaponData)
			{
				HandleWeaponChanged(WeaponComp->CurrentWeaponData);
				WeaponComp->BroadcastAmmoState();
			}
		}

		// Dialogue Widget 바인딩
		CachedPlayer->OnNextDialoguePressed.AddDynamic(this, &UPotatoPlayerHUD::HandleNextDialogueInput);
		if (DialogueWidget)
		{
			DialogueWidget->SetVisibility(ESlateVisibility::Hidden);
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

	// Build Mode 바인딩
	if (UBuildingSystemComponent* BuildComponent = GetBuildComp())
	{
		BuildComponent->OnBuildModeToggled.AddDynamic(this, &UPotatoPlayerHUD::HandleBuildModeToggled);
		BuildComponent->OnBuildSlotChanged.AddDynamic(this, &UPotatoPlayerHUD::HandleBuildSlotChanged);

		HandleBuildModeToggled(BuildComponent->bIsBuildMode);
	}
}

void UPotatoPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshStorageHP();
	RefreshClockNeedle(InDeltaTime);
	RefreshTimeText();
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

void UPotatoPlayerHUD::PlayDialogue(FName RowName)
{
	if (!DialogueDataTable || !DialogueWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayDialogue failed: Missing DataTable or Widget"));
		return;
	}

	static const FString ContextString(TEXT("DialogueContext"));
	FPotatoDialogueRow* Row = DialogueDataTable->FindRow<FPotatoDialogueRow>(RowName, ContextString);

	if (Row)
	{
		DialogueWidget->StartDialogue(Row);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialogue Row '%s' not found!"), *RowName.ToString());
	}
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
		break;
	case EResourceType::Stone:
		if (ResourceStone) ResourceStone->SetText(NewText);
		break;
	case EResourceType::Crop:
		if (ResourceCrop) ResourceCrop->SetText(NewText);
		break;
	case EResourceType::Livestock:
		if (ResourceLivestock) ResourceLivestock->SetText(NewText);
		break;
	}
}

void UPotatoPlayerHUD::HandleNextDialogueInput()
{
	if (DialogueWidget && DialogueWidget->IsVisible())
	{
		DialogueWidget->AdvanceDialogue();
	}
}

void UPotatoPlayerHUD::HandleBuildModeToggled(bool bIsBuildMode)
{
	ESlateVisibility NewVisibility = bIsBuildMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (Border_0)
	{
		Border_0->SetVisibility(NewVisibility);
	}
	if (CostBox)
	{
		CostBox->SetVisibility(NewVisibility);
	}
}

void UPotatoPlayerHUD::HandleBuildSlotChanged(int32 SlotIndex, const UPotatoStructureData* SelectedData)
{
	for (int32 i = 0; i < BuildSlotBorders.Num(); ++i)
	{
		SetBorderColor(BuildSlotBorders[i], (i == SlotIndex) ? BuildSlotSelectedColor : BuildSlotDefaultColor);
	}
	
	// CDO로 건설에 필요한 비용 가져오기
	if (SelectedData && SelectedData->StructureClass)
	{
		APotatoPlaceableStructure* CDO = SelectedData->StructureClass->GetDefaultObject<APotatoPlaceableStructure>();
		if (CDO)
		{
			UPotatoProductionComponent* ProductionComponent = CDO->GetComponentByClass<UPotatoProductionComponent>();
			if (ProductionComponent)
			{
				if (WoodAmount) WoodAmount->SetText(FText::AsNumber(ProductionComponent->GetBuyCostWood()));
				if (StoneAmount) StoneAmount->SetText(FText::AsNumber(ProductionComponent->GetBuyCostStone()));
				if (CropAmount) CropAmount->SetText(FText::AsNumber(ProductionComponent->GetBuyCostCrop()));
				if (LivestockAmount) LivestockAmount->SetText(FText::AsNumber(ProductionComponent->GetBuyCostLivestock()));
			}
		}
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

void UPotatoPlayerHUD::HandleEnemyHit(bool bIsKill)
{
	if (HitMarkerInstance)
	{
		HitMarkerInstance->PlayHitMarker(bIsKill);
	}
}

// ============================================================
// Internal Refresh
// ============================================================

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
