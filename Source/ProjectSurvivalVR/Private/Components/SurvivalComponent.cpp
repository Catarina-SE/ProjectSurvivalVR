// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SurvivalComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

USurvivalComponent::USurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USurvivalComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeSurvivalStats();

    // Find DayNightManager in the world
    DayNightManager = ADayNightManager::GetInstance(GetWorld());
    if (!DayNightManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("SurvivalComponent: No DayNightManager found in world"));
    }
}

void USurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Survival Stats and Functions

void USurvivalComponent::InitializeSurvivalStats()
{
    Hunger = MaxHunger;
    Thirst = MaxThirst;
	Temperature = NeutralTemperature;
    Stamina = MaxStamina;

    GetWorld()->GetTimerManager().SetTimer(SurvivalUpdateTimerHandle, this, &USurvivalComponent::UpdateSurvivalStats, 1.0f, true);
}

void USurvivalComponent::UpdateSurvivalStats()
{
    float DeltaTime = GetWorld()->GetDeltaSeconds();

    // Processes each survival stat
    ProcessHunger();
    ProcessThirst();
	ProcessTemperature();
    ProcessStamina();

}

void USurvivalComponent::ProcessHunger()
{
    float hungerDepletion = HungerDepletionRate;

    Hunger = FMath::Max(0.0f, Hunger - HungerDepletionRate);

	UE_LOG(LogTemp, Warning, TEXT("Hunger: %f"), Hunger);
}

void USurvivalComponent::ProcessThirst()
{
    float thirstDepletion = ThirstDepletionRate;

    Thirst = FMath::Max(0.0f, Thirst - thirstDepletion);

	UE_LOG(LogTemp, Warning, TEXT("Thirst: %f"), Thirst);
}

void USurvivalComponent::ProcessTemperature()
{
    // Default behavior: temperature depletes over time
    float temperatureChange = -TemperatureDepletionRate;

    // Apply day/night temperature modifier if DayNightManager exists
    float timeModifier = 0.0f;
    bool bIsNight = false;
    if (DayNightManager)
    {
        timeModifier = DayNightManager->GetTemperatureModifier();
        bIsNight = DayNightManager->IsNight();
        temperatureChange += timeModifier * 0.01f; // Scale it down to be subtle
    }

    // Check if character is in any heat zone and apply the appropriate recovery rate
    if (bIsInIntenseHeatZone)
    {
        // Character is in intense heat zone (near fire) - overrides time modifier
        temperatureChange = IntenseHeatRecoveryRate;
    }
    else if (bIsInShelteredZone)
    {
        // Character is in sheltered zone - combines with time modifier
        temperatureChange = FMath::Max(temperatureChange, ShelteredHeatRecoveryRate);
    }

    // Apply the temperature change (either depletion or recovery)
    Temperature = FMath::Clamp(Temperature + temperatureChange, MinTemperature, MaxTemperature);

    // LOG TEMPERATURE DEBUG INFO
    UE_LOG(LogTemp, Warning, TEXT("=== TEMPERATURE === Current: %.2f, Change: %.2f, TimeModifier: %.2f, IsNight: %s"),
        Temperature, temperatureChange, timeModifier, bIsNight ? TEXT("YES") : TEXT("NO"));
}

void USurvivalComponent::ConsumeFood(float NutritionValue)
{
    Hunger = FMath::Min(MaxHunger, Hunger + NutritionValue);

    RestoreStaminaFromFood();
}

void USurvivalComponent::ConsumeDrink(float HydrationValue)
{
    Thirst = FMath::Min(MaxThirst, Thirst + HydrationValue);

    RestoreStaminaFromDrink();
}

#pragma region Stamina System Functions

void USurvivalComponent::ProcessStamina()
{
    float staminaDepletion = GetCurrentStaminaDepletionRate();

    // Apply stamina depletion
    Stamina = FMath::Max(MinStamina, Stamina - staminaDepletion);

    // Check if we need to force stop climbing due to low stamina
    if (bIsClimbing && ShouldForceStopClimbing())
    {
        UE_LOG(LogTemp, Warning, TEXT("Forcing climbing stop due to low stamina: %.2f"), Stamina);

        // Notify character to stop climbing (we'll implement this callback next)
        OnStaminaDepleted.Broadcast();

        SetClimbingState(false);
    }
}

float USurvivalComponent::GetCurrentStaminaDepletionRate() const
{
    if (bIsClimbing)
    {
        return ClimbingStaminaDepletionRate;
    }
    else if (bIsSprinting)
    {
        return SprintingStaminaDepletionRate;
    }
    else
    {
        return BaseStaminaDepletionRate;
    }
}


void USurvivalComponent::SetClimbingState(bool bClimbing)
{
    if (bIsClimbing != bClimbing)
    {
        bIsClimbing = bClimbing;
    }

}

void USurvivalComponent::SetSprintingState(bool bSprinting)
{
    if (bIsSprinting != bSprinting)
    {
        bIsSprinting = bSprinting;
        UE_LOG(LogTemp, Warning, TEXT("Sprinting state changed to: %s"), bSprinting ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

bool USurvivalComponent::CanStartClimbing() const
{
    return Stamina >= MinStaminaForClimbing;
}

bool USurvivalComponent::CanStartSprinting() const
{
    return Stamina >= MinStaminaForSprinting;
}

bool USurvivalComponent::ShouldForceStopClimbing() const
{
    return Stamina <= CriticalStaminaForClimbing;
}

void USurvivalComponent::ConsumeStamina(float Amount)
{
    Stamina = FMath::Max(MinStamina, Stamina - Amount);
    UE_LOG(LogTemp, VeryVerbose, TEXT("Consumed %.2f stamina. Current: %.2f"), Amount, Stamina);
}

void USurvivalComponent::RestoreStamina(float Amount)
{
    Stamina = FMath::Min(MaxStamina, Stamina + Amount);
    UE_LOG(LogTemp, Warning, TEXT("Restored %.2f stamina. Current: %.2f"), Amount, Stamina);
}

void USurvivalComponent::RestoreStaminaFromFood(float FoodStaminaValue)
{
    float RestorationAmount = (FoodStaminaValue > 0) ? FoodStaminaValue : FoodStaminaRestoration;
    RestoreStamina(RestorationAmount);
    UE_LOG(LogTemp, Warning, TEXT("Food restored %.2f stamina"), RestorationAmount);
}

void USurvivalComponent::RestoreStaminaFromDrink(float DrinkStaminaValue)
{
    float RestorationAmount = (DrinkStaminaValue > 0) ? DrinkStaminaValue : DrinkStaminaRestoration;
    RestoreStamina(RestorationAmount);
    UE_LOG(LogTemp, Warning, TEXT("Drink restored %.2f stamina"), RestorationAmount);
}

void USurvivalComponent::RestoreStaminaFromSleep()
{
    RestoreStamina(SleepStaminaRestoration);
    UE_LOG(LogTemp, Warning, TEXT("Sleep restored %.2f stamina (full restoration)"), SleepStaminaRestoration);
}

float USurvivalComponent::GetStaminaPercentage() const
{
    if (MaxStamina > 0)
    {
        return FMath::Clamp(Stamina / MaxStamina, 0.0f, 1.0f);
    }
    return 0.0f;
}


#pragma endregion


