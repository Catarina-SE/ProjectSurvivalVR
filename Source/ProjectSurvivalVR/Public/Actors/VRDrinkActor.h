// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRConsumableActor.h"
#include "VRDrinkActor.generated.h"

UCLASS()
class PROJECTSURVIVALVR_API AVRDrinkActor : public AVRConsumableActor
{
    GENERATED_BODY()

public:
    AVRDrinkActor();

protected:
    virtual void BeginPlay() override;
    virtual void PrepareForDestroy() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Drink settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drink Settings")
    float HydrationValue = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drink Settings")
    float TotalWaterPercentage = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drink Settings")
    float WaterDecreaseRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drink Settings")
    float MinimumTiltAngleForDrinking = 45.0f;

    // Timer for repeated consumption
    FTimerHandle ConsumptionTimerHandle;

    // Helper function
    UFUNCTION(BlueprintPure, Category = "Drink")
    bool IsProperlyTiltedForDrinking() const;

public:
    virtual void Consume() override;

    // Stamina restoration || How much stamina this drink restores per consumption
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drink")
    float StaminaRestorationValue = 8.0f;
};