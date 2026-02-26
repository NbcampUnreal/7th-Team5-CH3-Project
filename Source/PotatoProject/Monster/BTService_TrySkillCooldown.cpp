#include "Monster/BTService_TrySkillCooldown.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "PotatoMonster.h"
#include "Monster/SpecialSkillComponent.h"

UBTService_TrySkillCooldown::UBTService_TrySkillCooldown()
{
	NodeName = TEXT("Try Special Skill (OnCooldown)");
	Interval = 0.3f;
	RandomDeviation = 0.1f;
}

void UBTService_TrySkillCooldown::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return;

	APotatoMonster* Monster = Cast<APotatoMonster>(AICon->GetPawn());
	if (!IsValid(Monster)) return;

	USpecialSkillComponent* SkillComp = Monster->FindComponentByClass<USpecialSkillComponent>();
	if (!IsValid(SkillComp)) return;

	// Busy sync
	if (bIsCastingSpecialKey.SelectedKeyType)
	{
		BB->SetValueAsBool(bIsCastingSpecialKey.SelectedKeyName, SkillComp->IsBusy());
	}

	// Target
	AActor* Target = nullptr;
	if (CurrentTargetKey.SelectedKeyType)
	{
		Target = Cast<AActor>(BB->GetValueAsObject(CurrentTargetKey.SelectedKeyName));
	}

	const bool bStarted = SkillComp->TryStartOnCooldown(Target);

	if (bDebugLog && bStarted)
	{
		UE_LOG(LogTemp, Log, TEXT("[BTService][OnCooldown] Started DefaultSkill Target=%s Monster=%s"),
			*GetNameSafe(Target),
			*GetNameSafe(Monster));
	}
}