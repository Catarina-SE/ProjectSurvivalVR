// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Environment/DayNightManager.h"
#include "SurvivalComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTSURVIVALVR_API USurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USurvivalComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Survival Stats
	
	// How fast the hunger depletes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Hunger")
	float HungerDepletionRate = 0.05f;

	// How fast the thirst depletes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Thirst")
	float ThirstDepletionRate = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Temperature")
	float NeutralTemperature = 25.0f;

	// How fast the temperature depletes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Temperature")
	float TemperatureDepletionRate = 0.03f;
	
	// Threshold for when the player is considered hungry
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Hunger")
	float CriticalHungerThreshold = 20.0f;

	// Threshold for when the player is considered thirsty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Thirst")
	float CriticalThirstThreshold = 15.0f;

	// Threshold for when the player is considered to be on hypothermia
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Temperature")
	float CriticalTemperatureThreshold = 0.0f;

	// Heat Zone Properties
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival | Temperature")
	bool bIsInShelteredZone = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival | Temperature")
	bool bIsInIntenseHeatZone = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival | Temperature")
	float ShelteredHeatRecoveryRate = 0.05f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival | Temperature")
	float IntenseHeatRecoveryRate = 0.2f;

	// Reference to day night manager for temperature modifiers
	UPROPERTY()
	ADayNightManager* DayNightManager = nullptr;

	// Timer for survival stats update
	FTimerHandle SurvivalUpdateTimerHandle;

	void InitializeSurvivalStats();
	void UpdateSurvivalStats();

	void ProcessHunger();
	void ProcessThirst();
	void ProcessTemperature();

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region Stamina System Variables
	
	// Stamina Events
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaDepleted OnStaminaDepleted;

	// Stamina Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MinStamina = 0.0f;

	// Stamina Depletion Rates
	// Stamina lost per second during normal activity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Depletion", meta = (ClampMin = "0.0"))
	float BaseStaminaDepletionRate = 0.4f; 

	// Stamina lost per second while climbing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Depletion", meta = (ClampMin = "0.0"))
	float ClimbingStaminaDepletionRate = 1.0f; 

	// Stamina lost per second while sprinting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Depletion", meta = (ClampMin = "0.0"))
	float SprintingStaminaDepletionRate = 0.8f; 

	// Activity State Tracking
	UPROPERTY(BlueprintReadOnly, Category = "Stamina|State")
	bool bIsClimbing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina|State")
	bool bIsSprinting = false;

	// Stamina Thresholds
	// Minimum stamina required to start climbing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MinStaminaForClimbing = 10.0f; 

	// Minimum stamina required to start sprinting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MinStaminaForSprinting = 5.0f; 

	// Below this, climbing is stopped
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CriticalStaminaForClimbing = 5.0f; 

	// Restoration Amounts
	// Base stamina restored by food
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Restoration", meta = (ClampMin = "0.0"))
	float FoodStaminaRestoration = 15.0f; 

	// Base stamina restored by drinks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Restoration", meta = (ClampMin = "0.0"))
	float DrinkStaminaRestoration = 8.0f; 

	// Stamina restored by sleeping (full restoration)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina|Restoration", meta = (ClampMin = "0.0"))
	float SleepStaminaRestoration = 100.0f; 

#pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Hunger", meta = (AllowPrivateAccess = "true"))
	float Hunger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Hunger")
	float MaxHunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival | Thirst", meta = (AllowPrivateAccess = "true"))
	float Thirst;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Thirst")
	float MaxThirst = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival | Temperature", meta = (AllowPrivateAccess = "true"))
	float Temperature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival | Temperature")
	float MaxTemperature = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Temperature")
	float MinTemperature = 0.0f;


#pragma region Stamina System Functions

	// Activity State Management
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetClimbingState(bool bClimbing);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetSprintingState(bool bSprinting);

	// Stamina Checks
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool CanStartClimbing() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool CanStartSprinting() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool ShouldForceStopClimbing() const;

	// Stamina Manipulation
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreStaminaFromFood(float FoodStaminaValue = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreStaminaFromDrink(float DrinkStaminaValue = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RestoreStaminaFromSleep();

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetStaminaPercentage() const;


#pragma endregion

	UFUNCTION(BlueprintCallable, Category = "Survival | Actions")
	void ConsumeFood(float NutritionValue);

	UFUNCTION(BlueprintCallable, Category = "Survival | Actions")
	void ConsumeDrink(float HydrationValue);

	// Heat zone functions
	UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
	void SetIsInShelteredZone(bool bInShelteredZone) { bIsInShelteredZone = bInShelteredZone; }

	UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
	void SetIsInIntenseHeatZone(bool bInIntenseHeatZone) { bIsInIntenseHeatZone = bInIntenseHeatZone; }

	UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
	void SetShelteredHeatRecoveryRate(float NewRate) { ShelteredHeatRecoveryRate = NewRate; }

	UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
	void SetIntenseHeatRecoveryRate(float NewRate) { IntenseHeatRecoveryRate = NewRate; }

	private:

	// Internal stamina processing
	void ProcessStamina();

	// Calculate current stamina depletion rate based on activity
	float GetCurrentStaminaDepletionRate() const;
};
