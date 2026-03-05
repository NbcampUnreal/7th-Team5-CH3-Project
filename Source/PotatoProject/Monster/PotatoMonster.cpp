// PotatoMonster.cpp (BUILDABLE - HPBar crash guard + Safe preset/apply + AI ensure + HitCapsule for wider hits)
//
// 핵심:
// - BeginPlay 이전 RefreshHPBar/UpdateHPBarLocation 차단
// - BeginPlay에서 HPBarWidgetComp->InitWidget()로 위젯 생성 보장
// - ApplyPresetsOnce는 "끝에서만" UI 갱신
// - OnPossess/Spawn 타이밍에서도 안전하게 돌아가도록 가드 강화
//
// + 추가(이번 반영):
// - 무기/투사체/폭발이 ECC_Pawn ObjectQuery로 판정하는 구조를 그대로 두고,
//   Monster에서만 "피격 전용 HitCapsule"을 크게 만들어 맞추기 쉽게 함
// - HitCapsule: QueryOnly + ObjectType=Pawn + Nav 영향 X
// - Root 캡슐(이동/NavAgent)은 건드리지 않음(=이동 막힘/경로 영향 최소화)
//

#include "PotatoMonster.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "../UI/HealthBar.h"
#include "../UI/PotatoDamageTextPoolActor.h"
#include "Core/PotatoGameStateBase.h"

#include "Combat/PotatoWeaponComponent.h"
#include "FXUtils/PotatoFXUtils.h"
#include "Monster/SpecialSkillComponent.h"
#include "PotatoCombatComponent.h"
#include "PotatoHardenShellComponent.h"
#include "PotatoPresetApplier.h"
#include "PotatoSplitComponent.h"
#include "PotatoAuraDamageComponent.h"
// Utils
#include "Monster/Utils/PotatoAnimUtils.h"              // ComputeMinVisiblePlayRate
#include "Monster/Utils/PotatoMonsterRuntimeUtils.h"    // GetAnimInstanceSafe, DisableMovementSafe, ScheduleTimerSafe, ComputeBoundsTopLocation
#include "Utils/PotatoActorSafety.h"

// ------------------------------------------------------------
// (Death) Ragdoll fallback은 Monster.cpp 고유 정책이므로 유지
// ------------------------------------------------------------

// ============================================================
// (LOCAL) HitCapsule tuning
// ============================================================
// - "몬스터만 수정" 조건이라 UPROPERTY를 추가로 늘리지 않고,
//   기본값은 여기 로컬 상수로 둠(원하면 나중에 헤더에 UPROPERTY로 빼도 됨)
static float G_HitCapsule_MinRadius     = 30.f;
static float G_HitCapsule_MaxRadius     = 220.f;
static float G_HitCapsule_MinHalfHeight = 50.f;
static float G_HitCapsule_MaxHalfHeight = 320.f;

// ============================================================
// UI
// ============================================================

void APotatoMonster::RefreshHPBar()
{
	// BeginPlay 이전에는 UI 건드리지 않음 (OnPossess/FinishSpawning 타이밍 크래시 방지)
	if (!HasActorBegunPlay()) return;

	// 위젯 생성 보장: InitWidget는 BeginPlay에서 수행하므로 여기선 "획득"만
	if (!IsValid(HPBarWidget) && HPBarWidgetComp)
	{
		HPBarWidget = Cast<UHealthBar>(HPBarWidgetComp->GetUserWidgetObject());
	}

	if (!IsValid(HPBarWidget)) return;
	if (MaxHealth <= 0.f) return;

	const float Ratio = Health / MaxHealth;
	HPBarWidget->SetHealthRatio(Ratio);
}

void APotatoMonster::UpdateHPBarLocation()
{
	// BeginPlay 이전에는 건드리지 않음 (Bounds/Widget 초기화 타이밍 보호)
	if (!HasActorBegunPlay()) return;
	if (!HPBarWidgetComp) return;

	float Z = 0.f;

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		Z = MeshComp->Bounds.BoxExtent.Z + HPBarExtraZ;
	}
	else if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Z = Cap->GetScaledCapsuleHalfHeight() + HPBarExtraZ;
	}

	Z += HPBarPerMonsterOffsetZ;
	Z = FMath::Clamp(Z, HPBarMinZ, HPBarMaxZ);

	HPBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, Z));
}

// ============================================================
// Constructor / BeginPlay
// ============================================================

APotatoMonster::APotatoMonster()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Split Child도 SpawnDefaultController가 정상 동작하도록
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CombatComp = CreateDefaultSubobject<UPotatoCombatComponent>(TEXT("CombatComp"));

	HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComp"));
	HPBarWidgetComp->SetupAttachment(RootComponent);
	HPBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComp->SetDrawAtDesiredSize(true);
	HPBarWidgetComp->SetPivot(FVector2D(0.5f, 0.f));

	SpecialSkillComp = CreateDefaultSubobject<USpecialSkillComponent>(TEXT("SpecialSkillComp"));

	HardenShellComp = CreateDefaultSubobject<UPotatoHardenShellComponent>(TEXT("HardenShellComp"));
	if (HardenShellComp)
	{
		HardenShellComp->Deactivate();
	}

	SplitComp = CreateDefaultSubobject<UPotatoSplitComponent>(TEXT("SplitComp"));
	AuraDamageComp = CreateDefaultSubobject<UPotatoAuraDamageComponent>(TEXT("AuraDamageComp"));
	if (AuraDamageComp)
	{
		AuraDamageComp->SetAutoActivate(false);
		AuraDamageComp->StopAura();
	}
	// ============================================================
	//  HitCapsule (피격 전용)
	// 목표:
	// - 히트스캔/폭발(ObjectQuery ECC_Pawn)에 잘 잡히게 "Pawn ObjectType" 유지
	// - Projectile은 보통 Pawn에 Overlap로 설계되는 경우가 많으니
	//   HitCapsule에서 Pawn 응답은 Overlap로 두어 OnOverlap가 안정적으로 뜨게
	// - QueryOnly + Nav 영향 X 유지
	// ============================================================
	HitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCapsule"));
	if (HitCapsule)
	{
		HitCapsule->SetupAttachment(RootComponent);
		HitCapsule->SetCollisionProfileName(TEXT("MonsterHit"));
		HitCapsule->SetGenerateOverlapEvents(true);
		HitCapsule->SetCanEverAffectNavigation(false);
		HitCapsule->InitCapsuleSize(60.f, 90.f);
		HitCapsule->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	}
}

void APotatoMonster::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[AuraDmg] BeginPlay ENTER Owner=%s Comp=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(this));
	// ============================================================
	//  HitCapsule 크기 보정 (Mesh Bounds 기반)
	// - 몬스터마다 메시 크기가 달라도 "적당히" 맞는 피격 범위를 자동으로 잡음
	// - Root 캡슐은 건드리지 않음 (Nav/이동 영향 최소화)
	// ============================================================
	if (HitCapsule)
	{
		float R = 60.f;
		float HH = 90.f;
		float Z = 0.f;

		if (USkeletalMeshComponent* MeshComp  = GetMesh())
		{
			// Mesh Bounds는 BeginPlay 이후 안정적
			const FVector Ext = MeshComp ->Bounds.BoxExtent;

			// XY 중 큰 값 = 대략적인 반경
			R = FMath::Max(Ext.X, Ext.Y);

			// Z 기준(키 높이 느낌)
			HH = FMath::Max(Ext.Z, R + 5.f);

			// 너무 과한 바닥 히트 방지 등 필요하면 조정
			Z = 0.f;

			// Nav 영향 확실히 차단(혹시 BP에서 건드렸을 때 방어)
			MeshComp ->SetCanEverAffectNavigation(false);
		}

		R  = FMath::Clamp(R,  G_HitCapsule_MinRadius,     G_HitCapsule_MaxRadius);
		HH = FMath::Clamp(HH, G_HitCapsule_MinHalfHeight, G_HitCapsule_MaxHalfHeight);

		HitCapsule->SetCapsuleSize(R, HH, true);
		HitCapsule->SetRelativeLocation(FVector(0.f, 0.f, Z));
	}

	// 위젯 생성 보장 (GetUserWidgetObject()가 null인 케이스 방지)
	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->InitWidget();
		HPBarWidget = Cast<UHealthBar>(HPBarWidgetComp->GetUserWidgetObject());
	}

	UpdateHPBarLocation();
	RefreshHPBar();

	DamageTextPool =
		Cast<APotatoDamageTextPoolActor>(
			UGameplayStatics::GetActorOfClass(this, APotatoDamageTextPoolActor::StaticClass())
		);

	// 정확한 피격지점 받기
	OnTakePointDamage.AddDynamic(this, &APotatoMonster::OnMonsterTakePointDamage);
	OnTakeRadialDamage.AddDynamic(this, &APotatoMonster::OnMonsterTakeRadialDamage);

	bHasLastHitPoint = false;
	LastHitPointWS = FVector::ZeroVector;
	LastHitBoneName = NAME_None;
	if (UCapsuleComponent* RootCap = GetCapsuleComponent())
	{
		RootCap->SetCollisionProfileName(TEXT("MonsterRoot"));
		//RootCap->OnComponentHit.AddDynamic(this, &APotatoMonster::Dbg_OnRootHit);
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bUseRVOAvoidance = false;
	}
}
void APotatoMonster::Dbg_OnRootHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this) return;

	UE_LOG(LogTemp, Warning, TEXT("[STOP] Hit %s / %s  Profile=%s ObjType=%s"),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp),
		OtherComp ? *OtherComp->GetCollisionProfileName().ToString() : TEXT("None"),
		OtherComp ? *UEnum::GetValueAsString(OtherComp->GetCollisionObjectType()) : TEXT("None"));
}
// ============================================================
// Split Child Context / AI Fix
// ============================================================

void APotatoMonster::CopyPresetContextFrom(const APotatoMonster* Parent)
{
	if (!IsValid(Parent)) return;

	TypePresetTable         = Parent->TypePresetTable;
	RankPresetTable         = Parent->RankPresetTable;
	SpecialSkillPresetTable = Parent->SpecialSkillPresetTable;
	DefaultBehaviorTree     = Parent->DefaultBehaviorTree;

	WaveBaseHP           = Parent->WaveBaseHP;
	PlayerReferenceSpeed = Parent->PlayerReferenceSpeed;

	MonsterType = Parent->MonsterType;
	Rank        = Parent->Rank;
}

void APotatoMonster::EnsureAIControllerAndStartLogic()
{
	// 1) Possess 보장
	if (!Controller)
	{
		SpawnDefaultController();
	}

	AAIController* AICon = Cast<AAIController>(Controller);
	if (!AICon) return;

	// 2) BT 실행 보장
	if (ResolvedBehaviorTree)
	{
		AICon->RunBehaviorTree(ResolvedBehaviorTree);
	}

	// 3) 이동 모드 보정
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (MoveComp->MovementMode == MOVE_None)
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}
	}
}

// ============================================================
// Preset
// ============================================================

void APotatoMonster::ApplyPresetsOnce()
{
	if (bPresetsApplied) return;
	bPresetsApplied = true;

	FinalStats = UPotatoPresetApplier::BuildFinalStats(
		this,
		MonsterType,
		Rank,
		WaveBaseHP,
		PlayerReferenceSpeed,
		TypePresetTable,
		RankPresetTable,
		SpecialSkillPresetTable,
		DefaultBehaviorTree
	);

	// Base Stats
	MaxHealth = FinalStats.MaxHP;
	Health = MaxHealth;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FinalStats.MoveSpeed;
	}

	ResolvedBehaviorTree = FinalStats.BehaviorTree;

	// Combat
	if (CombatComp)
	{
		CombatComp->InitFromStats(FinalStats);
	}

	// Split
	if (SplitComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Split] Applied. Enable=%d ThresholdNum=%d MinMaxHp=%.1f MaxDepth=%d"),
			FinalStats.bEnableSplit ? 1 : 0,
			FinalStats.SplitSpec.ThresholdPercents.Num(),
			FinalStats.SplitSpec.MinMaxHpToAllowSplit,
			FinalStats.SplitSpec.MaxDepth);

		SplitComp->ApplySpecFromFinalStats(FinalStats.SplitSpec, FinalStats.bEnableSplit);
	}
	// =========================
	// AuraDamage (Cactus contact damage)
	// =========================
	if (AuraDamageComp)
	{
		AuraDamageComp->StopAura(); // 항상 초기화

		if (FinalStats.bEnableAuraDamage)
		{
			AuraDamageComp->RequiredTargetTags = FinalStats.AuraRequiredTargetTags;
			AuraDamageComp->Configure(FinalStats.AuraRadius, FinalStats.AuraDps, FinalStats.AuraTickInterval);

			//  BeginPlay 전 호출돼도 PENDING으로 예약되고 BeginPlay에서 켜짐
			AuraDamageComp->StartAura();
		}
	}
	// AnimSet
	SetAnimSet(FinalStats.AnimSet);

	// HardenShell
	if (HardenShellComp && FinalStats.bEnableHardenShell)
	{
		HardenShellComp->Activate(true);
		HardenShellComp->InitFromFinalStats(
			FinalStats,
			GET_MEMBER_NAME_CHECKED(APotatoMonster, Health),
			GET_MEMBER_NAME_CHECKED(APotatoMonster, MaxHealth)
		);
	}
	else if (HardenShellComp)
	{
		HardenShellComp->Deactivate();
	}

	// SpecialSkill
	if (SpecialSkillComp)
	{
		SpecialSkillComp->SkillPresetTable = SpecialSkillPresetTable;
		SpecialSkillComp->InitFromFinalStats(FinalStats);
	}

	// UI는 BeginPlay 이후에만 + 마지막에만
	if (HasActorBegunPlay())
	{
		UpdateHPBarLocation();
		RefreshHPBar();
	}

	// AI/BT 보장 (Split child 포함)
	EnsureAIControllerAndStartLogic();
}

// ============================================================
// Damage
// ============================================================

float APotatoMonster::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (bIsDead) return 0.f;
	if (IsActorBeingDestroyed()) return 0.f;

	float Incoming = FMath::Max(0.f, DamageAmount);
	if (Incoming <= 0.f) return 0.f;

	// HardenShell 감쇠
	if (HardenShellComp && HardenShellComp->IsActive())
	{
		Incoming = HardenShellComp->ModifyIncomingDamage(Incoming);
		Incoming = FMath::Max(0.f, Incoming);
		if (Incoming <= 0.f) return 0.f;
	}

	const float Applied = Incoming;
	if (Applied <= 0.f) return 0.f;

	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	RefreshHPBar();

	// Harden post
	if (HardenShellComp && HardenShellComp->IsActive())
	{
		HardenShellComp->PostDamageCheck();
	}

	// Split post
	if (SplitComp)
	{
		SplitComp->PostDamageCheck();
	}

	// OnHit SpecialSkill
	if (SpecialSkillComp)
	{
		AActor* Target = DamageCauser;
		if (!Target && EventInstigator)
		{
			Target = EventInstigator->GetPawn();
		}

		if (IsValid(Target))
		{
			SpecialSkillComp->TryStartOnHit(Target);
		}
	}

	// Weapon hit notify
	if (EventInstigator)
	{
		APawn* InstigatorPawn = EventInstigator->GetPawn();
		if (InstigatorPawn)
		{
			if (UPotatoWeaponComponent* WeaponComp = InstigatorPawn->FindComponentByClass<UPotatoWeaponComponent>())
			{
				const bool bIsKill = (Health <= 0.f);
				WeaponComp->OnEnemyHit.Broadcast(bIsKill);
			}
		}
	}

	// DamageText
	if (DamageTextPool)
	{
		UWorld* W = GetWorld();
		const float Now = W ? W->TimeSeconds : 0.f;

		if (Now - LastDamageTime > DamageStackResetTime)
		{
			DamageStackIndex = 0;
		}

		LastDamageTime = Now;

		const float BaseZ = GetMesh() ? GetMesh()->Bounds.BoxExtent.Z : 80.f;

		const int32 Dir = (DamageStackIndex % 2 == 0) ? 1 : -1;
		const float XOffset = Dir * DamageStackOffsetStep * (DamageStackIndex / 2 + 1);

		APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CamRight = Cam ? Cam->GetActorRightVector() : FVector::RightVector;

		const FVector DamageLoc =
			GetActorLocation()
			+ CamRight * XOffset
			+ FVector(0.f, 0.f, BaseZ + 20.f);

		DamageTextPool->SpawnDamageText(
			FMath::RoundToInt(Applied),
			DamageLoc,
			DamageStackIndex
		);

		DamageStackIndex++;
	}

	if (Health > 0.f)
	{
		TryPlayHitReact();
	}
	else
	{
		Die();
	}

	return Applied;
}

// ============================================================
// Hit location update
// ============================================================

void APotatoMonster::OnMonsterTakePointDamage(AActor* DamagedActor, float Damage,
	AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent,
	FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
	AActor* DamageCauser)
{
	if (bIsDead) return;

	bHasLastHitPoint = true;
	LastHitPointWS = HitLocation;
	LastHitBoneName = BoneName;
}

void APotatoMonster::OnMonsterTakeRadialDamage(AActor* DamagedActor, float Damage,
	const UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead) return;

	FVector P = Origin;

	if (HitInfo.bBlockingHit)
	{
		P = FVector(HitInfo.ImpactPoint);
	}
	else if (!HitInfo.ImpactPoint.IsNearlyZero())
	{
		P = FVector(HitInfo.ImpactPoint);
	}

	bHasLastHitPoint = true;
	LastHitPointWS = P;
	LastHitBoneName = HitInfo.BoneName;
}

// ============================================================
// Hit React
// ============================================================

void APotatoMonster::TryPlayHitReact()
{
	if (bIsDead) return;
	if (IsActorBeingDestroyed()) return;

	UWorld* World = GetWorld();
	const float Now = World ? World->TimeSeconds : 0.f;

	const UPotatoMonsterAnimSet* AS = GetAnimSet();
	if (!AS || !AS->HitReactMontage) return;

	UAnimInstance* AnimInst = GetAnimInstanceSafe(this);
	if (!AnimInst) return;

	const FVector FallbackHitLoc = ComputeBoundsTopLocation(this, 0.5f, 0.f);
	const FVector HitLoc = bHasLastHitPoint ? LastHitPointWS : FallbackHitLoc;

	// A) already playing
	if (AnimInst->Montage_IsPlaying(AS->HitReactMontage))
	{
		DisableMovementSafe(this);

		if (Now - LastHitSFXTime >= HitSFXCooldown)
		{
			const bool bDistanceOK = PotatoFX::PassDistanceGate(this, GetActorLocation(), HitSFXNearDistance, HitSFXFarDistance, HitSFXFarChance);
			const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, 0.08f, 4, PotatoFX::EGlobalBudgetChannel::HitSFX);

			if (bDistanceOK && bBudgetOK)
			{
				PotatoFX::SpawnSFXSlotAtLocation(this, AS->GetHitSFX, GetActorLocation());
				LastHitSFXTime = Now;
			}
		}

		const bool bHasHitVFX = AS->HitVFX.HasAnyVFX();
		if (bHasHitVFX && (Now - LastHitVFXTime >= HitVFXCooldown))
		{
			const bool bDistanceOK = PotatoFX::PassDistanceGate(this, HitLoc, HitVFXNearDistance, HitVFXFarDistance, HitVFXFarChance);
			const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, HitVFXGlobalWindowSec, HitVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::HitVFX);

			if (bDistanceOK && bBudgetOK)
			{
				const float Scalar = PotatoFX::ComputeVFXSlotAutoScaleScalar(this, AS->HitVFX);
				const FVector FinalScale = AS->HitVFX.Scale * Scalar;

				PotatoFX::SpawnVFXSlotAtLocation(this, AS->HitVFX, HitLoc, FRotator::ZeroRotator, FinalScale);
				LastHitVFXTime = Now;
			}
		}

		const float BaseLen = AS->HitReactMontage->GetPlayLength();
		float FinalLen = BaseLen;
		(void)ComputeMinVisiblePlayRate(BaseLen, HitReactMinVisibleTime, FinalLen);

		float StunTime = FMath::Clamp(FinalLen, 0.1f, HitReactMaxStunTime);
		if (!FMath::IsFinite(StunTime)) StunTime = 0.1f;

		ScheduleTimerSafe(this, HitReactResumeTH, &APotatoMonster::ResumeMovementAfterHitReact, StunTime);
		return;
	}

	// B) cooldown
	if (Now - LastHitReactTime < HitReactCooldown) return;
	LastHitReactTime = Now;

	DisableMovementSafe(this);

	// start SFX
	{
		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, GetActorLocation(), HitSFXNearDistance, HitSFXFarDistance, HitSFXFarChance);
		const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, 0.08f, 4, PotatoFX::EGlobalBudgetChannel::HitSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AS->GetHitSFX, GetActorLocation());
			LastHitSFXTime = Now;
		}
	}

	// start VFX
	{
		const bool bHasHitVFX = AS->HitVFX.HasAnyVFX();
		if (bHasHitVFX && (Now - LastHitVFXTime >= HitVFXCooldown))
		{
			const bool bDistanceOK = PotatoFX::PassDistanceGate(this, HitLoc, HitVFXNearDistance, HitVFXFarDistance, HitVFXFarChance);
			const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, HitVFXGlobalWindowSec, HitVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::HitVFX);

			if (bDistanceOK && bBudgetOK)
			{
				const float Scalar = PotatoFX::ComputeVFXSlotAutoScaleScalar(this, AS->HitVFX);
				const FVector FinalScale = AS->HitVFX.Scale * Scalar;

				PotatoFX::SpawnVFXSlotAtLocation(this, AS->HitVFX, HitLoc, FRotator::ZeroRotator, FinalScale);
				LastHitVFXTime = Now;
			}
		}
	}

	// montage
	const float BaseLen = AS->HitReactMontage->GetPlayLength();
	float FinalLen = BaseLen;

	const float PlayRate = ComputeMinVisiblePlayRate(BaseLen, HitReactMinVisibleTime, FinalLen);
	const float PlayedLen = AnimInst->Montage_Play(AS->HitReactMontage, PlayRate);

	const float EffectiveLen = (PlayedLen > 0.f) ? FMath::Max(PlayedLen, FinalLen) : FinalLen;
	float StunTime = FMath::Clamp(EffectiveLen, 0.1f, HitReactMaxStunTime);
	if (!FMath::IsFinite(StunTime)) StunTime = 0.1f;

	ScheduleTimerSafe(this, HitReactResumeTH, &APotatoMonster::ResumeMovementAfterHitReact, StunTime);
}

void APotatoMonster::ResumeMovementAfterHitReact()
{
	if (bIsDead) return;
	if (IsActorBeingDestroyed()) return;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

// ============================================================
// Die
// ============================================================

void APotatoMonster::StopAIForDead()
{
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();

		if (UBrainComponent* Brain = AICon->BrainComponent)
		{
			Brain->StopLogic(TEXT("Dead"));
		}
	}
}

void APotatoMonster::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (APotatoGameStateBase* GS = Cast<APotatoGameStateBase>(UGameplayStatics::GetGameState(this)))
	{
		GS->AddKill(Rank);
	}

	if (SpecialSkillComp)
	{
		if (SpecialSkillComp->TryStartOnDeath(this))
		{
			CallUFunctionIfExists(SpecialSkillComp, TEXT("CancelActiveSkill"));
		}
	}

	UWorld* World = GetWorld();
	const float Now = World ? World->TimeSeconds : 0.f;

	const FVector DeathLoc = ComputeBoundsTopLocation(this, 0.2f, 0.f);

	if (const UPotatoMonsterAnimSet* AS0 = GetAnimSet())
	{
		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, GetActorLocation(), DeathSFXNearDistance, DeathSFXFarDistance, DeathSFXFarChance);
		const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, 0.12f, 2, PotatoFX::EGlobalBudgetChannel::DeathSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AS0->DeathSFX, GetActorLocation());
		}

		const bool bHasDeathVFX = (AS0->DeathVFX.Niagara || AS0->DeathVFX.Cascade);
		if (bHasDeathVFX && (Now - LastDeathVFXTime >= DeathVFXCooldown))
		{
			const bool bDistanceOK2 = PotatoFX::PassDistanceGate(this, DeathLoc, DeathVFXNearDistance, DeathVFXFarDistance, DeathVFXFarChance);
			const bool bBudgetOK2   = PotatoFX::PassGlobalBudget(Now, DeathVFXGlobalWindowSec, DeathVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::DeathVFX);

			if (bDistanceOK2 && bBudgetOK2)
			{
				const float Scalar = PotatoFX::ComputeVFXSlotAutoScaleScalar(this, AS0->DeathVFX);
				const FVector FinalScale = AS0->DeathVFX.Scale * Scalar;

				PotatoFX::SpawnVFXSlotAtLocation(this, AS0->DeathVFX, DeathLoc, GetActorRotation(), FinalScale);
				LastDeathVFXTime = Now;
			}
		}
	}

	if (World)
	{
		GetWorldTimerManager().ClearTimer(HitReactResumeTH);
	}

	StopAIForDead();
	DisableMovementSafe(this);

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetHiddenInGame(true);
	}

	SetActorEnableCollision(false);

	float Life = 2.f;

	const UPotatoMonsterAnimSet* AS = GetAnimSet();

	// (1) DeathMontage
	if (AS && AS->DeathMontage)
	{
		if (UAnimInstance* AnimInst = GetAnimInstanceSafe(this))
		{
			AnimInst->StopAllMontages(0.05f);

			const float BaseLen = AS->DeathMontage->GetPlayLength();
			float FinalLen = BaseLen;
			const float PlayRate = ComputeMinVisiblePlayRate(BaseLen, DeathMinVisibleTime, FinalLen);

			const float PlayedLen = AnimInst->Montage_Play(AS->DeathMontage, PlayRate);
			const float EffectiveLen = (PlayedLen > 0.f) ? FMath::Max(PlayedLen, FinalLen) : FinalLen;

			Life = FMath::Max(Life, EffectiveLen + 0.2f);
		}

		SetLifeSpan(Life);
		return;
	}

	// (2) Ragdoll fallback
	EnableRagdollFallback(this);

	const float RagdollLife = 3.0f;
	Life = FMath::Max(Life, RagdollLife);

	SetLifeSpan(Life);
}

// ============================================================
// Lane
// ============================================================

AActor* APotatoMonster::GetCurrentLaneTarget() const
{
	return LanePoints.IsValidIndex(LaneIndex) ? LanePoints[LaneIndex] : WarehouseActor;
}

void APotatoMonster::AdvanceLaneIndex()
{
	const int32 MaxIdx = FMath::Max(0, LanePoints.Num() - 1);
	LaneIndex = FMath::Clamp(LaneIndex + 1, 0, MaxIdx);
}

// ============================================================
// AnimSet
// ============================================================

void APotatoMonster::SetAnimSet(UPotatoMonsterAnimSet* InSet)
{
	AnimSet = InSet;
}

void APotatoMonster::OnFinalStatsApplied()
{
	// 외부에서 FinalStats 수동 주입 후 호출하는 용도
	if (SplitComp)
	{
		SplitComp->ApplySpecFromFinalStats(FinalStats.SplitSpec, FinalStats.bEnableSplit);
	}
	
	if (!AuraDamageComp) return;

	if (AuraDamageComp)
	{
		AuraDamageComp->StopAura(); // 항상 초기화

		if (FinalStats.bEnableAuraDamage)
		{
			AuraDamageComp->RequiredTargetTags = FinalStats.AuraRequiredTargetTags;
			AuraDamageComp->Configure(FinalStats.AuraRadius, FinalStats.AuraDps, FinalStats.AuraTickInterval);

			//  BeginPlay 전 호출돼도 PENDING으로 예약되고 BeginPlay에서 켜짐
			AuraDamageComp->StartAura();
		}
	}
	// 필요시 아래를 켜서 완전 경로도 커버 가능
	// SetAnimSet(FinalStats.AnimSet);
	// if (SpecialSkillComp) { SpecialSkillComp->SkillPresetTable = SpecialSkillPresetTable; SpecialSkillComp->InitFromFinalStats(FinalStats); }
	// EnsureAIControllerAndStartLogic();
}