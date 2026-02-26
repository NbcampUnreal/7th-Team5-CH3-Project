// PotatoMonster.cpp (STABILIZED)
// - Crash fix: ScheduleTimerSafe 안정화 (World null/teardown/Delay NaN/weak delegate)
// - Bug fix: TakeDamage에서 HardenShell로 줄인 Incoming을 Applied에 반영
// - Safety: GetWorld() null guard (DamageText Now 등)
// - Safety: TryPlayHitReact에 IsActorBeingDestroyed 가드
// - Safety: Die에서 CancelActiveSkill이 없어도 빌드되도록 UFunction 호출로 보호

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
#include "TimerManager.h"
#include "../UI/HealthBar.h"
#include "../UI/PotatoDamageTextPoolActor.h"
#include "Core/PotatoGameStateBase.h"


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
	// Obj 유효성
	if (!IsValid(Obj) || Obj->IsActorBeingDestroyed())
		return false;

	// DelaySec NaN/Inf 방어 (SetTimer는 이런 값에 약함)
	if (!FMath::IsFinite(DelaySec))
	{
		DelaySec = 0.f;
	}
	if (DelaySec < 0.f)
	{
		DelaySec = 0.f;
	}

	UWorld* W = Obj->GetWorld();
	if (!W || W->bIsTearingDown)
		return false;

	FTimerManager& TM = W->GetTimerManager();

	// 연장/재예약 정책: 기존 타이머는 제거 후 다시 건다
	TM.ClearTimer(Handle);

	// raw this 바인딩 대신 Weak로 안전화
	TWeakObjectPtr<APotatoMonster> WeakObj = Obj;

	auto Call = [WeakObj, Func]()
	{
		if (APotatoMonster* P = WeakObj.Get())
		{
			(P->*Func)();
		}
	};

	if (DelaySec <= 0.f)
	{
		TM.SetTimerForNextTick(FTimerDelegate::CreateLambda(Call));
		return true;
	}

	TM.SetTimer(Handle, FTimerDelegate::CreateLambda(Call), DelaySec, false);
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

	MaxHealth = FinalStats.MaxHP;
	Health = MaxHealth;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FinalStats.MoveSpeed;
	}

	ResolvedBehaviorTree = FinalStats.BehaviorTree;

	if (CombatComp)
	{
		CombatComp->InitFromStats(FinalStats);
	}

	SetAnimSet(FinalStats.AnimSet);

	// HardenShell
	if (HardenShellComp)
	{
		if (FinalStats.bEnableHardenShell)
		{
			HardenShellComp->Activate(true);

			HardenShellComp->HpPropertyName    = GET_MEMBER_NAME_CHECKED(APotatoMonster, Health);
			HardenShellComp->MaxHpPropertyName = GET_MEMBER_NAME_CHECKED(APotatoMonster, MaxHealth);

			HardenShellComp->TriggerHpPercent = FMath::Clamp(FinalStats.HardenTriggerHpPercent, 0.f, 1.f);
			HardenShellComp->DamageMultiplierWhenHardened = FMath::Max(0.f, FinalStats.HardenDamageMultiplier);
			HardenShellComp->HardenTint = FinalStats.HardenTint;
		}
		else
		{
			HardenShellComp->Deactivate();
		}
	}

	// SpecialSkill
	if (SpecialSkillComp)
	{
		SpecialSkillComp->SkillPresetTable = SpecialSkillPresetTable;
		SpecialSkillComp->InitFromFinalStats(FinalStats);
	}

	UpdateHPBarLocation();
	RefreshHPBar();
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

    if (EventInstigator)
    {
        APawn* InstigatorPawn = EventInstigator->GetPawn();
        if (InstigatorPawn)
        {
            // WeaponComponent 탐색
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

		FVector DamageLoc =
			GetActorLocation()
			+ FVector(XOffset, 0.f, BaseZ + 20.f);

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

	if (SpecialSkillComp && SpecialSkillComp->TryStartOnDeath(this))

	{
		SpecialSkillComp->TryStartOnDeath(this);

		// ✅ CancelActiveSkill이 C++에 없어도 빌드되게 보호 호출
		// (있으면 호출, 없으면 무시)
		CallUFunctionIfExists(SpecialSkillComp, TEXT("CancelActiveSkill"));
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