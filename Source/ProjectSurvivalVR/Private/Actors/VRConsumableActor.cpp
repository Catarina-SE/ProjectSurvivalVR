// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRConsumableActor.h"
#include "Components/SurvivalComponent.h"
#include "Characters/VRCharacterBase.h"
#include "Engine/World.h"

TArray<AVRConsumableActor*> AVRConsumableActor::ReusableConsumables;

AVRConsumableActor::AVRConsumableActor()
{
}

void AVRConsumableActor::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        CharacterReference = Cast<AVRCharacterBase>(PlayerController->GetPawn());
        if (CharacterReference)
        {
            SurvivalComponent = CharacterReference->FindComponentByClass<USurvivalComponent>();
        }
    }
}

AVRConsumableActor* AVRConsumableActor::GetOrCreate(UWorld* World, TSubclassOf<AVRConsumableActor> ConsumableClass, FVector Location)
{
    for (AVRConsumableActor* Item : ReusableConsumables)
    {
        if (Item && Item->IsHidden() && Item->GetClass() == ConsumableClass)
        {
            Item->Reactivate(Location);
            return Item;
        }
    }

    FActorSpawnParameters Params;
    AVRConsumableActor* NewItem = World->SpawnActor<AVRConsumableActor>(ConsumableClass, Location, FRotator::ZeroRotator, Params);
    if (NewItem)
    {
        ReusableConsumables.Add(NewItem);
    }
    return NewItem;
}

void AVRConsumableActor::Deactivate()
{
    ForceRelease();
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
}

void AVRConsumableActor::Reactivate(FVector NewLocation)
{
    SetActorLocation(NewLocation);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);
}

void AVRConsumableActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
}

void AVRConsumableActor::PrepareForDestroy()
{
    Super::PrepareForDestroy();
    ReusableConsumables.Remove(this);
    CharacterReference = nullptr;
    SurvivalComponent = nullptr;
}

void AVRConsumableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReusableConsumables.Remove(this);
    Super::EndPlay(EndPlayReason);
}