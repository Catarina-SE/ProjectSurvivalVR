// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRGrabbableActor.h"
#include "Environment/DayNightManager.h"
#include "VRBed.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSleepStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSleepCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSleepFailed, FString, Reason);

UCLASS()
class PROJECTSURVIVALVR_API AVRBed : public AVRGrabbableActor
{
	GENERATED_BODY()

public:
	AVRBed();

protected:
	virtual void BeginPlay() override;
	virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation, bool bIsLeftHand = false, ECollisionChannel HandChannel = ECC_Pawn) override;

#pragma endregion

#pragma region Components

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* SleepInteractionSphere;

#pragma endregion

#pragma region Sleep Settings

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	bool bCanSleep = true;

	// Whether to show debug messages
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	bool bShowDebugMessages = true;

	// Whether sleeping restores stamina 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	bool bRestoreStamina = true;

	// How much stamina to restore
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings", meta = (EditCondition = "bRestoresHealth", ClampMin = "0.0", ClampMax = "100.0"))
	float StaminaRestoreAmount = 100.0f;

	// Whether sleeping affects temperature (warms you up)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	bool bWarmsPlayer = true;

	// How much to warm the player (added to temperature)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings", meta = (EditCondition = "bWarmsPlayer", ClampMin = "0.0", ClampMax = "10.0"))
	float WarmthAmount = 3.0f;

	// Duration of the fade effect in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	float FadeDuration = 5.0f;

	// How long to stay in black screen before waking up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	float BlackScreenDuration = 3.0f;

	// How long to show the morning before removing fade
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	float WakeUpDisplayDuration = 1.0f;

	// Widget class for sleep fade overlay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sleep Settings")
	TSubclassOf<class UUserWidget> SleepFadeWidgetClass;

#pragma endregion

#pragma region Events

public:
	// Called when player starts sleeping
	UPROPERTY(BlueprintAssignable, Category = "Sleep Events")
	FOnSleepStarted OnSleepStarted;

	// Called when sleep is completed successfully
	UPROPERTY(BlueprintAssignable, Category = "Sleep Events")
	FOnSleepCompleted OnSleepCompleted;

	// Called when sleep fails (with reason)
	UPROPERTY(BlueprintAssignable, Category = "Sleep Events")
	FOnSleepFailed OnSleepFailed;

#pragma endregion

#pragma region Internal State

private:
	// Reference to the day/night manager
	UPROPERTY()
	ADayNightManager* DayNightManager = nullptr;

	bool bIsBeingUsed = false;

	// Timer handle for sleep transition
	FTimerHandle SleepTransitionTimer;

	// Reference to the fade widget
	UPROPERTY()
	class UUserWidget* CurrentFadeWidget = nullptr;

	// Actor currently sleeping (for delayed sleep completion)
	UPROPERTY()
	AActor* CurrentSleepingActor = nullptr;

#pragma endregion

#pragma region Sleep Functions

protected:

	// Overlap events for sleep interaction
	UFUNCTION()
	void OnSleepInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

// 	UFUNCTION()
// 	void OnSleepInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
// 		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Core sleep functionality
	UFUNCTION(BlueprintCallable, Category = "Sleep")
	void AttemptSleep(AActor* SleepingActor);

	// Apply sleep benefits to the player
	void ApplySleepBenefits(AActor* Player);

	// Check if the given actor can sleep
	bool CanActorSleep(AActor* Actor) const;

	// Start the fade-to-black effect
	void StartSleepFade();

	// Skip time to morning (happens during black screen)
	UFUNCTION()
	void SkipTimeToMorning();

	// Complete sleep and remove fade
	UFUNCTION()
	void CompleteSleepAndRemoveFade();

	// Remove the fade widget
	void RemoveFadeWidget();

#pragma endregion

public:

#pragma region Public Interface

	// Manual sleep trigger
	UFUNCTION(BlueprintCallable, Category = "Sleep")
	bool TriggerSleep(AActor* SleepingActor);

	// Check if bed is available for use
	UFUNCTION(BlueprintPure, Category = "Sleep")
	bool IsAvailable() const { return bCanSleep && !bIsBeingUsed; }

	// Check if it's currently night time 
	UFUNCTION(BlueprintPure, Category = "Sleep")
	bool IsNightTime() const;

#pragma endregion
};