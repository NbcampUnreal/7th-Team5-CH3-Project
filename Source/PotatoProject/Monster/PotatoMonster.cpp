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
#include "../UI/HealthBar.h"

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

// ------------------------------
// SFX: Distance Gate + Global Budget + Spawn
// ------------------------------

// 거리 기반 확률(near=100%, far=FarChance)
static float ComputeDistanceChance(float Dist, float NearDist, float FarDist, float FarChance)
{
	NearDist = FMath::Max(0.f, NearDist);
	FarDist = FMath::Max(NearDist + 1.f, FarDist);
	FarChance = FMath::Clamp(FarChance, 0.f, 1.f);

	if (Dist <= NearDist) return 1.0f;
	if (Dist >= FarDist)  return FarChance;

	const float Alpha = (Dist - NearDist) / (FarDist - NearDist);
	return FMath::Lerp(1.0f, FarChance, Alpha);
}

static bool PassDistanceGate(const UObject* WorldContextObject, const FVector& SoundLoc,
	float NearDist, float FarDist, float FarChance)
{
	if (!WorldContextObject) return false;

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
	if (!PlayerPawn)
	{
		// 테스트/특수 상황: 일단 허용
		return true;
	}

	const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), SoundLoc);
	const float Chance = ComputeDistanceChance(Dist, NearDist, FarDist, FarChance);
	return FMath::FRand() <= Chance;
}

// 전역 시간창 버짓: windowSec 동안 maxCount만 허용
static bool PassGlobalSFXBudget(float Now, float WindowSec, int32 MaxCount, TArray<float>& Times)
{
	// PIE/월드 재시작으로 시간이 역전될 수 있으니 보호
	static float G_LastNow = 0.f;
	if (Now + 0.01f < G_LastNow)
	{
		Times.Reset();
	}
	G_LastNow = Now;

	for (int32 i = Times.Num() - 1; i >= 0; --i)
	{
		if (Now - Times[i] > WindowSec)
		{
			Times.RemoveAtSwap(i, 1, false);
		}
	}

	if (Times.Num() >= MaxCount)
	{
		return false;
	}

	Times.Add(Now);
	return true;
}

// 전역 버짓 히스토리
static TArray<float> G_HitSFXTimes;
static TArray<float> G_DeathSFXTimes;

// Slot 재생 (랜덤 없음) + Attenuation/Concurrency 적용
static void SpawnSFXSlotAtLocation(const UObject* WorldContextObject, const FPotatoSFXSlot& Slot, const FVector& Location)
{
	if (!WorldContextObject) return;
	if (!Slot.Sound) return;

	const float Volume = FMath::Max(0.f, Slot.VolumeMultiplier);
	const float Pitch = FMath::Max(0.01f, Slot.PitchMultiplier);

	UGameplayStatics::SpawnSoundAtLocation(
		WorldContextObject,
		Slot.Sound,
		Location,
		FRotator::ZeroRotator,
		Volume,
		Pitch,
		0.0f,
		Slot.Attenuation,
		Slot.Concurrency,
		true
	);
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
// Hit React
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

	const FVector Loc = GetActorLocation();

	// ------------------------------------------------------------
	// A) 이미 재생 중이면: 몽타주는 유지 + 스턴만 연장 (+ SFX는 쿨타임/버짓/거리게이트로 제한)
	// ------------------------------------------------------------
	if (AnimInst->Montage_IsPlaying(AS->HitReactMontage))
	{
		// 이동 정지 유지
		DisableMovementSafe(this);

		//  연타 타격감용 SFX(너무 스팸 안 되게)
		if (Now - LastHitSFXTime >= HitSFXCooldown)
		{
			const bool bDistanceOK = PassDistanceGate(this, Loc, HitSFXNearDistance, HitSFXFarDistance, HitSFXFarChance);
			const bool bBudgetOK = PassGlobalSFXBudget(Now, /*Window*/0.08f, /*Max*/4, G_HitSFXTimes);

			if (bDistanceOK && bBudgetOK)
			{
				SpawnSFXSlotAtLocation(this, AS->GetHitSFX, Loc);
				LastHitSFXTime = Now;
			}
		}

		// 스턴 타이머 연장 (MinVisible 기준으로 길이 보정)
		const float BaseLen = AS->HitReactMontage->GetPlayLength();
		float FinalLen = BaseLen;
		ComputeMinVisiblePlayRate(BaseLen, HitReactMinVisibleTime, FinalLen);

		const float StunTime = FMath::Clamp(FinalLen, 0.1f, HitReactMaxStunTime);

		ScheduleTimerSafe(this, HitReactResumeTH,
			&APotatoMonster::ResumeMovementAfterHitReact,
			StunTime);

		return;
	}

	// ------------------------------------------------------------
	// B) 새로 HitReact 시작: 쿨타임으로 "새로 시작"만 제한
	// ------------------------------------------------------------
	if (Now - LastHitReactTime < HitReactCooldown) return;
	LastHitReactTime = Now;

	DisableMovementSafe(this);

	//  HitReact 시작 SFX 1회 (거리게이트 + 전역버짓)
	{
		const bool bDistanceOK = PassDistanceGate(this, Loc, HitSFXNearDistance, HitSFXFarDistance, HitSFXFarChance);
		const bool bBudgetOK = PassGlobalSFXBudget(Now, /*Window*/0.08f, /*Max*/4, G_HitSFXTimes);

		if (bDistanceOK && bBudgetOK)
		{
			SpawnSFXSlotAtLocation(this, AS->GetHitSFX, Loc);
			LastHitSFXTime = Now;
		}
	}

	// 몽타주 길이 보정(너무 짧으면 느리게 재생해서 최소 노출시간 확보)
	const float BaseLen = AS->HitReactMontage->GetPlayLength();
	float FinalLen = BaseLen;

	const float PlayRate = ComputeMinVisiblePlayRate(BaseLen, HitReactMinVisibleTime, FinalLen);

	const float PlayedLen = AnimInst->Montage_Play(AS->HitReactMontage, PlayRate);

	const float EffectiveLen =
		(PlayedLen > 0.f) ? FMath::Max(PlayedLen, FinalLen) : FinalLen;

	const float StunTime =
		FMath::Clamp(EffectiveLen, 0.1f, HitReactMaxStunTime);

	ScheduleTimerSafe(this,
		HitReactResumeTH,
		&APotatoMonster::ResumeMovementAfterHitReact,
		StunTime);
}

void APotatoMonster::ResumeMovementAfterHitReact()
{
	if (bIsDead) return;

	if (UCharacterMovementComponent* MoveComp =
		GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

// ============================================================
// Die
// ============================================================

void APotatoMonster::StopAIForDead()
{
	if (AAIController* AICon =
		Cast<AAIController>(GetController()))
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

	UWorld* World = GetWorld();
	const float Now = World ? World->TimeSeconds : 0.f;
	const FVector Loc = GetActorLocation();

	//  Death SFX: 1회 + (거리게이트) + (전역버짓) + (Attenuation/Concurrency)
	if (const UPotatoMonsterAnimSet* AS0 = GetAnimSet())
	{
		const bool bDistanceOK = PassDistanceGate(this, Loc, DeathSFXNearDistance, DeathSFXFarDistance, DeathSFXFarChance);
		const bool bBudgetOK = PassGlobalSFXBudget(Now, /*Window*/0.12f, /*Max*/2, G_DeathSFXTimes);

		if (bDistanceOK && bBudgetOK)
		{
			SpawnSFXSlotAtLocation(this, AS0->DeathSFX, Loc);
		}
	}

	// 남아있는 타이머 정리
	if (World)
	{
		GetWorldTimerManager().ClearTimer(HitReactResumeTH);
	}

	StopAIForDead();
	DisableMovementSafe(this);
	SetActorEnableCollision(false);

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetHiddenInGame(true);
	}

	float Life = 2.f;

	const UPotatoMonsterAnimSet* AS = GetAnimSet();
	if (AS && AS->DeathMontage)
	{
		if (UAnimInstance* AnimInst = GetAnimInstanceSafe(this))
		{
			AnimInst->StopAllMontages(0.05f);

			const float BaseLen = AS->DeathMontage->GetPlayLength();

			float FinalLen = BaseLen;
			const float PlayRate =
				ComputeMinVisiblePlayRate(
					BaseLen,
					DeathMinVisibleTime,
					FinalLen);

			const float PlayedLen =
				AnimInst->Montage_Play(
					AS->DeathMontage,
					PlayRate);

			const float EffectiveLen =
				(PlayedLen > 0.f)
				? FMath::Max(PlayedLen, FinalLen)
				: FinalLen;

			Life = FMath::Max(Life, EffectiveLen + 0.2f);
		}
	}

	SetLifeSpan(Life);
}

// ============================================================
// Lane
// ============================================================

void APotatoMonster::AdvanceLaneIndex()
{
	LaneIndex = FMath::Clamp(
		LaneIndex + 1,
		0,
		LanePoints.Num());
}

AActor* APotatoMonster::GetCurrentLaneTarget() const
{
	return LanePoints.IsValidIndex(LaneIndex)
		? LanePoints[LaneIndex]
		: WarehouseActor;
}

void APotatoMonster::SetAnimSet(UPotatoMonsterAnimSet* InSet)
{
	AnimSet = InSet;
}