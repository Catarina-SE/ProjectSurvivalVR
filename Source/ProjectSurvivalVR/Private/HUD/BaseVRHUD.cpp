// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/BaseVRHUD.h"

void UBaseVRHUD::NativeConstruct()
{
    Super::NativeConstruct();

    APlayerController* PlayerController = GetOwningPlayer();
    if (PlayerController)
    {
        APawn* PlayerPawn = PlayerController->GetPawn();
        if (PlayerPawn)
        {
            SurvivalComponent = PlayerPawn->FindComponentByClass<USurvivalComponent>();
        }
    }

    if (SurvivalComponent)
    {
        GetWorld()->GetTimerManager().SetTimer(UpdateUITimer, this, &UBaseVRHUD::RefreshUI, 1.0f, true);
    }

}

void UBaseVRHUD::RefreshUI()
{
    Hunger = GetHungerPercentage();
    Thirst = GetThirstPercentage();
   // Temperature = GetTemperatureText();
    Stamina = GetStaminaPercentage();
}

float UBaseVRHUD::GetHungerPercentage()
{
    if (SurvivalComponent && SurvivalComponent->MaxHunger > 0)
    {
        float HungerPercentage = SurvivalComponent->Hunger / SurvivalComponent->MaxHunger;
        return FMath::Clamp(HungerPercentage, 0.0f, 1.0f); 
    }

    return 0.0f; 
}

float UBaseVRHUD::GetThirstPercentage()
{
    if (SurvivalComponent && SurvivalComponent->MaxThirst > 0)
    {
        float ThirstPercentage = SurvivalComponent->Thirst / SurvivalComponent->MaxThirst;
        return FMath::Clamp(ThirstPercentage, 0.0f, 1.0f);
    }

    return 0.0f;
}


FString UBaseVRHUD::GetTemperatureText() const
{
    if (SurvivalComponent)
    {
        int32 TempValue = FMath::RoundToInt(SurvivalComponent->Temperature);
        return FString::Printf(TEXT("%d°C"), TempValue);
    }
    return TEXT("0°C");
}


float UBaseVRHUD::GetStaminaPercentage()
{
    if (SurvivalComponent)
    {
        return SurvivalComponent->GetStaminaPercentage();
    }

    return 0.0f;
}

