// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "DayNightManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDayStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNightStarted);

UCLASS()
class PROJECTSURVIVALVR_API ADayNightManager : public AActor
{
	GENERATED_BODY()

public:
	ADayNightManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#pragma region Settings

	// How long a full day takes in real minutes (default: 20 minutes = 24 game hours)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
	float DayLengthInMinutes = 20.0f;

	// Speed multiplier for night time (2.0 = night passes twice as fast)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float NightSpeedMultiplier = 2.0f;

	// Starting hour of the day (0-24, default: 12 = noon)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartingHour = 12.0f;

	// Hour when day begins (sunrise)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float DayStartHour = 6.0f;

	// Hour when night begins (sunset)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float NightStartHour = 18.0f;

	// Reference to the directional light that acts as the sun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	ADirectionalLight* SunLight = nullptr;

	// Sun intensity during day
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float DayLightIntensity = 3.0f;

	// Sun intensity during night
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float NightLightIntensity = 0.8f;

	// Temperature modifier during day (added to base temperature)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float DayTemperatureModifier = 1.0f;

	// Temperature modifier during night (added to base temperature)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float NightTemperatureModifier = -2.0f;

#pragma endregion

#pragma region Events

	// Called when day starts (sunrise)
	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnDayStarted OnDayStarted;

	// Called when night starts (sunset)
	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnNightStarted OnNightStarted;

#pragma endregion

private:

#pragma region Internal State

	// Current game hour (0.0 - 24.0)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Current Time", meta = (AllowPrivateAccess = "true"))
	float CurrentHour = 12.0f;

	// Track previous day/night state for event triggering
	bool bWasNight = false;

	// Cache for performance
	float CachedDayLengthInSeconds = 0.0f;

#pragma endregion

#pragma region Internal Functions

	void UpdateTime(float DeltaTime);
	void UpdateSunRotation();
	void UpdateSunIntensity();
	void CheckDayNightTransition();
	void UpdateCachedValues();

#pragma endregion

public:

#pragma region Public Interface

	// Get current hour (0.0 - 24.0)
	UFUNCTION(BlueprintPure, Category = "Time")
	float GetCurrentHour() const { return CurrentHour; }

	// Check if it's currently day
	UFUNCTION(BlueprintPure, Category = "Time")
	bool IsDay() const {
		// Day is from DayStartHour to NightStartHour
		// 6.0 <= CurrentHour < 18.0 = Day
		// Everything else = Night
		return CurrentHour >= DayStartHour && CurrentHour < NightStartHour;
	}

	// Check if it's currently night
	UFUNCTION(BlueprintPure, Category = "Time")
	bool IsNight() const { return !IsDay(); }

	// Get temperature modifier based on current time
	UFUNCTION(BlueprintPure, Category = "Temperature")
	float GetTemperatureModifier() const { return IsDay() ? DayTemperatureModifier : NightTemperatureModifier; }

	// Get current time as formatted string (e.g., "14:30")
	UFUNCTION(BlueprintPure, Category = "Time")
	FString GetFormattedTime() const;

	// Set current hour (useful for testing or save/load systems)
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetCurrentHour(float NewHour);

	// Sleep until morning (skip night)
	UFUNCTION(BlueprintCallable, Category = "Sleep")
	bool SleepUntilMorning();

	// Check if player can sleep (only at night)
	UFUNCTION(BlueprintPure, Category = "Sleep")
	bool CanSleep() const { return IsNight(); }

	// Get singleton instance (for easy access from other systems)
	UFUNCTION(BlueprintPure, Category = "Time")
	static ADayNightManager* GetInstance(const UWorld* World);

#pragma endregion
};