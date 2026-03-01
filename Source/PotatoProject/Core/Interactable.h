#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APotatoPlayerController;

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class POTATOPROJECT_API IInteractable
{
	GENERATED_BODY()

public:
	// 플레이어가 범위에 진입했을 때 (위젯 표시 등)
	virtual void OnPlayerEnter(APotatoPlayerController* PC) = 0;
	// 플레이어가 범위에서 나갔을 때 (위젯 숨김 등)
	virtual void OnPlayerExit(APotatoPlayerController* PC) = 0;
	// F키 등 실제 상호작용 입력 시
	virtual void Interact(APotatoPlayerController* PC) = 0;
};