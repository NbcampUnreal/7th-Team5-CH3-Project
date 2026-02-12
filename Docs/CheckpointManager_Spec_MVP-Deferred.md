# CheckpointManager (MVP Defer) — 명세 초안

> 목적: **Day 실패 시, 레벨을 유지한 채로** “Day 시작 상태”로 되돌리는 기능의 *명세*를 남긴다.
>
> MVP(현재): **Checkpoint 기능은 구현하지 않음**. 1 Day 기준으로만 게임 루프/자원/무기/건설을 먼저 완성한다.

---

## 용어

- **Snapshot(스냅샷)**: 특정 시점의 상태를 담은 데이터 묶음.
- **Checkpoint(체크포인트)**: 스냅샷을 저장해 두고, 실패 시 복원하는 기능.
- **Day Checkpoint**: “Day 시작(낮 시작)” 시점에 저장되는 체크포인트.

> Unreal Engine에 Checkpoint/Snapshot 기능이 기본 내장돼 있지는 않으며, 프로젝트 요구사항을 구현하기 위한 설계 용어다.

---

## 핵심 요구사항

1. **싱글 게임**
2. Day 실패(게임 오버) 시:
   - **현재 레벨은 리로드하지 않는다.**
   - “Day 시작” 상태로 **자원/탄약/플레이어 HP/구조물 배치 및 HP**를 복구한다.
   - 플레이어는 **PlayerStart 위치에서 재시작**한다.
3. 밤 종료 시 NPC 유지비 정산은 정상 흐름이지만, 실패 후 Restore 시점에서는 체크포인트로 되돌아가므로 결과적으로 정산은 의미가 없다(롤백됨).

### 반드시 문서로 박아둘 전제조건(채팅 없이 구현 재개 가능하게)

- Restore는 `OpenLevel`/맵 전환으로 처리하지 않는다.
  - 즉, **World는 유지**하며, 게임 오브젝트들의 상태만 스냅샷에 맞춰 **Destroy/Respawn 또는 값 덮어쓰기**로 복원한다.
- 체크포인트는 **Day 시작(낮 시작) 시점**에 1회 저장한다.
- 실패 시 복원 결과는 “그 Day 시작 시점”과 **동일한 플레이 가능 상태**여야 한다.

---

## 시스템 경계(역할 분리)

### UCheckpointManager (오케스트레이터)

- 참가 시스템들에게 `Capture()`를 요청해 스냅샷을 만든다.
- Restore 시 참가 시스템들에게 `Restore()`를 요청해 스냅샷을 적용한다.
- 체크포인트의 수명은 **런타임 메모리**로 관리한다.
  - Main Menu로 나가면 폐기되는 것이 기본 정책.

### 참가 시스템(Participant)

각 시스템은 “자기 상태”만 스냅샷으로 내보내고 복원한다.

- `UResourceManager` (자원)
- `UWeaponSystem` / 무기 또는 탄약 제작 UI 시스템 (탄약/슬롯)
- `UBuildSystemComponent` (구조물 목록 + 배치 상태)
- `APlayerCharacter` (HP 등)
- (추후) 웨이브/낮밤/몬스터 스포너 관련 시스템

---

## Snapshot 데이터(초안)

> 실제 필드는 구현 시점에 프로젝트 클래스/데이터에 맞춰 조정.

- Resources
  - Wood / Stone / Crop / Livestock
- Player
  - Health / MaxHealth
  - (재시작 위치는 PlayerStart로 강제, 좌표 저장 대신 PlayerStartTag 같은 간접키 고려)
- Weapons
  - CurrentSlotIndex
  - 각 슬롯의 CurrentClip / ReserveAmmo / MaxClipSize / WeaponId
- Build
  - 배치된 구조물 목록
    - StructureId
    - GridCoord
    - RotationStep90
    - CurrentHP / MaxHP
- Wave (추후)

- Wave (추후)
  - DayIndex
  - (결정 필요) 아래 중 어떤 것을 “복원”할지 먼저 선택하고 구현한다.
    - Option A (권장, 단순): **웨이브 진행은 무조건 Night 시작 상태로 초기화**
      - 저장할 것: `DayIndex` 정도만(또는 Night이 시작됐는지 여부)
      - 복원 시: Spawner/몬스터는 정리(Destroy)하고, Night 진행은 0부터 재시작
    - Option B (정교, 비용↑): **Night 진행도까지 복원**
      - 저장 후보 필드 예시:
        - `WaveId` / `CurrentWaveIndex`
        - `ElapsedNightTime`
        - `PendingSpawnEntries` 또는 `SpawnRowCursor`
      - MVP 이후에만 고려

---

## (초안) 코드 스니펫 — Snapshot USTRUCT / Participant Interface

> 목적: 이 문서만 보고도 이후 구현자가 빠르게 착수할 수 있도록 “헤더 뼈대”를 남긴다.
> 
> 주의: 아래 코드는 **초안**이며, 실제 프로젝트의 모듈명/클래스명/경로는 구현 시점에 맞춰 조정한다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CheckpointTypes.generated.h"

UENUM(BlueprintType)
enum class ECheckpointPhase : uint8
{
  Day   UMETA(DisplayName="Day"),
  Night UMETA(DisplayName="Night"),
};

USTRUCT(BlueprintType)
struct FResourceSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Wood = 0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Stone = 0; // 또는 Ore로 통일(리팩터 시점에 결정)
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Crop = 0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Livestock = 0;
};

USTRUCT(BlueprintType)
struct FPlayerSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) float Health = 100.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHealth = 100.f;

  // 요구사항: Restore 시 PlayerStart로 되돌림.
  // 따라서 월드 좌표를 직접 저장하기보단 Tag/Id 기반을 권장.
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PlayerStartTag = "Default";
};

USTRUCT(BlueprintType)
struct FAmmoSlotSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName WeaponId;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentClip = 0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReserveAmmo = 0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxClipSize = 0;
};

USTRUCT(BlueprintType)
struct FWeaponSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentSlotIndex = 0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FAmmoSlotSnapshot> Slots;
};

USTRUCT(BlueprintType)
struct FPlacedStructureSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StructureId;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint GridCoord = FIntPoint::ZeroValue;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 RotationStep90 = 0;

  // 요구사항: HP 포함 복원
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentHP = 0.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHP = 0.f;
};

USTRUCT(BlueprintType)
struct FBuildSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<FPlacedStructureSnapshot> PlacedStructures;
};

USTRUCT(BlueprintType)
struct FRunCheckpointSnapshot
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DayIndex = 1;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) ECheckpointPhase Phase = ECheckpointPhase::Day;

  UPROPERTY(EditAnywhere, BlueprintReadWrite) FResourceSnapshot Resources;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FPlayerSnapshot Player;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FWeaponSnapshot Weapons;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FBuildSnapshot Build;

  // Wave는 Option A/B 선택 후 확정
};

UINTERFACE(BlueprintType)
class UCheckpointParticipant : public UInterface
{
  GENERATED_BODY()
};

class ICheckpointParticipant
{
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Checkpoint")
  FName GetParticipantId() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Checkpoint")
  void CaptureToSnapshot(FRunCheckpointSnapshot& InOutSnapshot) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Checkpoint")
  void RestoreFromSnapshot(const FRunCheckpointSnapshot& Snapshot);
};
```

---

## Restore 방식(레벨 유지)

레벨 리로드 없이 복구하므로, `UBuildSystemComponent`가 복구 정책을 가져야 한다.

권장 정책(단순/안전):

1. **현재 배치된 구조물 전체 제거(Destroy)**
2. 스냅샷의 `PlacedStructures`를 순회하며 **재배치(Spawn/Place)**
3. 각 구조물의 HP를 스냅샷 값으로 덮어쓰기

### Restore 순서(권장)

복원 시점에 시스템 간 의존이 생기기 쉬우므로, 호출 순서를 문서로 고정해 둔다.

1) (선택) 전투/웨이브 정리
  - 살아있는 몬스터 제거(또는 스포너 정지)
2) Build 복원
  - 구조물 Destroy/Respawn → HP 적용
3) Resource 복원
  - 자원 덮어쓰기 + UI 이벤트
4) Weapon/Ammo 복원
  - 슬롯/탄창/예비탄 덮어쓰기
5) Player 복원
  - HP 덮어쓰기
  - PlayerStart로 텔레포트(또는 재스폰)
6) (선택) DayNight/Wave 재시작
  - Option A를 따를 경우: Night 진행 초기화

> “체크포인트 이후에 배치한 것만 제거” 최적화도 가능하지만, MVP 이후에 고려.

---

## 트리거 시점(초안)

- `SaveDayCheckpoint()`
  - Day 시작(낮 시작) 직후 1회
  - 혹은 `ResultPhase` 성공 처리 이후 다음 Day 들어가기 직전

- `RestoreDayCheckpoint()`
  - 실패(GameOver) 처리에서 호출

### 어디서 호출할지(연동 지점 후보)

- Day 시작 시점
  - `APotatoGameMode`가 Day 시작을 주도한다면 GameMode에서 호출
  - 또는 이미 구현된 `PotatoDayNightCycle`의 Day Start 이벤트에서 호출

- 실패 시점
  - 플레이어 사망 또는 창고 파괴를 감지하는 곳(대개 GameMode/Rule)에서 호출
  - 실패 UI 출력 후 “Restart Day” 버튼에서 호출해도 됨

---

## 구현 순서(Checkpoint 착수 시점)

1. `UResourceManager` / `UWeaponSystem` / `UBuildSystemComponent`가 먼저 안정화
2. Participant 인터페이스 도입
3. `UCheckpointManager`가 참가자를 등록하고 스냅샷 Capture/Restore 호출
4. GameMode(또는 DayNightCycle)에서 Day 시작/실패 시점에 Save/Restore 연결

---

## 리스크/주의사항

- 복원 시 순서가 중요할 수 있음
  - 예: 구조물 복원 전에 자원이 먼저 복원돼야 UI/검증 로직이 깨지지 않음
- Ghost/Preview Actor(건설 모드 유령)는 스냅샷 대상 아님
- 구조물의 ‘코어 구조물(창고)’은 복원 정책에서 예외처리가 필요할 수 있음

---

## ‘채팅 없이’ 구현 재개를 위한 TODO 체크리스트

아래 항목이 완료되면, 대화 맥락 없이도 체크포인트 구현을 바로 착수할 수 있다.

- [ ] Wave 복원 정책 최종 확정(Option A/B)
- [ ] Build 복원 정책 확정(Destroy/Respawn 전체 재구성으로 시작)
- [ ] PlayerStart 재시작 방식 확정
  - PlayerStart가 1개인지, Tag로 구분할지
- [ ] 참가자(Participant) 목록 확정
  - ResourceManager / BuildSystem / WeaponSystem / PlayerCharacter (+ Wave/Spawner)
- [ ] Restore 호출 순서 고정(위 ‘Restore 순서(권장)’을 코드 규칙으로 유지)

