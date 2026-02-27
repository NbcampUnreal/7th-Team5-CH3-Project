#include "PotatoPlaceableStructure.h"
#include "PotatoStructureData.h"
#include "Components/WidgetComponent.h"
#include "UI/HealthBar.h"

APotatoPlaceableStructure::APotatoPlaceableStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Monster 패턴과 동일하게 HPBarWidgetComp 생성
    HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComp"));
    HPBarWidgetComp->SetupAttachment(RootComponent);
    HPBarWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // 필요에 따라 위치 조정하세요
    HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    HPBarWidgetComp->SetDrawAtDesiredSize(true);
    HPBarWidgetComp->SetPivot(FVector2D(0.5f, 0.0f));
    HPBarWidgetComp->SetVisibility(false); // 초기에는 숨김
}

void APotatoPlaceableStructure::BeginPlay()
{
	Super::BeginPlay();
	
	if (StructureData)
	{
		if (StructureData->bIsDestructible)
		{
			CurrentHealth = StructureData->MaxHealth;
        
            if (HPBarWidgetComp)
            {
                HPBarWidget = Cast<UHealthBar>(HPBarWidgetComp->GetUserWidgetObject());
                RefreshHpBar();
            }
		}
		else
		{
			CurrentHealth = -1.0f; // 파괴 불가능
		}

		UE_LOG(LogTemp, Warning, TEXT("%s Initialized!"), *StructureData->DisplayName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Structure 객체가 DataAsset 없이 스폰됐습니다!"));
	}
}

void APotatoPlaceableStructure::Interact(AActor* Interactor)
{
	if (IsDestructible() && CurrentHealth <= 0.0f)
	{
		return;
	}
	
	if (OnInteractedDelegate.IsBound())
	{
		OnInteractedDelegate.Broadcast(Interactor);
	}
}

bool APotatoPlaceableStructure::IsDestructible() const
{
	if (StructureData)
	{
		return StructureData->bIsDestructible;
	}

	return false;
}

float APotatoPlaceableStructure::TakeDamage(float DamageAmount, 
											FDamageEvent const& DamageEvent,
											AController* EventInstigator, 
											AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (!StructureData)
	{
		return 0.0f;
	}
	if (bDestroyed)
	{
		return 0.0f;
	}
	if (!IsDestructible() || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}
	
	const float DamageToApply = FMath::Min(ActualDamage, CurrentHealth);
    
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0.0f, StructureData->MaxHealth);
    RefreshHpBar();
    ShowHPBar(); // 피격 시 HP Bar 표시

	if (CurrentHealth <= 0.0f)
	{
		// 파괴 처리: FX 재생 등
		// OnStructureDestroyed.Broadcast();
		bDestroyed = true;
		Destroy();
	}
	return DamageToApply;
}

void APotatoPlaceableStructure::RefreshHpBar()
{
    if (!HPBarWidget || !StructureData || StructureData->MaxHealth <= 0.f) return;
    HPBarWidget->SetHealthRatio(CurrentHealth / StructureData->MaxHealth);
}

void APotatoPlaceableStructure::ShowHPBar()
{
    if (HPBarWidgetComp)
    {
        HPBarWidgetComp->SetVisibility(true);
        GetWorldTimerManager().ClearTimer(HPBarHideTimerHandle);
        GetWorldTimerManager().SetTimer(HPBarHideTimerHandle, this, &APotatoPlaceableStructure::HideHPBar, HPBarHideDelay, false);
    }
}

void APotatoPlaceableStructure::HideHPBar()
{
    if (HPBarWidgetComp)
    {
        HPBarWidgetComp->SetVisibility(false);
    }
}
