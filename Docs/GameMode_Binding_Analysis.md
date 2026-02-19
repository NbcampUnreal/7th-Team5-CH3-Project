`# GameMode 바인딩 & 시스템 연결 분석서

> **작성 목적**: 현재 구현된 코드를 기획서와 대조하여, GameMode를 중심으로 각 매니저/시스템이 **어떻게 연결(바인딩)되어야 하는지** 전체 흐름을 정리한다.
>
> **대상 브랜치**: `feature/level-design`

---

## 목차

1. [현재 구현 상태 요약](#1-현재-구현-상태-요약)
2. [전체 아키텍처 다이어그램](#2-전체-아키텍처-다이어그램)
3. [GameMode 페이즈별 바인딩 흐름](#3-gamemode-페이즈별-바인딩-흐름)
4. [시스템별 연결 상세](#4-시스템별-연결-상세)
5. [GameMode에 추가해야 할 코드 변경사항](#5-gamemode에-추가해야-할-코드-변경사항)
6. [바인딩 우선순위 & 구현 순서 제안](#6-바인딩-우선순위--구현-순서-제안)
7. [몬스터 웨이브 시스템 상세 분석](#7-몬스터-웨이브-시스템-상세-분석)

---

## 1. 현재 구현 상태 요약

### 1-1. 이미 구현된 시스템들

| 시스템 | 클래스 | 타입 | 상태 |
|--------|--------|------|------|
| **GameMode** | `APotatoGameMode` | `AGameMode` | ✅ 기본 뼈대 (페이즈 전환 + 델리게이트 Broadcast) |
| **낮/밤 사이클** | `UPotatoDayNightCycle` | `UWorldSubsystem` | ✅ 완성 (Day→Evening→Night→Dawn 순환) |
| **자원 매니저** | `UPotatoResourceManager` | `UWorldSubsystem` | ✅ 완성 (자원 CRUD + 생산 틱) |
| **생산 컴포넌트** | `UPotatoProductionComponent` | `UActorComponent` | ✅ 완성 (비용/생산/환급) |
| **건설 시스템** | `UBuildingSystemComponent` | `UActorComponent` | ✅ 구현됨 (Ghost + 그리드 스냅 + 배치) |
| **구조물** | `APotatoPlaceableStructure` | `AActor` | ✅ 구현됨 (HP, 파괴, 상호작용 인터페이스) |
| **동물 관리** | `UPotatoAnimalManagementComp` | `UActorComponent` | ✅ 구현됨 (스폰/제거/비용 차감) |
| **NPC 관리** | `UPotatoNPCManagementComp` | `UActorComponent` | ✅ 구현됨 (고용/해고/비용 차감) |
| **동물** | `APotatoAnimal` | `ACharacter` | ✅ 구현됨 (ProductionComp + AI 배회) |
| **NPC** | `APotatoNPC` | `ACharacter` | ✅ 구현됨 (유지비/퇴직 + AI 작업) |
| **몬스터** | `APotatoMonster` | `ACharacter` | ✅ 구현됨 (프리셋/Lane/전투) |
| **몬스터 스포너** | `APotatoMonsterSpawner` | `AActor` | ✅ 구현됨 (DataTable 기반 웨이브 큐) |
| **무기 컴포넌트** | `UPotatoWeaponComponent` | `UActorComponent` | ✅ 구현됨 (슬롯/발사/탄약) |
| **플레이어** | `APotatoPlayerCharacter` | `ACharacter` | ✅ 구현됨 (입력/카메라/이동/전투/건설) |

### 1-2. 현재 GameMode가 하고 있는 일

```
BeginPlay()
  └─ StartGame()
       ├─ DayNightSystem 서브시스템 참조
       │    ├─ OnDayStarted    → StartDayPhase()
       │    ├─ OnEveningStarted → StartWarningPhase()
       │    ├─ OnNightStarted  → StartNightPhase()
       │    └─ OnDawnStarted   → StartResultPhase()
       │
       └─ ResourceManager 서브시스템 참조
            └─ StartSystem(초기자원)
```

**현재 각 페이즈 함수 내부**: 델리게이트 `Broadcast()`만 하고 있음 → **실질적 게임 로직 없음**

### 1-3. 아직 연결되지 않은 것들 (GAP 분석)

| 기획서 요구사항 | 현재 상태 | 필요한 바인딩 |
|---------------|----------|-------------|
| 밤에 건물 배치 불가 (바리케이드만 2배 비용) | ❌ 미구현 | GameMode → BuildingSystem 페이즈 통보 |
| 밤에 자원 생산 중단 | ✅ ResourceManager에서 자체 체크 | 이미 동작 (`Day`가 아니면 생산 스킵) |
| 밤에 웨이브 스폰 시작 | ❌ 미연결 | GameMode → MonsterSpawner.StartWave() |
| 밤 종료 시 몬스터 정리 | ❌ 미구현 | GameMode → 잔여 몬스터 Destroy |
| Dawn(결과) 시 NPC 유지비 차감 | ❌ 미연결 | GameMode → NPC.TryPayMaintenance() 순회 |
| Dawn 시 생존 보상 지급 | ❌ 미구현 | GameMode → ResourceManager.AddResource() |
| 플레이어 사망 → 게임 오버 | ❌ 미연결 | PlayerCharacter 사망 → GameMode.EndGame() |
| 창고 파괴 → 게임 오버 | ❌ 미연결 | Warehouse 파괴 이벤트 → GameMode.EndGame() |
| 10 Day 생존 → 승리 | ✅ 뼈대 있음 | CheckVictoryCondition()이 호출되긴 하나, EndGame() 내부 비어있음 |
| 동물 배치 - 낮에만 가능 | ❌ 미구현 | 페이즈 체크 필요 |
| 30초 경고 (Evening) | ✅ 페이즈 존재 | UI 바인딩 필요 |

---

## 2. 전체 아키텍처 다이어그램

```
┌─────────────────────────────────────────────────────────────────┐
│                      APotatoGameMode                            │
│                    (오케스트레이터 역할)                          │
│                                                                 │
│  ┌──────────────┐    ┌──────────────────┐    ┌───────────────┐  │
│  │ DayNight     │    │ ResourceManager  │    │ MonsterSpawner│  │
│  │ (Subsystem)  │    │ (Subsystem)      │    │ (Actor)       │  │
│  │              │    │                  │    │               │  │
│  │ OnDay─────────────→ 생산 시작        │    │ StartWave()   │  │
│  │ OnEvening────────→ (경고 알림)       │    │ StopWave()    │  │
│  │ OnNight──────────→ 생산 중단 (자체)  ────→ 웨이브 시작    │  │
│  │ OnDawn───────────→ NPC 유지비 정산   │    │               │  │
│  └──────────────┘    └──────────────────┘    └───────────────┘  │
│         │                     │                      │          │
│         ▼                     ▼                      ▼          │
│  ┌──────────────┐    ┌──────────────────┐    ┌───────────────┐  │
│  │ Building     │    │ Production       │    │ Monster       │  │
│  │ System       │    │ Component        │    │ (Character)   │  │
│  │ (Component)  │    │ (on Structures)  │    │               │  │
│  │              │    │                  │    │ Die() ────────────→ GameMode 킬카운트
│  │ 배치 규칙    │    │ Register/        │    │               │  │
│  │ 변경 수신    │    │ Unregister       │    └───────────────┘  │
│  └──────────────┘    └──────────────────┘                       │
│         │                                                       │
│         ▼                                                       │
│  ┌──────────────┐    ┌──────────────────┐                       │
│  │ Placeable    │    │ AnimalMgmt /     │                       │
│  │ Structure    │    │ NPCMgmt          │                       │
│  │ (Actor)      │    │ (Components)     │                       │
│  │              │    │                  │                       │
│  │ 창고 파괴 ──────→ GameMode.EndGame() │                       │
│  │ 상호작용 ────────→ Animal/NPC 스폰   │                       │
│  └──────────────┘    └──────────────────┘                       │
│                                                                 │
│  ┌──────────────┐                                               │
│  │ Player       │                                               │
│  │ Character    │                                               │
│  │              │                                               │
│  │ 사망 ────────────→ GameMode.EndGame()                        │
│  └──────────────┘                                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. GameMode 페이즈별 바인딩 흐름

### 3-1. Day Phase (낮) — `StartDayPhase()`

```
StartDayPhase() 호출 시:
│
├─ 1. 건설 모드 제한 해제
│     → BuildingSystemComponent에 bDayPhase = true 통보
│     → 모든 카테고리(Building + Barricade) 배치 허용
│     → 배치 비용 = 정상 비용
│
├─ 2. 동물/NPC 배치 허용
│     → AnimalManagementComp / NPCManagementComp에 bCanPlace = true
│
├─ 3. 자원 생산 활성화 (자동)
│     → ResourceManager는 자체적으로 DayPhase 체크하므로 추가 작업 불필요
│
├─ 4. 잔여 몬스터 정리 (이전 밤에서 남은 것 처리)
│     → GetAllActorsOfClass<APotatoMonster>() → Destroy()
│     → 또는 Dawn 단계에서 처리 완료 확인
│
├─ 5. UI 업데이트
│     → OnDayPhase.Broadcast()
│     → HUD: "DAY {N} - {남은시간}" 표시
│
└─ 6. BGM 전환
      → 평화로운 낮 BGM 재생
```

### 3-2. Warning Phase (저녁/경고) — `StartWarningPhase()`

```
StartWarningPhase() 호출 시:
│
├─ 1. 경고 UI 표시
│     → OnWarningPhase.Broadcast()
│     → HUD: "30초 후 밤이 됩니다!" 깜빡임 효과
│
├─ 2. 경고 사운드 재생
│     → SFX: Warning_Bell 또는 유사 효과음
│
└─ 3. (선택) 건설 모드 유지
      → 아직 낮이므로 건물 배치 가능 (마지막 기회)
```

### 3-3. Night Phase (밤) — `StartNightPhase()`

```
StartNightPhase() 호출 시:
│
├─ 1. 건설 모드 제한 적용
│     → Building 카테고리: 배치 불가
│     → Barricade 카테고리: 배치 가능 (비용 2배)
│     → 현재 건설 모드 활성화 중이면 강제 종료
│
├─ 2. 동물/NPC 배치 제한
│     → AnimalManagementComp / NPCManagementComp에 bCanPlace = false
│
├─ 3. 자원 생산 중단 (자동)
│     → ResourceManager가 자체적으로 처리
│
├─ 4. ★ 몬스터 웨이브 시작 ★
│     → CurrentDay 기반으로 WaveId 계산
│     → MonsterSpawner->StartWave(WaveId)
│     │
│     │  웨이브 ID 계산 예시:
│     │  Day 1 → "1-1", "1-2", "1-3" (순차 또는 동시)
│     │  Day 2 → "2-1", "2-2", "2-3"
│     │
│     │  ※ 현재 MonsterSpawner는 단일 WaveId를 받으므로,
│     │     서브웨이브 처리를 위한 추가 로직 필요
│     │     (타이머로 "1-2", "1-3" 순차 호출하는 방식 권장)
│
├─ 5. UI 업데이트
│     → OnNightPhase.Broadcast()
│     → HUD: "NIGHT - WAVE 1/3 - {남은시간}" 표시
│
├─ 6. BGM 전환
│     → 긴장감 있는 전투 BGM
│
└─ 7. (선택) 시각 효과
      → 조명 전환 (DirectionalLight 강도 감소)
      → 안개 효과 적용
```

### 3-4. Dawn Phase (새벽/결과) — `StartResultPhase()`

```
StartResultPhase() 호출 시:
│
├─ 1. 몬스터 스포너 정지
│     → MonsterSpawner->StopWave()
│
├─ 2. 잔여 몬스터 정리
│     → 월드 내 모든 APotatoMonster를 순회하여 Destroy
│
├─ 3. ★ NPC 유지비 정산 ★
│     → 월드 내 모든 APotatoNPC 순회
│     │
│     │  foreach NPC:
│     │    if NPC->TryPayMaintenance():
│     │      ResourceManager->RemoveResource(Livestock, NPC->MaintenanceCost)
│     │    else:
│     │      NPC->Retire()  // 자동 퇴직
│     │      → 경고 메시지 UI 출력
│
├─ 4. 생존 보상 지급
│     → CurrentDay 기반 보상 테이블 참조
│     → ResourceManager->AddResource(각 자원, 보상량)
│
├─ 5. 결과 UI 표시
│     → 처치 수, 생존 Day, 획득 자원 표시
│     → 짧은 시간 후 자동으로 다음 Day로 전환 (Dawn 지속시간 동안)
│
├─ 6. Day 카운터 증가
│     → CurrentDay++
│
└─ 7. 승리 조건 체크
      → CheckVictoryCondition()
      → CurrentDay > 10 이면 EndGame() (승리)
```

---

## 4. 시스템별 연결 상세

### 4-1. GameMode ↔ MonsterSpawner 연결

**현재 상태**: 연결 없음

**필요한 바인딩**:

```cpp
// GameMode.h에 추가
private:
    UPROPERTY()
    APotatoMonsterSpawner* MonsterSpawner;

    // 현재 진행 중인 서브웨이브 인덱스
    int32 CurrentSubWaveIndex = 0;
    int32 MaxSubWavesPerDay = 3;

    // 서브웨이브 간 대기 타이머
    FTimerHandle SubWaveTimerHandle;

    void StartNextSubWave();
    void OnWaveComplete();  // 스포너가 웨이브 끝나면 콜백
```

**바인딩 방식**:
1. `BeginPlay()`에서 월드 내 `APotatoMonsterSpawner`를 `FindActor` 또는 에디터 참조로 획득
2. `StartNightPhase()`에서 `MonsterSpawner->StartWave(FName("1-1"))` 호출
3. 서브웨이브는 타이머로 60초 간격 순차 호출:
   - `StartWave("N-1")` → 60초 후 → `StartWave("N-2")` → 60초 후 → `StartWave("N-3")`

**웨이브 완료 감지 (추가 구현 필요)**:
- `APotatoMonsterSpawner`에 `OnWaveFinished` 델리게이트 추가 권장
- 또는 `APotatoMonster::Die()` 시 카운트 감소 → 0이 되면 콜백

```
Day 1 Night:
  StartWave("1-1") ──0초──→ 일반 5마리
  StartWave("1-2") ──60초──→ 일반 8마리
  StartWave("1-3") ──120초──→ 일반 12마리

Day 2 Night:
  StartWave("2-1") ──0초──→ 일반 5 + 엘리트 2
  ...
```

### 4-2. GameMode ↔ BuildingSystem 연결

**현재 상태**: `BuildingSystemComponent`는 페이즈 인식 없이 항상 배치 가능

**필요한 바인딩**:

```
GameMode.OnNightPhase → BuildingSystem에 제한 적용
GameMode.OnDayPhase   → BuildingSystem에 제한 해제
```

**구현 방안 A (권장 — Subsystem 조회 방식)**:
- `BuildingSystemComponent::OnPlaceStructure()`에서 배치 시점에 `UPotatoDayNightCycle`을 조회하여 현재 페이즈 체크
- 밤이면 `StructureData->Category == Building`일 때 거부
- 밤이면 `StructureData->Category == Barricade`일 때 비용 2배 적용

```
OnPlaceStructure()
  ├─ Phase = DayNightCycle->GetCurrentPhase()
  ├─ if (Phase == Night && Category == Building) → 거부
  ├─ if (Phase == Night && Category == Barricade) → 비용 * 2
  └─ else → 정상 배치
```

**구현 방안 B (델리게이트 방식)**:
- GameMode의 `OnNightPhase`/`OnDayPhase`를 BuildingSystem이 바인딩
- 내부 `bIsNight` 플래그 설정

### 4-3. GameMode ↔ NPC 유지비 정산

**현재 상태**: `APotatoNPC::TryPayMaintenance()` 구현됨, 그러나 **호출하는 곳이 없음**

**필요한 바인딩**:

```
StartResultPhase() (Dawn) 에서:
  1. 월드 내 모든 APotatoNPC를 수집
  2. 각 NPC에 대해 TryPayMaintenance() 호출
  3. 실패 시 Retire() 호출
```

**주의사항**:
- 현재 `TryPayMaintenance()`는 `HasEnoughResource` 체크만 하고 **실제 차감을 하지 않음**
- 수정 필요: 축산물이 충분하면 `RemoveResource(Livestock, MaintenanceCost)` 호출해야 함

```cpp
// PotatoNPC.cpp - TryPayMaintenance() 수정 필요
bool APotatoNPC::TryPayMaintenance()
{
    UPotatoResourceManager* RM = GetWorld()->GetSubsystem<UPotatoResourceManager>();
    if (!RM) return false;

    if (!RM->HasEnoughResource(EResourceType::Livestock, MaintenanceCostLivestock))
        return false;

    // ★ 이 줄이 빠져있음 → 추가 필요 ★
    RM->RemoveResource(EResourceType::Livestock, MaintenanceCostLivestock);
    return true;
}
```

### 4-4. GameMode ↔ 게임 오버 조건

**패배 조건 1: 플레이어 사망**

```
APotatoPlayerCharacter
  └─ TakeDamage() 또는 Die()  ← 현재 미구현
       └─ GameMode->EndGame(EGameOverReason::PlayerDeath)
```

- `APotatoPlayerCharacter`에 HP 시스템 추가 필요
- 사망 시 GameMode에 통보

**패배 조건 2: 창고(Warehouse) 파괴**

```
APotatoPlaceableStructure (창고)
  └─ TakeDamage() → HP <= 0
       └─ if (Tag == "Target" 또는 bIsWarehouse)
            → GameMode->EndGame(EGameOverReason::WarehouseDestroyed)
```

- 현재 `APotatoPlaceableStructure::TakeDamage()`에서 `Destroy()` 호출됨
- 창고 식별 태그 또는 플래그 추가 필요
- `Destroy()` 전에 GameMode 통보

**GameMode.EndGame() 구현 필요**:

```
EndGame(Reason):
  ├─ MonsterSpawner->StopWave()
  ├─ DayNightSystem->EndSystem()
  ├─ ResourceManager->EndSystem()
  ├─ 결과 UI 표시 (통계: 생존 회차, 처치 수 등)
  └─ 선택지 제공:
       ├─ Restart → 해당 Day 낮으로 복귀 (Checkpoint)
       └─ Main Menu → 타이틀 화면
```

### 4-5. GameMode ↔ 몬스터 처치 추적

**현재 상태**: `APotatoMonster::Die()`는 존재하지만 GameMode에 통보하지 않음

**필요한 바인딩**:

```
APotatoMonster::Die()
  └─ GameMode에 처치 통보
       ├─ KillCount++ (통계용)
       └─ AliveMonsterCount-- → 0이면 웨이브 완료 판정
```

**구현 방안**:
- GameMode에 `OnMonsterKilled(APotatoMonster*)` 함수 추가
- `APotatoMonster::Die()`에서 `GameMode->OnMonsterKilled(this)` 호출
- 또는 `FOnMonsterDied` 델리게이트를 Spawner에 두고 GameMode가 바인딩

### 4-6. 자원 생산 흐름 (이미 작동하는 부분)

```
건물/동물/NPC 스폰
  └─ ProductionComponent::BeginPlay()
       └─ ResourceManager->RegisterProduction(Wood, Stone, Crop, Livestock)

ResourceManager::OnProductionTick() (매 1초)
  ├─ Phase == Day 인지 체크 (자체)
  ├─ AccumulateAndFlush() 로 초당 생산량 누적
  └─ 정수 단위가 되면 AddResource() → OnResourceChanged Broadcast
```

✅ **이 흐름은 이미 정상 동작**. GameMode 추가 개입 불필요.

---

## 5. GameMode에 추가해야 할 코드 변경사항

### 5-1. 헤더 (`PotatoGameMode.h`) 추가 항목

```cpp
// === 새로 추가할 Forward Declarations ===
class APotatoMonsterSpawner;
class APotatoMonster;
class APotatoNPC;
class APotatoPlaceableStructure;

// === 새로 추가할 Delegate ===
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOver, bool, bIsVictory);

// === 새로 추가할 멤버 (GameMode 클래스 내부) ===

#pragma region WaveSystem
private:
    // 레벨에 배치된 MonsterSpawner 참조
    UPROPERTY(EditInstanceOnly, Category = "Wave")
    APotatoMonsterSpawner* MonsterSpawner;

    int32 CurrentSubWaveIndex = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Wave")
    int32 MaxSubWavesPerDay = 3;

    // 서브웨이브 간격 (초)
    UPROPERTY(EditDefaultsOnly, Category = "Wave")
    float SubWaveInterval = 60.0f;

    FTimerHandle SubWaveTimerHandle;

    void StartNextSubWave();

    // 몬스터 킬카운트
    int32 TotalKillCount = 0;
    int32 AliveMonsterCount = 0;

public:
    void OnMonsterKilled(APotatoMonster* Monster);
    void OnMonsterSpawned(APotatoMonster* Monster);

#pragma endregion WaveSystem

#pragma region GameOverSystem
public:
    UPROPERTY(BlueprintAssignable)
    FOnGameOver OnGameOver;

    void HandlePlayerDeath();
    void HandleWarehouseDestroyed();

private:
    // 창고 참조 (레벨 배치)
    UPROPERTY(EditInstanceOnly, Category = "Core")
    APotatoPlaceableStructure* WarehouseStructure;

    bool bGameOver = false;

#pragma endregion GameOverSystem

#pragma region NPCMaintenance
private:
    void ProcessNPCMaintenance();
    void DistributeNightSurvivalReward();

#pragma endregion NPCMaintenance
```

### 5-2. 소스 (`PotatoGameMode.cpp`) 페이즈 함수 내용

```
StartGame():
  (기존) + MonsterSpawner 참조 획득
        + WarehouseStructure 참조 획득
        + Warehouse에 파괴 콜백 바인딩

StartDayPhase():
  ├─ 잔여 몬스터 클린업 (안전장치)
  ├─ SubWaveTimer 클리어
  ├─ CurrentSubWaveIndex 리셋
  └─ OnDayPhase.Broadcast()

StartWarningPhase():
  └─ OnWarningPhase.Broadcast()  (기존과 동일)

StartNightPhase():
  ├─ CurrentSubWaveIndex = 0
  ├─ StartNextSubWave()   ← 첫 번째 서브웨이브 즉시 시작
  └─ OnNightPhase.Broadcast()

StartNextSubWave():
  ├─ WaveId = FName(FString::Printf("%d-%d", CurrentDay, CurrentSubWaveIndex+1))
  ├─ MonsterSpawner->StartWave(WaveId)
  ├─ CurrentSubWaveIndex++
  └─ if (CurrentSubWaveIndex < MaxSubWavesPerDay):
       SetTimer(SubWaveTimerHandle, SubWaveInterval, StartNextSubWave)

StartResultPhase():
  ├─ MonsterSpawner->StopWave()
  ├─ 잔여 몬스터 Destroy
  ├─ ProcessNPCMaintenance()
  ├─ DistributeNightSurvivalReward()
  ├─ CurrentDay++
  ├─ CheckVictoryCondition()
  └─ OnResultPhase.Broadcast()

ProcessNPCMaintenance():
  ├─ TArray<AActor*> NPCs;
  ├─ GetAllActorsOfClass<APotatoNPC>(NPCs)
  └─ foreach NPC:
       if (!NPC->TryPayMaintenance()):
         NPC->Retire()

DistributeNightSurvivalReward():
  └─ ResourceManager->AddResource(각종 보상)

EndGame():
  ├─ if (bGameOver) return;
  ├─ bGameOver = true
  ├─ DayNightSystem->EndSystem()
  ├─ MonsterSpawner->StopWave()
  └─ OnGameOver.Broadcast(bIsVictory)
```

---

## 6. 바인딩 우선순위 & 구현 순서 제안

### Phase 1 — 핵심 루프 완성 (최우선)

| 순서 | 작업 | 관련 파일 | 예상 난이도 |
|------|------|----------|-----------|
| 1 | GameMode ↔ MonsterSpawner 연결 | `PotatoGameMode.h/cpp` | ⭐⭐ |
| 2 | Night 시작 시 웨이브 ID 생성 & 호출 | `PotatoGameMode.cpp` | ⭐ |
| 3 | Dawn 시 몬스터 정리 | `PotatoGameMode.cpp` | ⭐ |
| 4 | Dawn 시 NPC 유지비 정산 연결 | `PotatoGameMode.cpp` + `PotatoNPC.cpp` (TryPayMaintenance 수정) | ⭐⭐ |
| 5 | EndGame() 기본 구현 | `PotatoGameMode.cpp` | ⭐ |

> **Phase 1 완료 시**: 낮→건설→밤→웨이브→결과→다음날 의 **기본 게임 루프가 완성**됨

### Phase 2 — 게임 오버 & 제한 조건

| 순서 | 작업 | 관련 파일 | 예상 난이도 |
|------|------|----------|-----------|
| 6 | 창고 파괴 → GameMode 통보 | `PotatoPlaceableStructure.cpp` | ⭐ |
| 7 | 플레이어 HP/사망 시스템 | `PotatoPlayerCharacter.h/cpp` | ⭐⭐⭐ |
| 8 | 건설 제한 (밤: Building 불가, Barricade 2배) | `BuildingSystemComponent.cpp` | ⭐⭐ |
| 9 | 동물/NPC 배치 페이즈 제한 | `PotatoAnimalManagementComp.cpp` / `PotatoNPCManagementComp.cpp` | ⭐ |

### Phase 3 — 폴리시 & 통계

| 순서 | 작업 | 관련 파일 | 예상 난이도 |
|------|------|----------|-----------|
| 10 | 몬스터 처치 카운트 추적 | `PotatoMonster.cpp` + `PotatoGameMode.cpp` | ⭐ |
| 11 | 생존 보상 테이블 구현 | `PotatoGameMode.cpp` (또는 DataTable) | ⭐⭐ |
| 12 | 게임 오버/승리 UI 연동 | Widget Blueprint + `PotatoGameMode.cpp` | ⭐⭐ |
| 13 | Restart (해당 Day 낮으로) | `CheckpointManager` 또는 간단한 상태 복원 | ⭐⭐⭐⭐ |

---

## 7. 몬스터 웨이브 시스템 상세 분석

### 7-1. 웨이브 데이터 구조 (DataTable)

게임의 웨이브 시스템은 **두 개의 DataTable**로 구성된다.

```
┌──────────────────────────────────────────────────────┐
│  WaveMetaTable (FPotatoWaveMetaRow)                  │
│  ─ 웨이브 단위의 메타데이터                            │
│                                                      │
│  Round (int32)         : 라운드 번호 (Day)             │
│  WaveId (FName)        : 웨이브 고유 ID ("1-1", "2-3")│
│  TimeLimit (float)     : 웨이브 제한시간 (초)           │
│  PreDelay (float)      : 시작 전 대기시간              │
│  SpawnInterval (float) : 몬스터 간 스폰 간격 (초)       │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│  WaveSpawnTable (FPotatoWaveSpawnRow)                 │
│  ─ 웨이브 내 몬스터 스폰 정의 (여러 행)                 │
│                                                      │
│  WaveId (FName)        : 소속 웨이브 ID                │
│  Rank (EMonsterRank)   : Normal / Elite / Boss        │
│  Type (EMonsterType)   : 11종 몬스터 타입               │
│  Count (int32)         : 해당 타입 스폰 수              │
│  Delay (float)         : 추가 지연 (초)                │
│  SpawnGroup (FName)    : 레인 그룹 이름                 │
└──────────────────────────────────────────────────────┘
```

**관계**: 하나의 WaveId에 여러 SpawnRow가 매핑됨.

```
WaveId "1-1"
  ├─ Row A: Normal, Skeleton, Count=5, SpawnGroup="LaneA"
  └─ Row B: Normal, Zombie,   Count=3, SpawnGroup="LaneB"

WaveId "1-2"
  ├─ Row C: Normal, Skeleton, Count=8, SpawnGroup="LaneA"
  └─ Row D: Elite,  Ogre,     Count=1, SpawnGroup="LaneA"
```

### 7-2. 몬스터 스포너 내부 파이프라인

`APotatoMonsterSpawner`의 전체 스폰 흐름:

```
GameMode.StartNightPhase()
  │
  ▼
MonsterSpawner->StartWave(FName WaveId)
  │
  ├─ 1. WaveMetaTable에서 WaveId로 메타 조회
  │     → PreDelay, SpawnInterval, TimeLimit 획득
  │
  ├─ 2. BuildQueue(WaveId) 호출
  │     └─ WaveSpawnTable 전체 순회
  │          └─ WaveId가 일치하는 Row만 필터
  │               └─ Row.Count만큼 FPendingSpawn 생성
  │                    { MonsterClass, Rank, Type, Delay, SpawnGroup }
  │               └─ PendingQueue에 추가
  │
  ├─ 3. bWaveActive = true
  │     WaveTimer 시작 (SpawnInterval 간격)
  │
  ▼
TickSpawn() (SpawnInterval 초마다 호출)
  │
  ├─ PendingQueue가 비어있으면:
  │     → StopWave() 호출
  │     → UE_LOG("Wave finished")
  │     → ★ 현재 여기서 끝남 — 외부 통보 없음 ★
  │
  └─ PendingQueue에서 Pop
       └─ SpawnOne(Entry) 호출
            │
            ├─ SpawnActorDeferred<APotatoMonster>()
            │
            ├─ ★ 데이터 주입 (FinishSpawning 전) ★
            │     Monster->SetRank(Entry.Rank)
            │     Monster->SetType(Entry.Type)
            │     Monster->WaveBaseHP = BaseHP설정
            │     Monster->WarehouseActor = WarehouseActor
            │     Monster->TypePresetTable = TypePresetTable
            │     Monster->RankPresetTable = RankPresetTable
            │     Monster->SpecialSkillPresetTable = SpecialSkillPresetTable
            │
            ├─ ★ 레인 포인트 주입 ★
            │     LanePathProvider->GetLanePoints(SpawnGroup, OutPoints)
            │     Monster->LanePoints = OutPoints
            │
            └─ FinishSpawning()
                 → 이후 AI 자동 Possess (AutoPossessAI = PlacedInWorldOrSpawned)
```

### 7-3. 프리셋 파이프라인 (스폰 → 전투 준비 완료)

몬스터가 스폰된 후, AI Controller가 Possess되면서 **프리셋 시스템**이 작동한다.

```
FinishSpawning()
  │
  ▼
APotatoAIController::OnPossess(APawn* InPawn)
  │
  ├─ 1. Monster->ApplyPresetsOnce() 호출
  │     │
  │     └─ UPotatoPresetApplier::BuildFinalStats() 호출
  │          │
  │          ├─ TypePresetTable에서 Type으로 행 검색
  │          │     → BaseAnimSetId, HpMultiplier, MoveSpeedMultiplier
  │          │     → bIsRanged, AttackRange
  │          │     → RankPresetId (Rank별 매핑)
  │          │
  │          ├─ RankPresetTable에서 RankPresetId로 행 검색
  │          │     → AttackDamage, MoveSpeedRankRatio
  │          │     → BehaviorTree 오버라이드
  │          │     → SpecialSkillId (Optional)
  │          │
  │          ├─ SpecialSkillPresetTable에서 SpecialSkillId로 행 검색 (있을 경우)
  │          │     → SpecialLogic (FName), SpecialCooldown
  │          │     → SpecialDamageMultiplier
  │          │     → BehaviorTree 오버라이드 (우선순위 최고)
  │          │
  │          └─ FPotatoMonsterFinalStats 조합:
  │               {
  │                 MaxHP = WaveBaseHP * TypeHpMultiplier
  │                 AttackDamage = RankAttackDamage
  │                 AttackRange = TypeAttackRange
  │                 MoveSpeed = PlayerSpeed * RankRatio * TypeMultiplier
  │                 bIsRanged = TypeIsRanged
  │                 SpecialLogic = SpecialPreset.SpecialLogic
  │                 BehaviorTree = Special > Rank > Type 우선순위
  │                 AnimSet = Type.AnimSetId → AnimSet 에셋
  │               }
  │
  │     Monster->FinalStats = 결과
  │     Monster->CurrentHP = FinalStats.MaxHP
  │     Monster->MoveSpeed 적용
  │
  ├─ 2. UseBlackboard(FinalStats.BehaviorTree의 BB) 초기화
  │
  ├─ 3. RunBehaviorTree(FinalStats.BehaviorTree) 시작
  │
  └─ 4. Blackboard 키 초기화
        BB->SetValueAsObject("WarehouseActor", Monster->WarehouseActor)
        BB->SetValueAsVector("MoveTarget", Monster->GetCurrentLaneTarget())
        BB->SetValueAsVector("MoveGoal", WarehouseLocation)
        BB->SetValueAsBool("bIsDead", false)
        BB->SetValueAsName("SpecialLogic", FinalStats.SpecialLogic)
```

### 7-4. 몬스터 AI 행동 트리 구조

```
┌──────────────────────────────────────────────────┐
│            Behavior Tree (Root)                   │
│                                                   │
│  ┌─ Selector ─────────────────────────────────┐  │
│  │                                             │  │
│  │  ┌─ Sequence [Dead Check] ──────────────┐  │  │
│  │  │  Decorator: bIsDead == true           │  │  │
│  │  │  Task: Die / Cleanup                  │  │  │
│  │  └──────────────────────────────────────┘  │  │
│  │                                             │  │
│  │  ┌─ Sequence [Combat] ─────────────────┐   │  │
│  │  │  Service: BTService_UpdateAttackTarget│  │  │
│  │  │  Service: BTService_UpdateCombatState │  │  │
│  │  │                                      │  │  │
│  │  │  ┌─ Selector ──────────────────┐     │  │  │
│  │  │  │                              │     │  │  │
│  │  │  │  [공격 가능?]                │     │  │  │
│  │  │  │  Decorator: bInAttackRange   │     │  │  │
│  │  │  │  Task: BTTask_Attack         │     │  │  │
│  │  │  │                              │     │  │  │
│  │  │  │  [이동]                      │     │  │  │
│  │  │  │  Task: MoveTo(MoveTarget)    │     │  │  │
│  │  │  └──────────────────────────────┘     │  │  │
│  │  └──────────────────────────────────────┘  │  │
│  │                                             │  │
│  │  ┌─ Sequence [Lane Advance] ──────────┐   │  │
│  │  │  Task: BT_AdvanceLaneTarget        │   │  │
│  │  │  Task: MoveTo(MoveTarget)          │   │  │
│  │  └────────────────────────────────────┘   │  │
│  └─────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

**핵심 서비스 역할**:

| 서비스 | 역할 | 주기 |
|--------|------|------|
| `BTService_UpdateAttackTarget` | 복도(Corridor) 기반 차단 구조물 탐색, 접근 지점 계산, 공격 범위 판정 | 0.25초 |
| `BTService_UpdateCombatState` | `bIsDead`, `bIsAttacking` 플래그를 BB에 동기화 | 0.5초 |

**`BTService_UpdateAttackTarget` 상세 (핵심 타겟팅 로직)**:

```
매 Tick:
  │
  ├─ 1. 몬스터의 현재 위치 → 최종 목표(WarehouseActor) 방향으로 "복도" 정의
  │     복도 반폭 = CorridorHalfWidth (기본값 설정)
  │     복도 길이 = 현재 위치 → 창고 거리
  │
  ├─ 2. 복도 내 Overlap으로 Destructible 구조물 검색
  │     → ECC_GameTraceChannel1 (구조물 전용 트레이스 채널)
  │
  ├─ 3. 검색된 구조물 중 가장 가까운 것을 CurrentTarget으로 설정
  │     → BB["CurrentTarget"] = ClosestStructure
  │
  ├─ 4. 타겟이 있으면 접근 지점(ApproachPoint) 계산
  │     → 구조물 캡슐 + 몬스터 캡슐 간 거리 고려
  │     → BB["MoveTarget"] = ApproachPoint
  │     → BB["MoveGoal"] ≠ BB["MoveTarget"] (전투 상태 표시)
  │
  ├─ 5. 공격 범위 판정 (ComputeInAttackRange)
  │     → 캡슐 반지름 보정한 실 거리 계산
  │     → AttackRange 내이면 BB["bInAttackRange"] = true
  │
  └─ 6. 타겟이 없으면
        → BB["CurrentTarget"] = nullptr
        → BB["MoveTarget"] = 현재 LaneTarget (진행 방향)
        → BB["MoveGoal"] == BB["MoveTarget"] (비전투 상태)
```

### 7-5. 레인 시스템 상세

몬스터는 **레인 포인트**를 따라 이동하며, 모든 레인을 통과하면 최종적으로 **창고(Warehouse)**를 향한다.

```
스폰 위치 (SpawnPoint)
    │
    ▼
LanePoint[0]  ←──── BPI_LanePathProvider::GetLanePoints(SpawnGroup)
    │                  (블루프린트 인터페이스로 레벨에서 구현)
    ▼
LanePoint[1]
    │
    ▼
LanePoint[2]
    │
    ▼
  ... (N개)
    │
    ▼
WarehouseActor  ←── LanePoints를 모두 통과한 후 Fallback 타겟
```

**레인 진행 로직** (`BT_AdvanceLaneTarget`):

```
매 BT Task 실행:
  │
  ├─ 현재 LaneIndex의 LanePoint 위치와 몬스터 위치 비교
  │
  ├─ 거리 < 60 units (ArrivalThreshold)?
  │     ├─ YES → LaneIndex++
  │     │         └─ 새로운 MoveTarget 설정
  │     │
  │     └─ NO → 계속 이동
  │
  ├─ LaneIndex >= LanePoints.Num()?
  │     └─ YES → MoveTarget = WarehouseActor 위치
  │              (최종 목표 도달 모드)
  │
  └─ MoveGoal ≠ MoveTarget인 경우 (전투 중)?
        → 레인 진행 Skip (전투 완료 후 재개)
```

### 7-6. 몬스터 전투 컴포넌트 흐름

```
BTService_UpdateAttackTarget: bInAttackRange = true
  │
  ▼
BTTask_Attack: CombatComponent->RequestBasicAttack(Target)
  │
  ├─ AttackMontage 재생
  │
  ├─ [근접] AnimNotify AN_ApplyDamage 발생
  │     └─ CombatComponent->ApplyPendingBasicDamage()
  │          └─ Target->TakeDamage(AttackDamage * StructureDamageMultiplier)
  │
  ├─ [원거리] AnimNotify AN_FireProjectile 발생
  │     └─ CombatComponent->FirePendingRangedProjectile()
  │          └─ 프로젝타일 스폰 → 목표 지점 발사
  │
  └─ AnimNotify AN_EndBasicAttack 발생
        └─ CombatComponent->EndBasicAttack()
             └─ BB["bIsAttacking"] = false
```

**공격 대상 검증** (`IsAllowedAttackTarget`):

```
공격 가능한 대상:
  ├─ WarehouseActor (창고) → 항상 가능
  ├─ Destructible 구조물 → 가능
  └─ LanePoint / TargetPoint → 공격 불가 (무시)
```

### 7-7. 몬스터 사망 흐름 (현재 & 필요한 변경)

**현재 구현**:

```
APotatoMonster::Die()
  ├─ if (bIsDead) return;
  ├─ bIsDead = true
  ├─ 이동 중단 (StopMovementImmediately)
  ├─ 충돌 비활성화
  ├─ 사망 몽타주 재생
  └─ 타이머 → Destroy()

  ★ 문제: GameMode, Spawner 어디에도 통보하지 않음
  ★ AliveMonsterCount를 관리하는 곳이 없음
```

**필요한 변경**:

```
APotatoMonster::Die()  (수정 후)
  ├─ (기존 로직 유지)
  ├─ ★ 추가: GameMode->OnMonsterKilled(this) 호출
  │     │
  │     └─ APotatoGameMode::OnMonsterKilled(APotatoMonster* Monster)
  │          ├─ TotalKillCount++
  │          ├─ AliveMonsterCount--
  │          └─ if (AliveMonsterCount <= 0 && !bWaveSpawning)
  │               → 모든 서브웨이브 클리어 판정
  │
  └─ ★ 또는: OnMonsterDied 델리게이트 Broadcast
              → Spawner와 GameMode 양쪽에서 바인딩 가능
```

### 7-8. 웨이브 완료 감지 (현재 누락 — 구현 필요)

현재 `APotatoMonsterSpawner`는 PendingQueue가 비면 `StopWave()`를 호출하지만, 이는 **"스폰 완료"**일 뿐 **"웨이브 클리어"**가 아니다.

```
                        스폰 완료                  웨이브 클리어
                    (큐가 비었음)              (살아있는 몬스터 0)
                         │                           │
PendingQueue: ████████░░ │                           │
                         │                           │
Alive Count:  ▁▂▃▄▅▆▇█ │ █▇▆▅▄▃▂▁                  │
                         │                           │
                         ▼                           ▼
               Spawner.StopWave()           GameMode: 웨이브 클리어!
               (현재 여기까지만 있음)         (★ 구현 필요 ★)
```

**필요한 구현**:

```cpp
// 1. MonsterSpawner에 추가
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawningFinished);

UPROPERTY(BlueprintAssignable)
FOnSpawningFinished OnSpawningFinished;
// → PendingQueue 소진 시 Broadcast

// 2. GameMode에서 관리
int32 AliveMonsterCount = 0;
bool bSpawningComplete = false;

// 몬스터 스폰 시: AliveMonsterCount++
// 몬스터 사망 시: AliveMonsterCount--
// OnSpawningFinished 수신: bSpawningComplete = true
// 체크: if (bSpawningComplete && AliveMonsterCount <= 0) → 웨이브 클리어
```

### 7-9. GameMode ↔ 웨이브 전체 오케스트레이션

기획서 기준: **밤(Night) = 3분, 서브웨이브 3개**

```
Night Phase 시작 (180초)
│
├── t=0s ──── StartWave("N-1")
│             └─ WaveMeta에서 PreDelay, SpawnInterval 읽음
│             └─ 스폰 시작
│
├── t=60s ─── StartWave("N-2")
│             └─ 첫 번째 웨이브와 병행 가능
│             └─ 동시에 여러 그룹이 밀려옴
│
├── t=120s ── StartWave("N-3")
│             └─ 최종 웨이브 (보스급 포함 가능)
│
└── t=180s ── Night 종료 → Dawn 전환
              └─ StopWave() 호출
              └─ 잔여 몬스터 처리

Day별 서브웨이브 구성 예시:
┌──────────────────────────────────────────────────┐
│ Day │ Wave   │ 구성                               │
│─────│────────│────────────────────────────────────│
│  1  │ "1-1"  │ Normal Skeleton x5                 │
│     │ "1-2"  │ Normal Skeleton x8                 │
│     │ "1-3"  │ Normal Zombie x10                  │
│─────│────────│────────────────────────────────────│
│  3  │ "3-1"  │ Normal Mixed x10                   │
│     │ "3-2"  │ Normal x8 + Elite x2               │
│     │ "3-3"  │ Normal x12 + Elite x3              │
│─────│────────│────────────────────────────────────│
│  5  │ "5-1"  │ Normal x12 + Elite x3              │
│     │ "5-2"  │ Normal x15 + Elite x5              │
│     │ "5-3"  │ Normal x10 + Elite x5 + Boss x1    │
│─────│────────│────────────────────────────────────│
│ 10  │ "10-1" │ Elite x10                          │
│     │ "10-2" │ Elite x15 + Boss x2                │
│     │ "10-3" │ Elite x20 + Boss x3 (최종 보스)     │
└──────────────────────────────────────────────────┘
```

### 7-10. 프리셋 DataTable 구조 정리

몬스터의 최종 능력치는 **3단계 프리셋 합성**으로 결정된다.

```
                ┌─────────────────────────┐
                │   TypePresetTable       │
                │  (EMonsterType 기준)     │
                │                         │
                │  - HpMultiplier         │
                │  - MoveSpeedMultiplier  │
                │  - bIsRanged            │
                │  - AttackRange          │
                │  - BaseAnimSetId        │
                │  - NormalRankPresetId   │
                │  - EliteRankPresetId    │
                │  - BossRankPresetId     │
                └────────┬────────────────┘
                         │ Rank에 맞는 PresetId 선택
                         ▼
                ┌─────────────────────────┐
                │   RankPresetTable       │
                │  (PresetId 기준)         │
                │                         │
                │  - AttackDamage         │
                │  - MoveSpeedRankRatio   │
                │  - AnimSetOverrideId    │
                │  - BehaviorTreeOverride │
                │  - SpecialSkillId       │
                └────────┬────────────────┘
                         │ SpecialSkillId 있으면
                         ▼
                ┌─────────────────────────┐
                │  SpecialSkillPreset     │
                │  (SkillId 기준)          │
                │                         │
                │  - SpecialLogic (FName) │
                │  - SpecialCooldown      │
                │  - SpecialDmgMultiplier │
                │  - BehaviorTree (최우선) │
                └─────────────────────────┘
                         │
                         ▼
                ┌─────────────────────────┐
                │  FPotatoMonsterFinalStats│
                │  (최종 합성 결과)          │
                │                         │
                │  MaxHP                  │
                │  AttackDamage           │
                │  AttackRange            │
                │  MoveSpeed              │
                │  bIsRanged              │
                │  SpecialLogic           │
                │  SpecialCooldown        │
                │  SpecialDmgMultiplier   │
                │  StructureDmgMultiplier │
                │  BehaviorTree           │
                │  AnimSet                │
                └─────────────────────────┘
```

**BehaviorTree 우선순위**: `SpecialSkillPreset > RankPreset > TypePreset(기본 BT)`

### 7-11. 몬스터 랭크 & 타입 정리

**랭크 (EMonsterRank)**:

| 랭크 | 설명 | 특징 |
|------|------|------|
| `Normal` | 일반 몬스터 | 기본 능력치, 대량 출현 |
| `Elite` | 정예 몬스터 | 높은 HP/공격력, 특수 스킬 가능 |
| `Boss` | 보스 몬스터 | 최고 능력치, 반드시 특수 스킬 보유 |

**타입 (EMonsterType)** — 기획서 기준 11종:

| 타입 | 근/원거리 | 특징 |
|------|----------|------|
| `Skeleton` | 근접 | 기본 타입 |
| `Zombie` | 근접 | 느리지만 높은 HP |
| `Goblin` | 근접 | 빠른 이동 |
| `Ogre` | 근접 | 높은 공격력 |
| `Wraith` | 원거리 | 투사체 공격 |
| ... (나머지 타입) | | DataTable에서 확장 가능 |

### 7-12. 웨이브 시스템 GAP 요약 & 필요 작업

| 항목 | 현재 상태 | 필요한 작업 |
|------|----------|-----------|
| `StartWave(WaveId)` | ✅ 구현됨 | GameMode에서 호출만 하면 됨 |
| `StopWave()` | ✅ 구현됨 | Dawn에서 호출 |
| `BuildQueue()` + DataTable 필터 | ✅ 구현됨 | — |
| `SpawnOne()` + Deferred Spawn | ✅ 구현됨 | — |
| 프리셋 파이프라인 | ✅ 구현됨 | DataTable 에셋 제작 필요 |
| 레인 포인트 주입 | ✅ 구현됨 | 레벨에 BPI_LanePathProvider 구현체 배치 필요 |
| AI BehaviorTree + 서비스 | ✅ 구현됨 | BT 에셋 + BB 에셋 제작 필요 |
| **`OnWaveFinished` / `OnSpawningFinished` 델리게이트** | ❌ 없음 | Spawner에 추가 |
| **`Die()` → GameMode 통보** | ❌ 없음 | Monster.Die()에서 GameMode 호출 또는 델리게이트 |
| **AliveMonsterCount 추적** | ❌ 없음 | GameMode에서 Spawn/Kill 카운트 관리 |
| **서브웨이브 순차 호출 (타이머)** | ❌ 없음 | GameMode.StartNextSubWave() 구현 |
| **웨이브 클리어 판정** | ❌ 없음 | Spawning 완료 + Alive=0 → 클리어 |
| **밤 종료 시 잔여 몬스터 정리** | ❌ 없음 | Dawn에서 GetAllActorsOfClass → Destroy |
| **킬 카운트 / 통계** | ❌ 없음 | GameMode에서 TotalKillCount 관리 |

### 7-13. 구현 순서 제안 (웨이브 한정)

```
Step 1: GameMode에 MonsterSpawner 참조 추가 + StartWave 호출
        └─ Night 시작 → StartWave("N-1") 한 줄 추가만으로 기본 동작 확인

Step 2: 서브웨이브 타이머 로직 (StartNextSubWave)
        └─ "N-1" → 60초 → "N-2" → 60초 → "N-3"

Step 3: Monster.Die() → GameMode.OnMonsterKilled() 연결
        └─ 킬 카운트 + AliveMonsterCount 관리

Step 4: Spawner에 OnSpawningFinished 델리게이트 추가
        └─ GameMode에서 바인딩 → bSpawningComplete 플래그

Step 5: 웨이브 클리어 판정 로직
        └─ bSpawningComplete && AliveMonsterCount <= 0

Step 6: Dawn에서 잔여 몬스터 Destroy + 통계 출력

Step 7: WaveMeta/WaveSpawn DataTable 에셋 제작
        └─ 10 Day × 3 Sub-wave = 30개 Wave 데이터
```

---

## 부록: 핵심 참조 관계 정리

```
[APotatoGameMode]
    │
    ├─ owns→ [UPotatoDayNightCycle]  (WorldSubsystem, GetSubsystem으로 접근)
    │         └─ 4개 Phase 델리게이트 → GameMode 바인딩 완료 ✅
    │
    ├─ owns→ [UPotatoResourceManager]  (WorldSubsystem, GetSubsystem으로 접근)
    │         └─ 자원 CRUD + 생산 틱 ✅
    │         └─ OnResourceChanged → HUD 바인딩 필요 ❌
    │
    ├─ refs→ [APotatoMonsterSpawner]  (레벨 배치 Actor, FindActor 또는 UPROPERTY)
    │         └─ StartWave(WaveId) / StopWave() ← GameMode가 호출해야 함 ❌
    │
    ├─ refs→ [APotatoPlaceableStructure] 창고  (레벨 배치)
    │         └─ 파괴 시 EndGame() 호출 필요 ❌
    │
    └─ 간접→ [APotatoNPC]  (월드에서 수집)
              └─ TryPayMaintenance() + Retire() ← GameMode가 Dawn에 호출해야 함 ❌

[APotatoPlayerCharacter]
    ├─ owns→ [UBuildingSystemComponent]
    │         └─ 페이즈별 배치 제한 체크 필요 ❌
    ├─ owns→ [UPotatoWeaponComponent]
    │         └─ 독립 동작 ✅
    └─ 사망 이벤트 → GameMode 통보 필요 ❌
```

### 범례
- ✅ = 이미 구현/연결됨
- ❌ = 구현 또는 연결 필요

---

> **요약**: GameMode는 현재 **델리게이트 Broadcast 껍데기**만 있는 상태입니다.
> 핵심 게임 루프를 완성하려면 **MonsterSpawner 연결**, **NPC 유지비 정산**, **게임 오버 조건 바인딩**을 우선적으로 구현해야 합니다.
> 가장 큰 변경은 `PotatoGameMode.h/cpp`에 집중되며, 각 서브시스템/액터는 이미 잘 분리되어 있어 GameMode에서 적절히 호출만 하면 됩니다.
