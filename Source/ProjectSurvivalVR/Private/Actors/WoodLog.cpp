// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/WoodLog.h"
#include "Engine/Engine.h"

AWoodLog::AWoodLog()
{
    UE_LOG(LogTemp, Warning, TEXT("WoodLog Constructor - Created wood log"));
}

void AWoodLog::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("WoodLog BeginPlay - Wood log spawned at: %s"), *GetActorLocation().ToString());

    // Debug: Show on screen that wood log exists
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue,
            FString::Printf(TEXT("Wood log spawned at: %s"), *GetActorLocation().ToString()));
    }
}

void AWoodLog::NotifyPlaced()
{
    SafeDestroy();
}
