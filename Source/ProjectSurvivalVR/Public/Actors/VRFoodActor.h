// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRConsumableActor.h"
#include "VRFoodActor.generated.h"

UCLASS()
class PROJECTSURVIVALVR_API AVRFoodActor : public AVRConsumableActor
{
    GENERATED_BODY()

public:
    AVRFoodActor();

protected:
    // How much nutrition the food provides
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food Settings")
    float NutritionValue = 25.0f;

public:
    virtual void Consume() override;

    // Stamina restoration | How much stamina this food restores
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
    float StaminaRestorationValue = 15.0f;
};