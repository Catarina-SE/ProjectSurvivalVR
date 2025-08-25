// Fill out your copyright notice in the Description page of Project Settings.

#include "Environment/DayNightManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ADayNightManager::ADayNightManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADayNightManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentHour = StartingHour;
	UpdateCachedValues();

	// Auto-find sun light if not set
	if (!SunLight)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightManager: SunLightNOTFound"),
			SunLight ? *SunLight->GetName() : TEXT("None"));
		TArray<AActor*> FoundLights;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADirectionalLight::StaticClass(), FoundLights);

		if (FoundLights.Num() > 0)
		{
			SunLight = Cast<ADirectionalLight>(FoundLights[0]);
			UE_LOG(LogTemp, Warning, TEXT("DayNightManager: Auto-found sun light: %s"),
				SunLight ? *SunLight->GetName() : TEXT("None"));
		}
	}

	// Initialize sun state
	if (SunLight)
	{
		UE_LOG(LogTemp, Warning, TEXT("DayNightManager: SunLightFound"),
			SunLight ? *SunLight->GetName() : TEXT("None"));
		UpdateSunRotation();
		UpdateSunIntensity();
	}

	// Set initial day/night state
	bWasNight = IsNight();

	UE_LOG(LogTemp, Warning, TEXT("DayNightManager initialized - Starting hour: %.2f, Is Night: %s"),
		CurrentHour, IsNight() ? TEXT("Yes") : TEXT("No"));
}

void ADayNightManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTime(DeltaTime);

	if (SunLight)
	{
		UpdateSunRotation();
		UpdateSunIntensity();
	}

	CheckDayNightTransition();
}

void ADayNightManager::UpdateTime(float DeltaTime)
{
	if (CachedDayLengthInSeconds <= 0.0f)
	{
		UpdateCachedValues();
	}

	// Calculate base time progression
	float HoursPerSecond = 24.0f / CachedDayLengthInSeconds;

	// Apply night speed multiplier if it's night
	float TimeMultiplier = IsNight() ? NightSpeedMultiplier : 1.0f;

	// Update current hour with speed modifier
	CurrentHour += HoursPerSecond * DeltaTime * TimeMultiplier;

	// Wrap around 24 hours
	if (CurrentHour >= 24.0f)
	{
		CurrentHour = FMath::Fmod(CurrentHour, 24.0f);
	}
}

void ADayNightManager::UpdateSunRotation()
{
	if (!SunLight)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Sunlight Found"), SunLight);
		return;
	}
		

	float SunAngle = (CurrentHour / 24.0f) * 360.0f;

	// Apply rotation - sun moves in a full circle
	FRotator NewRotation = FRotator(SunAngle, 0.0f, 0.0f);
	SunLight->SetActorRotation(NewRotation);
}

void ADayNightManager::UpdateSunIntensity()
{
	if (!SunLight || !SunLight->GetLightComponent())
		return;

	// Simple intensity based on day/night
	float TargetIntensity = IsDay() ? DayLightIntensity : NightLightIntensity;

	// Set the intensity
	SunLight->GetLightComponent()->SetIntensity(TargetIntensity);
}

void ADayNightManager::CheckDayNightTransition()
{
	bool bCurrentlyNight = IsNight();

	if (bWasNight != bCurrentlyNight)
	{
		if (bCurrentlyNight)
		{
			// Just became night
			OnNightStarted.Broadcast();
			UE_LOG(LogTemp, Warning, TEXT("Night started at hour %.2f"), CurrentHour);
		}
		else
		{
			// Just became day
			OnDayStarted.Broadcast();
			UE_LOG(LogTemp, Warning, TEXT("Day started at hour %.2f"), CurrentHour);
		}

		bWasNight = bCurrentlyNight;
	}
}

void ADayNightManager::UpdateCachedValues()
{
	CachedDayLengthInSeconds = DayLengthInMinutes * 60.0f;
}

FString ADayNightManager::GetFormattedTime() const
{
	int32 Hours = FMath::FloorToInt(CurrentHour);
	int32 Minutes = FMath::FloorToInt((CurrentHour - Hours) * 60.0f);

	return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

void ADayNightManager::SetCurrentHour(float NewHour)
{
	CurrentHour = FMath::Clamp(NewHour, 0.0f, 24.0f);

	if (SunLight)
	{
		UpdateSunRotation();
		UpdateSunIntensity();
	}

	// Check for immediate day/night transition
	CheckDayNightTransition();
}

bool ADayNightManager::SleepUntilMorning()
{
	if (!IsNight())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot sleep during daytime"));
		return false;
	}

	// Set time to morning (DayStartHour)
	SetCurrentHour(DayStartHour);

	UE_LOG(LogTemp, Warning, TEXT("Player slept until morning - Time set to %.2f:00"), DayStartHour);
	return true;
}

ADayNightManager* ADayNightManager::GetInstance(const UWorld* World)
{
	if (!World)
		return nullptr;

	// Find existing instance in world
	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(World, ADayNightManager::StaticClass(), FoundManagers);

	if (FoundManagers.Num() > 0)
	{
		return Cast<ADayNightManager>(FoundManagers[0]);
	}

	return nullptr;
}