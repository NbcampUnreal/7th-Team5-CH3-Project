// PotatoMonster.cpp (STABILIZED + Split Child Context + AI Possess Fix)
//
// ✅ 추가/수정 요약
// 1) Split로 스폰된 Child가 Type/Rank/Skill 테이블 포인터를 못 받아서 FinalStats가 기본값으로 떨어지는 문제 해결
//    - CopyPresetContextFrom(Parent) 추가
// 2) Split Child가 AIController Possess/BT 실행이 안 돼서 "안 움직이는" 문제 해결
//    - EnsureAIControllerAndStartLogic() 추가
//    - ApplyPresetsOnce 마지막에 EnsureAIControllerAndStartLogic() 호출
// 3) OnFinalStatsApplied는 "프리셋 외부에서 FinalStats를 수동 주입"할 때만 쓰도록 유지
//    - ApplyPresetsOnce 내부에서는 이미 SplitSpec 주입하므로 중복 호출은 하지 않음

#include "PotatoMonster.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

#include "../UI/HealthBar.h"
#include "../UI/PotatoDamageTextPoolActor.h"
#include "Core/PotatoGameStateBase.h"

#include "PotatoSplitComponent.h"
#include "FXUtils/PotatoFXUtils.h"
#include "Monster/SpecialSkillComponent.h"
#include "PotatoCombatComponent.h"
#include "PotatoHardenShellComponent.h"
#include "PotatoPresetApplier.h"
#include "Combat/PotatoWeaponComponent.h"

// ==============================
// Static Helpers (CPP Local)
// ==============================

static float ComputeMinVisiblePlayRate(float BaseLen, float MinVisible, float& OutFinalLen)
{
	OutFinalLen = BaseLen;

	if (BaseLen <= KINDA_SMALL_NUMBER) return 1.f;
	if (MinVisible <= 0.f) return 1.f;

	if (BaseLen < MinVisible)
	{
		OutFinalLen = MinVisible;
		return BaseLen / MinVisible; // 0~1 배속
	}

	return 1.f;
}

static UAnimInstance* GetAnimInstanceSafe(APotatoMonster* M)
{
	if (!M) return nullptr;
	USkeletalMeshComponent* Skel = M->GetMesh();
	return Skel ? Skel->GetAnimInstance() : nullptr;
}

static void DisableMovementSafe(APotatoMonster* M)
{
	if (!M) return;
	if (UCharacterMovementComponent* MoveComp = M->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

// (선택) UFUNCTION이 존재할 때만 호출(빌드 에러 방지용)
static bool CallUFunctionIfExists(UObject* Obj, FName FuncName)
{
	if (!Obj) return false;
	UFunction* Fn = Obj->FindFunction(FuncName);
	if (!Fn) return false;
	Obj->ProcessEvent(Fn, nullptr);
	return true;
}

static bool ScheduleTimerSafe(
	APotatoMonster* Obj,
	FTimerHandle& Handle,
	void (APotatoMonster::*Func)(),
	float DelaySec)
{
	if (!IsValid(Obj) || Obj->IsActorBeingDestroyed())
		return false;

	// DelaySec NaN/Inf/음수 방어
	if (!FMath::IsFinite(DelaySec) || DelaySec < 0.f)
	{
		DelaySec = 0.f;
	}

	UWorld* W = Obj->GetWorld();
	if (!W || W->bIsTearingDown)
		return false;

	FTimerManager& TM = W->GetTimerManager();
	TM.ClearTimer(Handle);

	// ✅ 0초는 굳이 next-tick 타이머로 돌리지 말고 즉시 호출(가장 안전)
	if (DelaySec <= 0.f)
	{
		(Obj->*Func)();
		return true;
	}

	// ✅ 엔진이 제공하는 "UObject + 멤버함수" 오버로드 사용 (가장 안정적)
	TM.SetTimer(Handle, Obj, Func, DelaySec, false);
	return true;
}

// bounds fallback
static FVector ComputeBoundsTopLocation(APotatoMonster* M, float ZMul, float ZAdd)
{
	if (!M) return FVector::ZeroVector;

	if (USkeletalMeshComponent* Mesh = M->GetMesh())
	{
		const FVector Origin = Mesh->Bounds.Origin;
		const float Z = Mesh->Bounds.BoxExtent.Z * ZMul + ZAdd;
		return Origin + FVector(0.f, 0.f, Z);
	}

	if (UCapsuleComponent* Cap = M->GetCapsuleComponent())
	{
		return M->GetActorLocation() + FVector(0.f, 0.f, Cap->GetScaledCapsuleHalfHeight() * ZMul + ZAdd);
	}

	return M->GetActorLocation();
}

// ============================================================
// UI
// ============================================================

void APotatoMonster::RefreshHPBar()
{
	if (!HPBarWidget) return;
	if (MaxHealth <= 0.f) return;

	const float Ratio = Health / MaxHealth;
	HPBarWidget->SetHealthRatio(Ratio);
}

void APotatoMonster::UpdateHPBarLocation()
{
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

	// ✅ Split Child도 SpawnDefaultController가 정상 동작하도록
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	CombatComp = CreateDefaultSubobject<UPotatoCombatComponent>(TEXT("CombatComp"));

	HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComp"));
	HPBarWidgetComp->SetupAttachment(RootComponent);
	HPBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComp->SetDrawAtDesiredSize(true);
	HPBarWidgetComp->SetPivot(FVector2D(0.5f, 0.f));

	SpecialSkillComp = CreateDefaultSubobject<USpecialSkillComponent>(TEXT("SpecialSkillComp"));

	// HardenShell
	HardenShellComp = CreateDefaultSubobject<UPotatoHardenShellComponent>(TEXT("HardenShellComp"));
	if (HardenShellComp)
	{
		HardenShellComp->Deactivate();
	}

	// Split
	SplitComp = CreateDefaultSubobject<UPotatoSplitComponent>(TEXT("SplitComp"));
}

void APotatoMonster::BeginPlay()
{
	Super::BeginPlay();

	if (HPBarWidgetComp)
	{
		HPBarWidget = Cast<UHealthBar>(HPBarWidgetComp->GetUserWidgetObject());
	}

	UpdateHPBarLocation();
	RefreshHPBar();

	DamageTextPool =
		Cast<APotatoDamageTextPoolActor>(
			UGameplayStatics::GetActorOfClass(
				this,
				APotatoDamageTextPoolActor::StaticClass()
			)
		);

	// 정확한 피격지점 받기
	OnTakePointDamage.AddDynamic(this, &APotatoMonster::OnMonsterTakePointDamage);
	OnTakeRadialDamage.AddDynamic(this, &APotatoMonster::OnMonsterTakeRadialDamage);

	bHasLastHitPoint = false;
	LastHitPointWS = FVector::ZeroVector;
	LastHitBoneName = NAME_None;
	
	UE_LOG(LogTemp, Warning, TEXT("[Monster] BeginPlay Pawn=%s Controller=%s AutoPossessAI=%d AIClass=%s"),
	*GetNameSafe(this),
	*GetNameSafe(GetController()),
	(int32)AutoPossessAI,
	*GetNameSafe(AIControllerClass)
);
}

// ============================================================
// ✅ Split Child Context / AI Fix
// ============================================================

void APotatoMonster::CopyPresetContextFrom(const APotatoMonster* Parent)
{
	if (!IsValid(Parent)) return;

	// ✅ 테이블/컨텍스트 복사 (Split Child가 가장 자주 놓치는 부분)
	TypePresetTable         = Parent->TypePresetTable;
	RankPresetTable         = Parent->RankPresetTable;
	SpecialSkillPresetTable = Parent->SpecialSkillPresetTable;
	DefaultBehaviorTree     = Parent->DefaultBehaviorTree;

	// ✅ 스탯 빌드에 필요한 값들
	WaveBaseHP           = Parent->WaveBaseHP;
	PlayerReferenceSpeed = Parent->PlayerReferenceSpeed;

	// ✅ 정책: Split이면 보통 동일 타입/랭크 유지
	MonsterType = Parent->MonsterType;
	Rank        = Parent->Rank;

	// (옵션) 필요하면 더 복사: LanePoints/WarehouseActor 등
	// LanePoints = Parent->LanePoints;
	// WarehouseActor = Parent->WarehouseActor;
}

void APotatoMonster::EnsureAIControllerAndStartLogic()
{
	// 1) Possess 보장
	if (!Controller)
	{
		SpawnDefaultController();
	}

	AAIController* AICon = Cast<AAIController>(Controller);
	if (!AICon)
	{
		return;
	}

	// 2) BT 실행 보장 (Split Child "안 움직임"의 핵심 원인 케이스)
	if (ResolvedBehaviorTree)
	{
		AICon->RunBehaviorTree(ResolvedBehaviorTree);
	}

	// 3) 이동 모드 보정(혹시 DisableMovement 상태로 남는 케이스)
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

	// ----------------------------
	// Base Stats Apply
	// ----------------------------
	MaxHealth = FinalStats.MaxHP;
	Health = MaxHealth;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FinalStats.MoveSpeed;
	}

	ResolvedBehaviorTree = FinalStats.BehaviorTree;

	// ----------------------------
	// Combat
	// ----------------------------
	if (CombatComp)
	{
		CombatComp->InitFromStats(FinalStats);
	}

	// ----------------------------
	// Split (스펙 주입만! PostDamageCheck 호출 금지)
	// ----------------------------
	if (SplitComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Split] Applied. Enable=%d ThresholdNum=%d MinMaxHp=%.1f MaxDepth=%d"),
			FinalStats.bEnableSplit ? 1 : 0,
			FinalStats.SplitSpec.ThresholdPercents.Num(),
			FinalStats.SplitSpec.MinMaxHpToAllowSplit,
			FinalStats.SplitSpec.MaxDepth);

		SplitComp->ApplySpecFromFinalStats(FinalStats.SplitSpec, FinalStats.bEnableSplit);
		// Depth는 "부모가 자식에게 넘길 때" SetSplitDepth로 처리.
	}

	// ----------------------------
	// AnimSet
	// ----------------------------
	SetAnimSet(FinalStats.AnimSet);

	// ----------------------------
	// HardenShell
	// ----------------------------
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

	// ----------------------------
	// SpecialSkill
	// ----------------------------
	if (SpecialSkillComp)
	{
		SpecialSkillComp->SkillPresetTable = SpecialSkillPresetTable;
		SpecialSkillComp->InitFromFinalStats(FinalStats);
	}

	UpdateHPBarLocation();
	RefreshHPBar();

	// ✅ Split Child 포함: 프리셋 적용 후 AI/BT 보장
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

	// (전) HardenShell 감쇠
	if (HardenShellComp && HardenShellComp->IsActive())
	{
		Incoming = HardenShellComp->ModifyIncomingDamage(Incoming);
		Incoming = FMath::Max(0.f, Incoming);
		if (Incoming <= 0.f) return 0.f;
	}

	// ✅ BUG FIX: Applied는 DamageAmount가 아니라 Incoming을 써야 함
	const float Applied = Incoming;
	if (Applied <= 0.f) return 0.f;

	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	RefreshHPBar();

	// (후) HP% 기반 Harden 전환 체크
	if (HardenShellComp && HardenShellComp->IsActive())
	{
		HardenShellComp->PostDamageCheck();
	}

	// ✅ HP 기반 Split 체크
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

		float BaseZ = GetMesh()
			? GetMesh()->Bounds.BoxExtent.Z
			: 80.f;

		const int32 Dir = (DamageStackIndex % 2 == 0) ? 1 : -1;
		const float XOffset = Dir * DamageStackOffsetStep * (DamageStackIndex / 2 + 1);

		// [World Space]  월드 X축 기준 좌우 분산 — 카메라 각도에 따라 묻힐 수 있음
		// FVector DamageLoc = GetActorLocation() + FVector(XOffset, 0.f, BaseZ + 20.f);

		// [Screen Space] 카메라 Right 벡터 기준 좌우 분산 — 카메라 방향 무관하게 항상 화면 좌우
		APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CamRight = Cam ? Cam->GetActorRightVector() : FVector::RightVector;
		FVector DamageLoc =
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
// 정확한 피격 위치 갱신
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
// Hit React (+ Hit VFX)
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

	// A) 이미 재생 중이면: 스턴만 연장 + 제한적 SFX/VFX
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
		ComputeMinVisiblePlayRate(BaseLen, HitReactMinVisibleTime, FinalLen);

		float StunTime = FMath::Clamp(FinalLen, 0.1f, HitReactMaxStunTime);
		if (!FMath::IsFinite(StunTime)) StunTime = 0.1f;

		ScheduleTimerSafe(this, HitReactResumeTH, &APotatoMonster::ResumeMovementAfterHitReact, StunTime);
		return;
	}

	// B) 새로 시작은 쿨타임 제한
	if (Now - LastHitReactTime < HitReactCooldown) return;
	LastHitReactTime = Now;

	DisableMovementSafe(this);

	// 시작 SFX 1회
	{
		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, GetActorLocation(), HitSFXNearDistance, HitSFXFarDistance, HitSFXFarChance);
		const bool bBudgetOK   = PotatoFX::PassGlobalBudget(Now, 0.08f, 4, PotatoFX::EGlobalBudgetChannel::HitSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AS->GetHitSFX, GetActorLocation());
			LastHitSFXTime = Now;
		}
	}

	// 시작 VFX 1회
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

	// 몽타주 길이 보정
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
// Die (+ Death VFX + Ragdoll Fallback)
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

static void EnableRagdollFallback(APotatoMonster* M)
{
	if (!M) return;

	if (UCapsuleComponent* Cap = M->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Cap->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	if (USkeletalMeshComponent* Mesh = M->GetMesh())
	{
		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));

		Mesh->SetAllBodiesSimulatePhysics(true);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
		Mesh->bBlendPhysics = true;

		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
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

	// Death SFX/VFX는 기존 로직 유지 (생략 가능)
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

	// 타이머 정리
	if (World)
	{
		GetWorldTimerManager().ClearTimer(HitReactResumeTH);
	}

	StopAIForDead();
	DisableMovementSafe(this);

	// UI 숨김
	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetHiddenInGame(true);
	}

	// 충돌 끄기
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

	// (2) Ragdoll Fallback
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
	// ✅ "외부에서 Monster->FinalStats = AppliedStats;" 후 수동 호출하는 용도
	// ApplyPresetsOnce 내부에서는 이미 Split/Harden/Skill/AnimSet까지 적용하므로
	// 여기서는 최소 범위만 유지 (필요한 것만)
	if (SplitComp)
	{
		SplitComp->ApplySpecFromFinalStats(FinalStats.SplitSpec, FinalStats.bEnableSplit);
	}

	// (선택) 수동 주입 경로까지 커버하고 싶으면 아래를 켜도 됨
	// SetAnimSet(FinalStats.AnimSet);
	// if (SpecialSkillComp) { SpecialSkillComp->SkillPresetTable = SpecialSkillPresetTable; SpecialSkillComp->InitFromFinalStats(FinalStats); }
	// EnsureAIControllerAndStartLogic();
}