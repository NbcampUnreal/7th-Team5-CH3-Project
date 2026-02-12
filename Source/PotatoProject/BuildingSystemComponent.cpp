#include "BuildingSystemComponent.h"
#include "PlaceableStructure/PotatoStructureData.h"
#include "PlaceableStructure/PotatoPlaceableStructure.h"

UBuildingSystemComponent::UBuildingSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UBuildingSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UBuildingSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UBuildingSystemComponent::ToggleBuildMode(bool bEnable)
{
}

void UBuildingSystemComponent::SelectStructure(const UPotatoStructureData* NewData)
{
}
