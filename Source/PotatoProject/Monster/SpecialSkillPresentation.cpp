#include "SpecialSkillPresentation.h"
#include "SpecialSkillComponent.h"

#include "Engine/World.h"
#include "SkillTransformResolver.h"
#include "FXUtils//PotatoFXUtils.h"

static void PlayPresentationCommon(
	USpecialSkillComponent* Comp,
	const FPotatoMonsterSpecialSkillPresetRow& Row,
	AActor* Target,
	const FPotatoSkillVFXSlot& Vfx,
	const FPotatoSkillSFXSlot& Sfx,
	const TCHAR* PhaseName)
{
	if (!Comp) return;

	AActor* Owner = Comp->GetOwner();
	if (!IsValid(Owner) || Owner->IsActorBeingDestroyed()) return;

	UWorld* World = Comp->GetWorld();
	if (!World || World->bIsTearingDown) return;

	const FVector Origin = FSkillTransformResolver::ResolveOrigin(Owner, Target, Row);

	// VFX/SFX 공통 거리 정책(유틸)
	if (!PotatoFX::PassesFxDistanceGate(World, Origin, Row.MaxFxDistance))
	{
		return;
	}

	const FRotator Facing = Owner->GetActorRotation();

	// ✅ VFX / SFX 실제 스폰은 전부 Utils가 책임
	PotatoFX::PlaySkillVFXSlot(Comp, Vfx, Origin, Facing);

	const bool bImportant = (FCString::Stricmp(PhaseName, TEXT("Execute")) == 0);
	PotatoFX::PlaySkillSFXSlot(Comp, Sfx, Origin, Row.MaxFxDistance, bImportant);
}

void FSpecialSkillPresentation::PlayTelegraph(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.TelegraphVFX, Row.Presentation.TelegraphSFX, TEXT("Telegraph"));
}
void FSpecialSkillPresentation::PlayCast(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.CastVFX, Row.Presentation.CastSFX, TEXT("Cast"));
}
void FSpecialSkillPresentation::PlayExecute(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.ExecuteVFX, Row.Presentation.ExecuteSFX, TEXT("Execute"));
}
void FSpecialSkillPresentation::PlayEnd(USpecialSkillComponent* Comp, const FPotatoMonsterSpecialSkillPresetRow& Row, AActor* Target)
{
	PlayPresentationCommon(Comp, Row, Target, Row.Presentation.EndVFX, Row.Presentation.EndSFX, TEXT("End"));
}