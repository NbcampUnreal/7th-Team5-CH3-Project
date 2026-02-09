// Fill out your copyright notice in the Description page of Project Settings.


#include "PotatoPlaceableStructure.h"

// Sets default values
APotatoPlaceableStructure::APotatoPlaceableStructure()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APotatoPlaceableStructure::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APotatoPlaceableStructure::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

