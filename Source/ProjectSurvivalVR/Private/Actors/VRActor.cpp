// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRActor.h"

AVRActor::AVRActor()
{
    PrimaryActorTick.bCanEverTick = false;

//     Root = CreateDefaultSubobject<USceneComponent>("Root");
//     RootComponent = Root;

    ActorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
//     ActorMesh->SetupAttachment(RootComponent);
    RootComponent = ActorMesh;
}

void AVRActor::BeginPlay()
{
    Super::BeginPlay();
}
