// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRFoodActor.h"
#include "Components/SurvivalComponent.h"

AVRFoodActor::AVRFoodActor()
{
}

void AVRFoodActor::Consume()
{
    if (!SurvivalComponent || !CharacterReference) return;

    if (SurvivalComponent->Hunger < SurvivalComponent->MaxHunger)
    {
        SurvivalComponent->ConsumeFood(NutritionValue);
        UE_LOG(LogTemp, Warning, TEXT("Food consumed: %f"), NutritionValue);

        // Restore stamina with the specific value for this food
        SurvivalComponent->RestoreStaminaFromFood(StaminaRestorationValue);

        UE_LOG(LogTemp, Warning, TEXT("Food consumed: Nutrition=%.2f, Stamina=%.2f"),NutritionValue, StaminaRestorationValue);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                FString::Printf(TEXT("Food restored %.0f stamina"), StaminaRestorationValue));
        }

        // Call blueprint event
        OnConsumed();

        Deactivate();
    }
}