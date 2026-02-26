#include "PotatoMonster.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "PotatoPresetApplier.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "PotatoCombatComponent.h"
#include "../UI/PotatoDamageTextPoolActor.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "../UI/HealthBar.h"
#include "Core/PotatoGameStateBase.h"

#include "FXUtils/PotatoFXUtils.h"

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

static void ScheduleTimerSafe(APotatoMonster* M,
	FTimerHandle& Handle,
	void(APotatoMonster::* Func)(),
	float Delay)
{
	if (!M) return;
	if (UWorld* W = M->GetWorld())
	{
		M->GetWorldTimerManager().ClearTimer(Handle);
		M->GetWorldTimerManager().SetTimer(Handle, M, Func, Delay, false);
	}
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

	const float Applied = FMath::Max(0.f, DamageAmount);
	if (Applied <= 0.f) return 0.f;

	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	RefreshHPBar();
	if (SpecialSkillComp && !SpecialSkill_OnHit.IsNone())
	{
		// 타겟 후보: DamageCauser가 있으면 그걸, 없으면 EventInstigator의 Pawn
		AActor* Target = DamageCauser;

		if (!Target && EventInstigator)
		{
			Target = EventInstigator->GetPawn();
		}

		// 그래도 없으면 현재 타겟(네 BB/멤버가 있다면)
		// if (!Target) Target = CurrentTarget; // 있으면 사용

		if (IsValid(Target))
		{
			SpecialSkillComp->TryStartSkill(SpecialSkill_OnHit, Target);
		}
	}
	// DamageText (기존 유지)
	if (DamageTextPool)
	{
		const float Now = GetWorld()->TimeSeconds;

		if (Now - LastDamageTime > DamageStackResetTime)
		{
			DamageStackIndex = 0;
		}

		LastDamageTime = Now;

		float BaseZ = GetMesh()
			? GetMesh()->Bounds.BoxExtent.Z
			: 80.f;

		const int32 Dir = (DamageStackIndex % 2 == 0) ? 1 : -1;
		const float XOffset =
			Dir * DamageStackOffsetStep * (DamageStackIndex / 2 + 1);

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
			const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, 0.08f, 4, PotatoFX::EGlobalBudgetChannel::HitSFX);

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
			const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, HitVFXGlobalWindowSec, HitVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::HitVFX);

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

		const float StunTime = FMath::Clamp(FinalLen, 0.1f, HitReactMaxStunTime);

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
		const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, 0.08f, 4, PotatoFX::EGlobalBudgetChannel::HitSFX);

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
			const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, HitVFXGlobalWindowSec, HitVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::HitVFX);

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
	const float StunTime = FMath::Clamp(EffectiveLen, 0.1f, HitReactMaxStunTime);

	ScheduleTimerSafe(this, HitReactResumeTH, &APotatoMonster::ResumeMovementAfterHitReact, StunTime);
}

void APotatoMonster::ResumeMovementAfterHitReact()
{
	if (bIsDead) return;

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

	// 캡슐은 끄고, 메쉬가 물리로 굴러가게
	if (UCapsuleComponent* Cap = M->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Cap->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	if (USkeletalMeshComponent* Mesh = M->GetMesh())
	{
		// 캐릭터는 보통 캡슐이 Root라서, 메쉬를 분리해줘야 "즉시 멈춤/튕김"이 덜함
		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// Ragdoll 프로파일은 프로젝트 기본값(PhysicsAsset 기준) 사용하는 게 가장 안전
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));

		Mesh->SetAllBodiesSimulatePhysics(true);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
		Mesh->bBlendPhysics = true;

		// 혹시 남아있는 애니 영향 최소화
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

	if (SpecialSkillComp && !SpecialSkill_OnDeath.IsNone())
	{
		// 분열은 Target이 꼭 필요 없으면 Self로 넘겨도 됨
		SpecialSkillComp->TryStartSkill(SpecialSkill_OnDeath, this);
	}
	UWorld* World = GetWorld();
	const float Now = World ? World->TimeSeconds : 0.f;

	const FVector DeathLoc = ComputeBoundsTopLocation(this, 0.2f, 0.f);
	if (SpecialSkillComp)
	{
		SpecialSkillComp->CancelSkill(ESpecialSkillCancelReason::Forced);
	}
	// Death SFX
	if (const UPotatoMonsterAnimSet* AS0 = GetAnimSet())
	{
		const bool bDistanceOK = PotatoFX::PassDistanceGate(this, GetActorLocation(), DeathSFXNearDistance, DeathSFXFarDistance, DeathSFXFarChance);
		const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, 0.12f, 2, PotatoFX::EGlobalBudgetChannel::DeathSFX);

		if (bDistanceOK && bBudgetOK)
		{
			PotatoFX::SpawnSFXSlotAtLocation(this, AS0->DeathSFX, GetActorLocation());
		}
	}

	// Death VFX
	if (const UPotatoMonsterAnimSet* AS0 = GetAnimSet())
	{
		const bool bHasDeathVFX = (AS0->DeathVFX.Niagara || AS0->DeathVFX.Cascade);
		if (bHasDeathVFX && (Now - LastDeathVFXTime >= DeathVFXCooldown))
		{
			const bool bDistanceOK = PotatoFX::PassDistanceGate(this, DeathLoc, DeathVFXNearDistance, DeathVFXFarDistance, DeathVFXFarChance);
			const bool bBudgetOK = PotatoFX::PassGlobalBudget(Now, DeathVFXGlobalWindowSec, DeathVFXGlobalMaxCount, PotatoFX::EGlobalBudgetChannel::DeathVFX);

			if (bDistanceOK && bBudgetOK)
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

	// 충돌은 "기본은 끄되", Ragdoll일 때는 메쉬 물리 충돌로 대체
	SetActorEnableCollision(false);

	float Life = 2.f;

	const UPotatoMonsterAnimSet* AS = GetAnimSet();

	// --------------------------------------------
	// (1) DeathMontage 있으면: 기존 몽타주 죽음
	// --------------------------------------------
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

	// --------------------------------------------
	// (2) DeathMontage 없으면: Ragdoll Fallback
	// --------------------------------------------
	EnableRagdollFallback(this);

	// 너무 오래 남기면 성능/가시성 문제 생길 수 있어서 별도 상한(필요시 변수로 빼도 됨)
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
	// 안정: Num-1까지만 유지 (그 다음은 GetCurrentLaneTarget()이 Warehouse로)
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